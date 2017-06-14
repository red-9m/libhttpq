#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

#include "httpq.h"

#define RESP_DEFAULT_LEN  (8 * 1024)
#define RESP_DEFAULT_LIMIT (4 * 1024 * 1024)

static CURL *g_curl;
static char *g_post;
static long g_post_len;
static long g_resp_limit = RESP_DEFAULT_LIMIT;

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
        g_curl = NULL;

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
    CURL* curl;
    long i, local_res, offset = 0;
    long result = CURLE_OK;
    char** escaped_posts = NULL;
    long post_len = 0;
    long item_count = 0;

    if (!postData)
        return CURLE_BAD_FUNCTION_ARGUMENT;

    i = 0;
    while (postData[i][0])
    {
        i++;
    }
    item_count = i;

    escaped_posts = malloc(sizeof(char*) * item_count);
    for (i = 0; i < item_count; i++)
    {
        escaped_posts[i] = curl_easy_escape(curl, postData[i][1], 0);
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

long httpq_set_headers(const char *headerData[])
{
    long i;
    long result = CURLE_OK;
    long item_count = 0;
    struct curl_slist *chunk = NULL;

    if (!headerData)
        return CURLE_BAD_FUNCTION_ARGUMENT;

    i = 0;
    while (headerData[i])
    {
        i++;
    }
    item_count = i;

    for (i = 0; i < item_count; i++)
        chunk = curl_slist_append(chunk, headerData[i]);

    result = curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, chunk);

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

void httpq_set_limit_resp(long respLimit)
{
    g_resp_limit = respLimit;
}

char* httpq_request_post(long* errorCode, long* httpCode)
{
    long result = CURLE_FAILED_INIT;
    char* resp = malloc(RESP_DEFAULT_LEN);
    struct curl_callback_data response = { resp, 0, RESP_DEFAULT_LEN };

    result = curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);

    if (result == CURLE_OK)
        result = curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);

    if (result == CURLE_OK)
        result = curl_easy_perform(g_curl);

    if (result == CURLE_OK)
        result = curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, httpCode);

    if (result != CURLE_OK)
    {
        free(response.buffer);
        response.buffer = NULL;
        *httpCode = 0;
    }
    *errorCode = result;

    return response.buffer;
}

const char* httpq_error(long errorCode)
{
    return curl_easy_strerror(errorCode);
}
