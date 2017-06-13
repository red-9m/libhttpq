#include <stdlib.h>
#include <stdio.h>
#include <httpq.h>

#define URL_MAX_LEN 1024
#define POST_MAX_LEN 1024
#define RESP_MAX_LEN (16 * 1024)

int main(int argc, char** argv)
{
    printf("libHTTPQ sample\n");

    if (argc != 3)
    {
        printf("Usage: %s <bot_key> <chat_id>\n", argv[0]);
        return 1;
    }

    char url[URL_MAX_LEN];
    char resp[RESP_MAX_LEN];
    char post[POST_MAX_LEN];


    const char *post_data[3][2] = {
        {"parse_mode", "HTML"},
        {"chat_id", argv[2]},
        {"text", "<b>Header</b>\n<code>This is httpq sample message with special http characters: [&][=][?]</code>"}};

    const char *header_data[2] = {
        "Accept: application/json",
        "Accept-Language: en_US"};

    snprintf(url, URL_MAX_LEN, "https://api.telegram.org/bot%s/sendMessage", argv[1]);

    long res = httpq_prepare_post(post_data, 3, post, POST_MAX_LEN);
    if (res == 0)
        res = httpq_request_post(url, post, header_data, 2, resp, RESP_MAX_LEN);

    printf("HTTPQ result[%ld] [%s]\n", res, httpq_error(res));

    return 0;
}
