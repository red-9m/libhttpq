#include <stdlib.h>
#include <stdio.h>
#include <httpq.h>

#define URL_MAX_LEN 256

int main(int argc, char** argv)
{
    printf("libHTTPQ sample\n");

    if (argc != 3)
    {
        printf("Usage: %s <bot_key> <chat_id>\n", argv[0]);
        return 1;
    }

    char url[URL_MAX_LEN];

    const char *post_data[][2] = {
        {"parse_mode", "HTML"},
        {"chat_id", argv[2]},
        {"text", "<b>Hi!</b>\n<code>This is httpq sample message: 0123456789. Some special characters: [@][&][=][?]</code>"},
        {"dummy", "dummy"},
        {NULL, NULL}
    };

    const char *header_data[] = {
        "Accept: application/json",
        "Accept-Language: en_US",
        NULL
    };

    snprintf(url, URL_MAX_LEN, "https://api.telegram.org/bot%s/sendMessage", argv[1]);

    httpq_init();
    httpq_set_url(url);
    httpq_set_post(post_data);
    httpq_set_headers(header_data);

    long http, res;
    char *resp = httpq_request_post(&res, &http);
    if (resp)
    {
        printf("resp[%s]\n", resp);
        free(resp);
    }
    printf("libHTTPQ sample result[%ld] http code[%ld] error string[%s]\n", res, http, httpq_error(res));

    return 0;
}
