#include <stdio.h>
#include <stdlib.h>
#include <httpq.h>

#define URL_MAX_LEN 256

int main(int argc, char** argv)
{
    long http, res;
    char url[URL_MAX_LEN];
    char *response;
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

    printf("libHTTPQ sample\n");

    if (argc != 3)
    {
        printf("Usage: %s <bot_key> <chat_id>\n", argv[0]);
        return 1;
    }

    snprintf(url, URL_MAX_LEN, "https://api.telegram.org/bot%s/sendMessage", argv[1]);

    httpq_init();
    httpq_set_url(url);
    httpq_set_post(post_data);
    httpq_set_headers(header_data);

    response = httpq_request_post(&res, &http);
    if (res == 0)
    {
        printf("Response[%s]\n", response);
        free(response);
    }
    printf("libHTTPQ sample result[%ld] http code[%ld] error string[%s]\n", res, http, httpq_error(res));

    return 0;
}
