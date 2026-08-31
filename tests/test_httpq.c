#include <curl/curl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "httpq.h"

struct thread_data
{
    pthread_barrier_t *barrier;
    const char *url;
    const char *post;
    const char *expected;
    int failed;
};

static void fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

static void require_curl(long result, const char *message)
{
    if (result != CURLE_OK)
    {
        fprintf(stderr, "FAIL: %s: %s\n", message, curl_easy_strerror(result));
        exit(1);
    }
}

static void set_url(const char *base_url, const char *path)
{
    char url[512];

    if (snprintf(url, sizeof(url), "%s%s", base_url, path) >= (int)sizeof(url))
        fail("test URL is too long");
    require_curl(httpq_set_url(url), "set URL");
}

static char *perform(long *error_code, long *http_code)
{
    char *response = httpq_request_post(error_code, http_code);

    if (*error_code == CURLE_OK && *http_code != 200)
        fail("unexpected HTTP status");
    return response;
}

static void *thread_request(void *userp)
{
    struct thread_data *data = (struct thread_data*)userp;
    char *response;
    long error_code;
    long http_code;

    pthread_barrier_wait(data->barrier);
    if (httpq_init() != CURLE_OK ||
        httpq_set_url(data->url) != CURLE_OK ||
        httpq_set_post(data->post) != CURLE_OK)
    {
        data->failed = 1;
        return NULL;
    }

    pthread_barrier_wait(data->barrier);
    response = httpq_request_post(&error_code, &http_code);
    if (error_code != CURLE_OK || http_code != 200 || !response ||
        strcmp(response, data->expected) != 0)
        data->failed = 1;

    free(response);
    httpq_cleanup();
    return NULL;
}

static void test_thread_isolation(const char *base_url)
{
    pthread_barrier_t barrier;
    pthread_t threads[2];
    char urls[2][512];
    struct thread_data data[2] = {
        { &barrier, urls[0], "alpha", "/thread-a:alpha", 0 },
        { &barrier, urls[1], "beta", "/thread-b:beta", 0 }
    };

    snprintf(urls[0], sizeof(urls[0]), "%s/thread-a", base_url);
    snprintf(urls[1], sizeof(urls[1]), "%s/thread-b", base_url);
    if (pthread_barrier_init(&barrier, NULL, 2) != 0)
        fail("initialize thread barrier");
    if (pthread_create(&threads[0], NULL, thread_request, &data[0]) != 0 ||
        pthread_create(&threads[1], NULL, thread_request, &data[1]) != 0)
        fail("create request threads");

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_barrier_destroy(&barrier);
    if (data[0].failed || data[1].failed)
        fail("request state crossed thread boundaries");
}

int main(int argc, char **argv)
{
    const char *headers[] = { "X-Httpq-Test: present", NULL };
    const char *form[][3] = {
        { "field", "value", NULL },
        { NULL, NULL, NULL }
    };
    const char *file_form[][3] = {
        { "sender", "John", NULL },
        { "pic", __FILE__, "file" },
        { NULL, NULL, NULL }
    };
    char *response;
    long error_code;
    long http_code;

    if (argc != 2)
        fail("expected the test server base URL");
    test_thread_isolation(argv[1]);
    require_curl(httpq_init(), "initialize httpq");

    if (httpq_request_post(NULL, NULL) != NULL)
        fail("NULL output pointers were accepted");
    http_code = 123;
    if (httpq_request_post(NULL, &http_code) != NULL || http_code != 0)
        fail("NULL error output pointer was accepted");
    error_code = CURLE_OK;
    if (httpq_request_post(&error_code, NULL) != NULL ||
        error_code != CURLE_BAD_FUNCTION_ARGUMENT)
        fail("NULL HTTP output pointer was accepted");

    httpq_reset();
    set_url(argv[1], "/empty-post");
    response = perform(&error_code, &http_code);
    if (!response || strcmp(response, "ok") != 0)
        fail("body-less POST was sent with the wrong method");
    free(response);

    require_curl(httpq_set_post("operation=yes"), "set POST data before reset");
    httpq_reset();
    set_url(argv[1], "/empty-post");
    response = perform(&error_code, &http_code);
    if (!response || strcmp(response, "ok") != 0)
        fail("reset body-less POST was sent with the wrong method or body");
    free(response);

    require_curl(httpq_set_key_http_post(form), "set multipart data before reset");
    httpq_reset();
    set_url(argv[1], "/empty-post");
    response = perform(&error_code, &http_code);
    if (!response || strcmp(response, "ok") != 0)
        fail("reset retained multipart request state");
    free(response);

    httpq_reset();
    set_url(argv[1], "/binary");
    require_curl(httpq_set_post(""), "set binary request POST data");
    response = perform(&error_code, &http_code);
    if (error_code != CURLE_OK || !response ||
        memcmp(response, "A\0B", 3) != 0 || response[3] != '\0')
        fail("binary response was not copied byte-for-byte");
    free(response);

    httpq_reset();
    set_url(argv[1], "/header");
    require_curl(httpq_set_post(""), "set header request POST data");
    require_curl(httpq_set_headers(headers), "set headers");
    response = perform(&error_code, &http_code);
    if (!response || strcmp(response, "present") != 0)
        fail("configured header was not sent");
    free(response);
    response = perform(&error_code, &http_code);
    if (!response || response[0] != '\0')
        fail("freed headers remained attached to the easy handle");
    free(response);

    httpq_reset();
    set_url(argv[1], "/multipart-file");
    require_curl(httpq_set_key_http_post(file_form), "set multipart file form");
    response = perform(&error_code, &http_code);
    if (!response || strcmp(response, "ok") != 0)
        fail("multipart file was not uploaded");
    free(response);

    httpq_reset();
    set_url(argv[1], "/multipart");
    require_curl(httpq_set_key_http_post(form), "set multipart form");
    response = perform(&error_code, &http_code);
    free(response);
    response = perform(&error_code, &http_code);
    if (!response || strcmp(response, "ok") != 0)
        fail("freed multipart form remained attached to the easy handle");
    free(response);

    httpq_reset();
    set_url(argv[1], "/large");
    require_curl(httpq_set_post(""), "set limited request POST data");
    require_curl(httpq_set_limit_resp(10), "set small response limit");
    response = httpq_request_post(&error_code, &http_code);
    if (response || error_code != CURLE_WRITE_ERROR || http_code != 0)
        fail("small response limit was not enforced");
    require_curl(httpq_set_limit_resp(1000), "set exact response limit");
    response = perform(&error_code, &http_code);
    if (!response || strlen(response) != 1000)
        fail("exact response limit rejected a valid response");
    free(response);
    if (httpq_set_limit_resp(-1) != CURLE_BAD_FUNCTION_ARGUMENT)
        fail("negative response limit was accepted");

    httpq_reset();
    set_url(argv[1], "/auth");
    require_curl(httpq_set_post(""), "set auth request POST data");
    require_curl(httpq_set_user_name("alice"), "set username");
    require_curl(httpq_set_user_pwd("secret"), "set password");
    response = perform(&error_code, &http_code);
    if (!response || strcmp(response, "Basic YWxpY2U6c2VjcmV0") != 0)
        fail("username and password produced incorrect Basic credentials");
    free(response);

    httpq_reset();
    set_url(argv[1], "/timeout-default");
    require_curl(httpq_set_post("operation"), "set timeout request POST data");
    require_curl(httpq_set_max_time(1), "set timeout");
    response = httpq_request_post(&error_code, &http_code);
    if (response || error_code != CURLE_OPERATION_TIMEDOUT)
        fail("timed-out POST did not fail without a default retry");

    httpq_reset();
    set_url(argv[1], "/timeout-partial");
    require_curl(httpq_set_post("operation"), "set retry request POST data");
    require_curl(httpq_set_max_time(1), "set retry timeout");
    require_curl(httpq_set_retry(rpRetryOnTimeoutError), "enable timeout retry");
    response = perform(&error_code, &http_code);
    if (!response || strcmp(response, "SECOND") != 0)
        fail("retry response contained bytes from the failed attempt");
    free(response);

    httpq_cleanup();
    puts("httpq integration tests passed");
    return 0;
}
