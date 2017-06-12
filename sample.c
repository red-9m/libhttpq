#include <stdlib.h>
#include <stdio.h>
#include <httpq.h>

#define URL_MAX_LEN 1024
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

    snprintf(url, URL_MAX_LEN, "https://api.telegram.org/bot%s/sendMessage", argv[1]);
    char_p post_data[3][2] = {
        {"parse_mode", "HTML"},
        {"chat_id", argv[2]},
        {"text", "<b>Header</b>\n<code>This is httpq sample message with special http characters: [&][=][?]</code>"}};

    long res = httpq_post(url, post_data, 3, resp, RESP_MAX_LEN);

    printf("HTTPq result[%ld] [%s]\n", res, httpq_error(res));

    return 0;
}
