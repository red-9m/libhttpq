#include <stdio.h>
#include <stdlib.h>
#include <httpq.h>

#define URL_MAX_LEN 256

int main(int argc, char** argv)
{
    long http = 0;
    long res;
    int url_len;
    char url[URL_MAX_LEN];
    char *response = NULL;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <bot_key> <chat_id>\n", argv[0]);
        return EXIT_FAILURE;
    }

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

    url_len = snprintf(url, sizeof(url),
        "https://api.telegram.org/bot%s/sendMessage", argv[1]);
    if (url_len < 0 || (size_t)url_len >= sizeof(url))
    {
        fprintf(stderr, "Bot key is too long\n");
        return EXIT_FAILURE;
    }

    res = httpq_init();
    if (res != HTTPQ_OK)
    {
        fprintf(stderr, "Initialization failed: %s\n", httpq_error(res));
        return EXIT_FAILURE;
    }

    res = httpq_set_url(url);
    if (res == HTTPQ_OK)
        res = httpq_set_key_post(post_data);
    if (res == HTTPQ_OK)
        res = httpq_set_headers(header_data);
    if (res == HTTPQ_OK)
        response = httpq_request_post(&res, &http);

    if (response != NULL)
    {
        printf("Response[%s]\n", response);
        free(response);
    }
    printf("libHTTPQ sample result[%ld] http code[%ld] error string[%s]\n", res, http, httpq_error(res));

    httpq_cleanup();
    return res == HTTPQ_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
