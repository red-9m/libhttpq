#include <stdlib.h>
#include <curl/curl.h>

#include "httpq.h"

#define ERR_MAX_LEN 64
#define POST_MAX_LEN (64 * 1024)

static CURL *g_curl = NULL;
static char g_error[ERR_MAX_LEN];
static char *g_post = NULL;

struct curl_callback_data
{
    char* buffer;
    unsigned long max_len;
    unsigned long curr_len;
};

static void cleanup()
{
    if (g_curl)
    {
        curl_easy_cleanup(g_curl);
        g_curl = NULL;
        free(g_post);
        g_post = NULL;
    }
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t result = size * nmemb;
    struct curl_callback_data *data = (struct curl_callback_data*)userp;

    if ((data->buffer) && (data->curr_len + size * nmemb < data->max_len))
    {
        // Copy received data plus trailing null
        snprintf(data->buffer + data->curr_len, size * nmemb + 1, "%s", (char *)contents);
        data->curr_len += size * nmemb;
    } else
        result = 0;

    return result;
}

long httpq_post(const char *aURL, const char* postData[][2], long postCount, char *aResp, long respLen)
{
    long http_code, i, local_res, offset = 0;
    char* escaped_post;
    CURL* curl;

    long result = CURLE_FAILED_INIT;
    struct curl_callback_data response = { aResp, respLen, 0 };

    if (aResp)
        *aResp = '\0';

    if (!g_curl)
    {
        g_curl = curl_easy_init();
        if ((!g_post) && (g_curl))
            g_post = malloc(POST_MAX_LEN);

        atexit(cleanup);
    }
    curl = g_curl;

    if (curl)
    {
        result = curl_easy_setopt(curl, CURLOPT_URL, aURL);
        if (result == CURLE_OK)
        {
            for (i = 0; i < postCount; i++)
            {
                escaped_post = curl_easy_escape(curl, postData[i][1], 0);
                local_res = snprintf(g_post + offset, POST_MAX_LEN - offset, "%s=%s&", postData[i][0], escaped_post);
                curl_free(escaped_post);

                if ((local_res >= 0) && (local_res < POST_MAX_LEN - offset))
                {
                    offset += local_res;
                }
                else
                {
                    result = CURLE_HTTP_POST_ERROR;
                    break;
                }
            }

            if (result == CURLE_OK)
                result = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, g_post);
        }

        if (result == CURLE_OK)
            result = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        if (result == CURLE_OK)
            result = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        if (result == CURLE_OK)
            result = curl_easy_perform(curl);

        if (result == CURLE_OK)
        {
            result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (result == CURLE_OK)
                result = http_code;
        }
    } 

    return result;
}

const char* httpq_error(long aError)
{
    const char* result = g_error;
    if ((aError >= 200) && (aError < 400))
        snprintf(g_error, ERR_MAX_LEN, "HTTP OK: %ld", aError);
    else if ((aError >= 400) && (aError < 600))
        snprintf(g_error, ERR_MAX_LEN, "HTTP ERROR: %ld", aError);
    else
        result = curl_easy_strerror(aError);

    return result;
}
