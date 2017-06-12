#include <stdlib.h>
#include <curl/curl.h>

#include "httpq.h"

static CURL *g_curl = NULL;
static char g_error[64];

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
    }
}

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t result = size * nmemb;
    struct curl_callback_data *data = (struct curl_callback_data*)userp;

    printf("1. size[%ld] nmemb[%ld] curr_len[%ld] max_len[%ld]\n", size, nmemb, data->curr_len, data->max_len);
    if ((data->buffer) && (data->curr_len + size * nmemb < data->max_len))
    {
        printf("C. Do copy\n");
        // Copy received data plus trailing null
        snprintf(data->buffer + data->curr_len, size * nmemb + 1, "%s", (char *)contents);
        data->curr_len += size * nmemb;
    } else
        result = 0;

    return result;
}

long httpq_post(const char *aURL, const char *aPost, char *aResp, unsigned long respLen)
{
    long http_code;
    CURL* curl;
    long result = CURLE_FAILED_INIT;
    struct curl_callback_data response = { aResp, respLen, 0 };

    if (aResp)
        *aResp = '\0';

    if (!g_curl)
    {
        g_curl = curl_easy_init();
        atexit(cleanup);
    }
    curl = g_curl;

    if (curl)
    {
        result = curl_easy_setopt(curl, CURLOPT_URL, aURL);
        if (result == CURLE_OK)
            result = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, aPost);
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
        snprintf(g_error, 64, "HTTP OK: %ld", aError);
    else if ((aError >= 400) && (aError < 600))
        snprintf(g_error, 64, "HTTP ERROR: %ld", aError);
    else
        result = curl_easy_strerror(aError);

    return result;
}
