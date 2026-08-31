#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail_malloc;
static int fail_realloc;
static int atexit_call_count;
static void (*last_atexit_handler)(void);

void *__real_malloc(size_t size);
void *__real_realloc(void *pointer, size_t size);

void *__wrap_malloc(size_t size)
{
    if (fail_malloc)
        return NULL;
    return __real_malloc(size);
}

void *__wrap_realloc(void *pointer, size_t size)
{
    if (fail_realloc)
        return NULL;
    return __real_realloc(pointer, size);
}

static int test_atexit(void (*handler)(void))
{
    atexit_call_count++;
    last_atexit_handler = handler;
    return atexit(handler);
}

#define atexit test_atexit
#include "../httpq.c"
#undef atexit

static void fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

int main(void)
{
    const char chunk[] = { 'a', 'b', 'c' };
    char *original = __real_malloc(2);
    struct curl_callback_data data = { original, 1, 2, 100 };
    char *response;
    char *old_post;
    long old_post_len;
    long error_code;
    long http_code;

    if (!original)
        fail("test allocation");
    original[0] = 'X';
    original[1] = '\0';
    if (write_callback((void*)chunk, 1, sizeof(chunk), &data) != sizeof(chunk) ||
        data.len != 4 || memcmp(data.buffer, "Xabc", 5) != 0)
        fail("response buffer did not grow and preserve exact bytes");
    free(data.buffer);

    original = __real_malloc(2);
    if (!original)
        fail("second test allocation");
    original[0] = 'X';
    original[1] = '\0';
    data = (struct curl_callback_data) { original, 1, 2, 100 };
    fail_realloc = 1;
    if (write_callback((void*)chunk, 1, sizeof(chunk), &data) != 0 ||
        data.buffer != original || data.len != 1 || original[0] != 'X')
        fail("failed response realloc did not preserve the original buffer");
    fail_realloc = 0;
    free(original);

    if (httpq_init() != CURLE_OK)
        fail("initialize httpq");
    if (atexit_call_count != 1 || last_atexit_handler != curl_global_cleanup)
        fail("registered a per-thread exit handler");
    fail_malloc = 1;
    response = httpq_request_post(&error_code, &http_code);
    fail_malloc = 0;
    if (response || error_code != CURLE_OUT_OF_MEMORY || http_code != 0)
        fail("initial response allocation failure was not reported");

    if (httpq_set_post("x") != CURLE_OK)
        fail("set initial POST buffer");
    old_post = g_client.post;
    old_post_len = g_client.post_len;
    fail_realloc = 1;
    if (httpq_set_post("a larger POST buffer") != CURLE_OUT_OF_MEMORY ||
        g_client.post != old_post || g_client.post_len != old_post_len)
        fail("failed POST realloc did not preserve the original buffer");
    fail_realloc = 0;

    httpq_cleanup();
    if (httpq_init() != CURLE_OK)
        fail("reinitialize httpq");
    if (atexit_call_count != 1)
        fail("registered another exit handler after reinitialization");
    httpq_cleanup();
    puts("httpq allocation tests passed");
    return 0;
}
