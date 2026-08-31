# libhttpq 1.0.0

`libhttpq` is a small C11 wrapper around libcurl for making synchronous
HTTP/HTTPS POST requests. It is intended for small C programs that need a
simple, blocking request API without configuring libcurl directly.

The library supports raw and URL-encoded POST bodies, custom headers, Basic
authentication, response-size and timeout limits, optional timeout retries,
and MIME multipart forms (including file uploads). It is deliberately a thin
wrapper, not a general-purpose or asynchronous HTTP client.

The CMake target for consumers is `libhttpq::httpq` (the underlying build
target is `httpq`). Each thread gets its own implicit client state: call
`httpq_init()` before using the API and `httpq_cleanup()` before that thread
exits.

## Requirements

- CMake 4.2 or newer
- A C11 compiler with `_Thread_local` support
- libcurl 8.11.1 or newer, built with thread-safety support
- POSIX threads

## Build

Build the shared library (the default):

```sh
cmake -S . -B build
cmake --build build --parallel
```

For a static library, add `-DBUILD_SHARED_LIBS=OFF` when configuring:

```sh
cmake -S . -B build-static -DBUILD_SHARED_LIBS=OFF
cmake --build build-static --parallel
```

To install the library and `httpq.h`:

```sh
cmake --install build
```

An installed CMake package exports the target `libhttpq::httpq`:

```cmake
find_package(libhttpq 1 CONFIG REQUIRED)
target_link_libraries(my-program PRIVATE libhttpq::httpq)
```

## Minimal example

```c
#include <stdio.h>
#include <stdlib.h>
#include <httpq.h>

int main(void)
{
    long curl_code;
    long http_code;
    char *response;

    if (httpq_init() != HTTPQ_OK)
        return 1;

    if (httpq_set_url("https://example.com/api") != HTTPQ_OK ||
        httpq_set_post("message=hello") != HTTPQ_OK)
    {
        httpq_cleanup();
        return 1;
    }

    response = httpq_request_post(&curl_code, &http_code);
    if (response != NULL)
    {
        printf("HTTP %ld: %s\n", http_code, response);
        free(response);
    }
    else
    {
        fprintf(stderr, "Request failed: %s\n", httpq_error(curl_code));
    }

    httpq_cleanup();
    return curl_code == HTTPQ_OK ? 0 : 1;
}
```

The returned response belongs to the caller and must be released with
`free()`. Requests use a 4 MiB response limit, a 20-second timeout, and no
automatic retry by default. See [`httpq.h`](httpq.h) for the complete API and
configuration functions.

## Tests and sample

Run the local regression suite with:

```sh
python3 tests/run_tests.py
```

The `httpq-sample` executable is a Telegram-specific example that sends a real
message and requires a bot key and chat ID. It is not needed to use or test the
library.
