#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

#include "httpq.h"

#define RESP_DEFAULT_LEN  (8 * 1024)
#define RESP_DEFAULT_LIMIT (4 * 1024 * 1024)
#define REQ_DEFAULT_MAXTIME 20

static CURL *g_curl;
static char *g_post;
static struct curl_httppost *g_httppost = NULL;
static long g_post_len;
static struct curl_slist *g_headers = NULL;
static long g_resp_limit = RESP_DEFAULT_LIMIT;
static long g_maxtime_limit = REQ_DEFAULT_MAXTIME;

struct curl_callback_data
{
    char* buffer;
    long len;
    long allocated;
};

static void cleanup()
{
    if (g_curl)
    {
        curl_easy_cleanup(g_curl);
        curl_formfree(g_httppost);
        curl_slist_free_all(g_headers);
        g_curl = NULL;
        g_httppost = NULL;
        g_headers = NULL;

        if (g_post && g_post_len > 0)
        {
            free(g_post);
            g_post = NULL;
            g_post_len = 0;
        }
    }
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    long data_size = size * nmemb;
    size_t result = data_size;
    struct curl_callback_data *data = (struct curl_callback_data*)userp;

    // printf("data->allocated[%ld] data->len[%ld] data_size[%ld]\n", data->allocated, data->len, data_size);

    if (data->allocated - data->len < data_size + 1)
    {
        if (data->allocated < g_resp_limit)
        {
            data->buffer = realloc(data->buffer, data->allocated + (data_size + 1) * 2);
            data->allocated += (data_size + 1) * 2;
        } else
            result = 0;
    }

    // Copy received data plus trailing null
    snprintf(data->buffer + data->len, result + 1, "%s", (char*)contents);

    data->len += result;
    // printf("data->allocated[%ld] data->len[%ld] data_size[%ld] result[%ld]\n", data->allocated, data->len, data_size, result);

    return result;
}

long httpq_init()
{
    long result = CURLE_FAILED_INIT;

    if (!g_curl)
    {
        g_curl = curl_easy_init();
        if (g_curl)
            atexit(cleanup);
    }

    if (g_curl)
        result = CURLE_OK;

    return result;
}

long httpq_set_url(const char *aURL)
{
    if (!aURL)
        return CURLE_BAD_FUNCTION_ARGUMENT;
    else
        return curl_easy_setopt(g_curl, CURLOPT_URL, aURL);
}

long httpq_set_post(const char *postData[][2])
{
    long local_res, offset = 0;
    long result = CURLE_OK;
    char** escaped_posts = NULL;
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

    escaped_posts = malloc(sizeof(char*) * item_count);
    for (i = 0; i < item_count; i++)
    {
        escaped_posts[i] = curl_easy_escape(g_curl, postData[i][1], 0);
        //printf("item[%ld] - [%ld]=[%ld]\n", i, strlen(postData[i][0]), strlen(escaped_posts[i]));
        post_len += strlen(postData[i][0]) + strlen(escaped_posts[i]) + 2; // "=&"
    }
    post_len++; // For trailing zero
    //printf("post_len[%ld]\n", post_len);

    if (post_len > g_post_len)
    {
        if (g_post && g_post_len > 0)
            free(g_post);
        g_post = malloc(post_len);
        g_post_len = post_len;
    }

    for (i = 0; i < item_count; i++)
    {
        local_res = snprintf(g_post + offset, g_post_len - offset, "%s=%s&", postData[i][0], escaped_posts[i]);
        curl_free(escaped_posts[i]);
        if ((local_res >= 0) && (local_res < g_post_len - offset))
            offset += local_res;
        else
            result = CURLE_HTTP_POST_ERROR;
    }
    //printf("[%s][%ld][%ld]\n", g_post, strlen(g_post), result);

    free(escaped_posts);

    if (result == CURLE_OK)
        result = curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, g_post);

    return result;
}

long httpq_set_httppost(const char *postData[][3])
{
    long result = 0;
    struct curl_httppost *lastptr = NULL;
    int i = 0;

    if (!postData)
        return CURLE_BAD_FUNCTION_ARGUMENT;

    if (g_httppost)
    {
        curl_formfree(g_httppost);
        g_httppost = NULL;
    }

    while (result == 0 && postData[i][0])
    {
        if (postData[i][2] == 0)
        {
            result = curl_formadd(&g_httppost, &lastptr,
                CURLFORM_COPYNAME, postData[i][0],
                CURLFORM_COPYCONTENTS, postData[i][1],
                CURLFORM_END);
        } else
        {
            result = curl_formadd(&g_httppost, &lastptr,
                CURLFORM_COPYNAME, postData[i][0],
                CURLFORM_FILE, postData[i][1],
                CURLFORM_END);
        }
        i++;
    }

    if (result == 0)
        result = curl_easy_setopt(g_curl, CURLOPT_HTTPPOST, g_httppost);
    else
        result = CURLE_BAD_FUNCTION_ARGUMENT;

    return result;
}

long httpq_set_headers(const char *headerData[])
{
    long result = CURLE_OK;
    int i = 0;

    if (!headerData)
        return CURLE_BAD_FUNCTION_ARGUMENT;

    if (g_headers)
    {
        curl_slist_free_all(g_headers);
        g_headers = NULL;
    }

    while (headerData[i])
    {
        g_headers = curl_slist_append(g_headers, headerData[i]);
        i++;
    }
    result = curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, g_headers);

    return result;
}

long httpq_set_username(const char *userName)
{
    return curl_easy_setopt(g_curl, CURLOPT_USERNAME, userName);
}

long httpq_set_userpwd(const char *userPwd)
{
    return curl_easy_setopt(g_curl, CURLOPT_USERPWD, userPwd);
}

long httpq_set_limit_resp(long respLimit)
{
    g_resp_limit = respLimit;
    return CURLE_OK;
}

long httpq_set_maxtime(long maxTime)
{
    g_maxtime_limit = maxTime;
    return CURLE_OK;
}

char* httpq_request_post(long* errorCode, long* httpCode)
{
    long result = CURLE_FAILED_INIT;
    char* resp = malloc(RESP_DEFAULT_LEN);
    struct curl_callback_data response = { resp, 0, RESP_DEFAULT_LEN };

    result = curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, g_maxtime_limit);

    if (result == CURLE_OK)
        result = curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);

    if (result == CURLE_OK)
        result = curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);

    if (result == CURLE_OK)
        result = curl_easy_perform(g_curl);

    if (result == CURLE_OPERATION_TIMEDOUT)
    {
        curl_easy_setopt(g_curl, CURLOPT_FRESH_CONNECT, 1L);
        result = curl_easy_perform(g_curl);
    }

    if (result == CURLE_OK)
        result = curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, httpCode);

    if (result != CURLE_OK)
    {
        free(response.buffer);
        response.buffer = NULL;
        *httpCode = 0;
    }
    *errorCode = result;

    curl_formfree(g_httppost);
    curl_slist_free_all(g_headers);
    g_headers = NULL;
    g_httppost = NULL;

    return response.buffer;
}

void httpq_reset()
{
    httpq_set_limit_resp(RESP_DEFAULT_LIMIT);
    httpq_set_maxtime(REQ_DEFAULT_MAXTIME);
    curl_easy_reset(g_curl);
}

const char* httpq_error(long errorCode)
{
    return curl_easy_strerror(errorCode);
}
