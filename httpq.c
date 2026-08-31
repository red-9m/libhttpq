#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <curl/curl.h>
#include <string.h>

#include "httpq.h"

#define RESP_DEFAULT_LEN  (8 * 1024)
#define RESP_DEFAULT_LIMIT (4 * 1024 * 1024)
#define REQ_DEFAULT_MAXTIME 20
#define REQ_MAXKEYS 512

#if LIBCURL_VERSION_NUM < 0x080b01
#error libhttpq requires libcurl 8.11.1 or newer
#endif

const int HTTPQ_OK = CURLE_OK;

static pthread_once_t g_curl_once = PTHREAD_ONCE_INIT;
static CURLcode g_curl_init_result = CURLE_FAILED_INIT;

struct httpq_client
{
    CURL *curl;
    char *post;
    curl_mime *mimepost;
    long post_len;
    struct curl_slist *headers;
    long resp_limit;
    long maxtime_limit;
    enum httpq_retry_policy retry_policy;
};

static _Thread_local struct httpq_client g_client = {
    NULL, NULL, NULL, 0, NULL,
    RESP_DEFAULT_LIMIT, REQ_DEFAULT_MAXTIME, rpNoRetry
};

static void initialize_curl(void)
{
    const curl_version_info_data *version;

    g_curl_init_result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (g_curl_init_result != CURLE_OK)
        return;

    version = curl_version_info(CURLVERSION_NOW);
    if (!version || !(version->features & CURL_VERSION_THREADSAFE) ||
        atexit(curl_global_cleanup) != 0)
    {
        curl_global_cleanup();
        g_curl_init_result = CURLE_FAILED_INIT;
    }
}

struct curl_callback_data
{
    char* buffer;
    size_t len;
    size_t allocated;
    size_t limit;
};

static void cleanup(void)
{
    if (g_client.curl)
    {
        curl_easy_cleanup(g_client.curl);
        curl_mime_free(g_client.mimepost);
        curl_slist_free_all(g_client.headers);
        g_client.curl = NULL;
        g_client.mimepost = NULL;
        g_client.headers = NULL;

        if (g_client.post && g_client.post_len > 0)
        {
            free(g_client.post);
            g_client.post = NULL;
            g_client.post_len = 0;
        }
    }

    g_client.resp_limit = RESP_DEFAULT_LIMIT;
    g_client.maxtime_limit = REQ_DEFAULT_MAXTIME;
    g_client.retry_policy = rpNoRetry;
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t data_size;
    size_t required;
    struct curl_callback_data *data = (struct curl_callback_data*)userp;

    if (nmemb != 0 && size > SIZE_MAX / nmemb)
        return 0;
    data_size = size * nmemb;

    if (data->len > data->limit || data_size > data->limit - data->len)
        return 0;

    required = data->len + data_size + 1;
    if (data->allocated < required)
    {
        char *new_buffer;
        size_t new_allocated = data->allocated;

        if (new_allocated <= (data->limit + 1) / 2)
            new_allocated *= 2;
        else
            new_allocated = data->limit + 1;
        if (new_allocated < required)
            new_allocated = required;
        if (new_allocated > data->limit + 1)
            new_allocated = data->limit + 1;

        new_buffer = realloc(data->buffer, new_allocated);
        if (!new_buffer)
            return 0;
        data->buffer = new_buffer;
        data->allocated = new_allocated;
    }

    memcpy(data->buffer + data->len, contents, data_size);
    data->len += data_size;
    data->buffer[data->len] = '\0';

    return data_size;
}

static long post_resize(long postLen)
{
    char *new_post = realloc(g_client.post, postLen);

    if (!new_post)
        return CURLE_OUT_OF_MEMORY;

    g_client.post = new_post;
    g_client.post_len = postLen;
    return CURLE_OK;
}

long httpq_init(void)
{
    long result = CURLE_FAILED_INIT;

    if (pthread_once(&g_curl_once, initialize_curl) != 0)
        return CURLE_FAILED_INIT;
    if (g_curl_init_result != CURLE_OK)
        return g_curl_init_result;

    if (!g_client.curl)
        g_client.curl = curl_easy_init();

    if (g_client.curl)
        result = CURLE_OK;

    return result;
}

void httpq_cleanup(void)
{
    cleanup();
}

long httpq_set_url(const char *aURL)
{
    if (!aURL)
        return CURLE_BAD_FUNCTION_ARGUMENT;
    else
        return curl_easy_setopt(g_client.curl, CURLOPT_URL, aURL);
}

long httpq_set_post(const char *postData)
{
    long local_res;
    long result = CURLE_BAD_FUNCTION_ARGUMENT;
    long post_len = 0;

    if (!postData)
        return result;

    post_len = strlen(postData);
    post_len++; // For trailing zero

    if (post_len > g_client.post_len)
    {
        result = post_resize(post_len);
        if (result != CURLE_OK)
            return result;
    }

    local_res = snprintf(g_client.post, g_client.post_len, "%s", postData);

    if (local_res >= g_client.post_len)
        result = CURLE_HTTP_POST_ERROR;
    else
        result = curl_easy_setopt(g_client.curl, CURLOPT_POSTFIELDS, g_client.post);

    return result;
}

long httpq_set_key_post(const char *postData[][2])
{
    long local_res, offset = 0;
    long result = CURLE_OK;
    char* escaped_posts[REQ_MAXKEYS];
    long post_len = 0;
    long item_count = 0;
    int i = 0;

    if (!postData)
        return CURLE_BAD_FUNCTION_ARGUMENT;

    while (postData[i][0])
    {
        i++;
    }
    item_count = i;

    if (item_count > REQ_MAXKEYS)
        return CURLE_BAD_FUNCTION_ARGUMENT;

    for (i = 0; i < item_count; i++)
    {
        escaped_posts[i] = curl_easy_escape(g_client.curl, postData[i][1], 0);
        if (!escaped_posts[i])
        {
            while (i > 0)
                curl_free(escaped_posts[--i]);
            return CURLE_OUT_OF_MEMORY;
        }
        post_len += strlen(postData[i][0]) + strlen(escaped_posts[i]) + 2; // "=&"
    }
    post_len++; // For trailing zero

    if (post_len > g_client.post_len)
    {
        result = post_resize(post_len);
        if (result != CURLE_OK)
        {
            for (i = 0; i < item_count; i++)
                curl_free(escaped_posts[i]);
            return result;
        }
    }

    g_client.post[0] = '\0';

    for (i = 0; i < item_count; i++)
    {
        local_res = snprintf(g_client.post + offset, g_client.post_len - offset, "%s=%s&", postData[i][0], escaped_posts[i]);

        curl_free(escaped_posts[i]);
        if (local_res < g_client.post_len - offset)
            offset += local_res;
        else
            result = CURLE_HTTP_POST_ERROR;
    }

    if (result == CURLE_OK)
        result = curl_easy_setopt(g_client.curl, CURLOPT_POSTFIELDS, g_client.post);

    return result;
}

long httpq_set_key_http_post(const char *postData[][3])
{
    long result = CURLE_OK;
    int i = 0;

    if (!postData)
        return CURLE_BAD_FUNCTION_ARGUMENT;

    if (g_client.mimepost)
    {
        curl_easy_setopt(g_client.curl, CURLOPT_MIMEPOST, NULL);
        curl_mime_free(g_client.mimepost);
        g_client.mimepost = NULL;
    }

    g_client.mimepost = curl_mime_init(g_client.curl);
    if (!g_client.mimepost)
        return CURLE_OUT_OF_MEMORY;

    while (result == CURLE_OK && postData[i][0])
    {
        curl_mimepart *part;

        if (postData[i][2] &&
            (!postData[i][1] || postData[i][1][0] == '\0'))
        {
            i++;
            continue;
        }
        if (!postData[i][1])
        {
            result = CURLE_BAD_FUNCTION_ARGUMENT;
            break;
        }

        part = curl_mime_addpart(g_client.mimepost);
        if (!part)
        {
            result = CURLE_OUT_OF_MEMORY;
            break;
        }
        result = curl_mime_name(part, postData[i][0]);
        if (result == CURLE_OK && postData[i][2])
            result = curl_mime_filedata(part, postData[i][1]);
        else if (result == CURLE_OK)
            result = curl_mime_data(part, postData[i][1], CURL_ZERO_TERMINATED);
        i++;
    }

    if (result == CURLE_OK)
        result = curl_easy_setopt(g_client.curl, CURLOPT_MIMEPOST, g_client.mimepost);
    if (result != CURLE_OK)
    {
        curl_mime_free(g_client.mimepost);
        g_client.mimepost = NULL;
    }

    return result;
}

long httpq_set_headers(const char *headerData[])
{
    long result = CURLE_OK;
    int i = 0;

    if (!headerData)
        return CURLE_BAD_FUNCTION_ARGUMENT;

    if (g_client.headers)
    {
        curl_easy_setopt(g_client.curl, CURLOPT_HTTPHEADER, NULL);
        curl_slist_free_all(g_client.headers);
        g_client.headers = NULL;
    }

    while (headerData[i])
    {
        struct curl_slist *new_headers = curl_slist_append(g_client.headers, headerData[i]);

        if (!new_headers)
        {
            curl_slist_free_all(g_client.headers);
            g_client.headers = NULL;
            return CURLE_OUT_OF_MEMORY;
        }
        g_client.headers = new_headers;
        i++;
    }
    result = curl_easy_setopt(g_client.curl, CURLOPT_HTTPHEADER, g_client.headers);

    return result;
}

long httpq_set_user_name(const char *userName)
{
    return curl_easy_setopt(g_client.curl, CURLOPT_USERNAME, userName);
}

long httpq_set_user_pwd(const char *userPwd)
{
    return curl_easy_setopt(g_client.curl, CURLOPT_PASSWORD, userPwd);
}

long httpq_set_limit_resp(long respLimit)
{
    if (respLimit < 0)
        return CURLE_BAD_FUNCTION_ARGUMENT;

    g_client.resp_limit = respLimit;
    return CURLE_OK;
}

long httpq_set_max_time(long maxTime)
{
    g_client.maxtime_limit = maxTime;
    return CURLE_OK;
}

long httpq_set_retry(enum httpq_retry_policy retryPolicy)
{
    g_client.retry_policy = retryPolicy;
    return CURLE_OK;
}

char* httpq_request_post(long* errorCode, long* httpCode)
{
    long result = CURLE_FAILED_INIT;
    size_t initial_size = RESP_DEFAULT_LEN;
    char* resp;
    struct curl_callback_data response;

    if (!errorCode || !httpCode)
    {
        if (errorCode)
            *errorCode = CURLE_BAD_FUNCTION_ARGUMENT;
        if (httpCode)
            *httpCode = 0;
        return NULL;
    }

    if ((size_t)g_client.resp_limit + 1 < initial_size)
        initial_size = (size_t)g_client.resp_limit + 1;

    resp = malloc(initial_size);
    if (!resp)
    {
        *errorCode = CURLE_OUT_OF_MEMORY;
        *httpCode = 0;
        return NULL;
    }
    resp[0] = '\0';
    response = (struct curl_callback_data) {
        resp, 0, initial_size, (size_t)g_client.resp_limit
    };

    result = curl_easy_setopt(g_client.curl, CURLOPT_NOSIGNAL, 1L);
    /* CURLOPT_POST would replace libcurl's multipart request mode. */
    if (result == CURLE_OK && !g_client.mimepost)
        result = curl_easy_setopt(g_client.curl, CURLOPT_POST, 1L);

    if (result == CURLE_OK)
        result = curl_easy_setopt(g_client.curl, CURLOPT_TIMEOUT, g_client.maxtime_limit);

    if (result == CURLE_OK)
        result = curl_easy_setopt(g_client.curl, CURLOPT_WRITEFUNCTION, write_callback);

    if (result == CURLE_OK)
        result = curl_easy_setopt(g_client.curl, CURLOPT_WRITEDATA, &response);

    if (result == CURLE_OK)
        result = curl_easy_perform(g_client.curl);

    if (result == CURLE_OPERATION_TIMEDOUT &&
        g_client.retry_policy == rpRetryOnTimeoutError)
    {
        result = curl_easy_setopt(g_client.curl, CURLOPT_FRESH_CONNECT, 1L);
        if (result == CURLE_OK)
        {
            response.len = 0;
            response.buffer[0] = '\0';
            result = curl_easy_perform(g_client.curl);
        }
        curl_easy_setopt(g_client.curl, CURLOPT_FRESH_CONNECT, 0L);
    }

    if (result == CURLE_OK)
        result = curl_easy_getinfo(g_client.curl, CURLINFO_RESPONSE_CODE, httpCode);

    if (result != CURLE_OK)
    {
        free(response.buffer);
        response.buffer = NULL;
        *httpCode = 0;
    }
    *errorCode = result;

    curl_easy_setopt(g_client.curl, CURLOPT_MIMEPOST, NULL);
    curl_easy_setopt(g_client.curl, CURLOPT_HTTPHEADER, NULL);
    curl_mime_free(g_client.mimepost);
    curl_slist_free_all(g_client.headers);
    g_client.headers = NULL;
    g_client.mimepost = NULL;

    return response.buffer;
}

void httpq_reset(void)
{
    httpq_set_limit_resp(RESP_DEFAULT_LIMIT);
    httpq_set_max_time(REQ_DEFAULT_MAXTIME);
    httpq_set_retry(rpNoRetry);
    curl_easy_reset(g_client.curl);
    curl_mime_free(g_client.mimepost);
    curl_slist_free_all(g_client.headers);
    g_client.mimepost = NULL;
    g_client.headers = NULL;
}

const char* httpq_error(long errorCode)
{
    return curl_easy_strerror(errorCode);
}
