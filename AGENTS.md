# Repository guidance

## Project purpose and scope

`libhttpq` is a small C11 wrapper around libcurl for synchronous HTTP/HTTPS
requests. It supports raw and URL-encoded POST bodies, headers, Basic
authentication, response limits, timeout policy, and MIME multipart forms.
Keep changes focused and easy to review; this library is intentionally thin.

Always inspect `git status` and the relevant diff before editing. Preserve all
existing worktree changes unless the user explicitly asks to discard them.

## Required toolchain

- CMake 4.2 or newer.
- A C11 compiler with `_Thread_local` support.
- libcurl 8.11.1 or newer, built with `CURL_VERSION_THREADSAFE`.
- POSIX threads. CMake links `Threads::Threads`.
- Python 3, `pkg-config`, and compiler sanitizer support for the regression
  suite.

The current CMake options are GNU-oriented (`-flto`, `-fuse-ld=gold`, and
`-ffast-math`). Treat compiler/linker portability changes as separate focused
work and verify them with both shared and static builds.

Keep dependency discovery through `find_package(CURL 8.11.1 REQUIRED)` and link
`CURL::libcurl`; do not revert to a literal `curl` library name. Both shared and
static `httpq` builds must continue to work.

## Build and verification

Prefer out-of-source or temporary builds so generated files do not pollute the
repository:

```sh
cmake -S . -B build
cmake --build build --parallel

cmake -S . -B build-static -DBUILD_SHARED_LIBS=OFF
cmake --build build-static --parallel
```

Run the focused suite after every behavior, memory, retry, lifecycle, or
threading change:

```sh
python3 tests/run_tests.py
SANITIZERS=thread python3 tests/run_tests.py
```

The default test run uses AddressSanitizer and UndefinedBehaviorSanitizer. The
suite compiles into a temporary directory and talks only to a local ephemeral
HTTP server. `tests/test_alloc.c` intentionally includes `httpq.c` and wraps
`malloc`/`realloc` to exercise otherwise difficult allocation failures.

For a full handoff, also run both shared and static CMake builds, optionally
install them to temporary prefixes, run GCC `-fanalyzer` when available, and
finish with `git diff --check`. Do not suppress new compiler warnings.

Do not use `httpq-sample` as a routine test: it sends a real Telegram request
and requires live credentials. Never put bot keys or other secrets in the
repository or command output. `clean-dir` is intended only for an in-source
CMake build; prefer deleting a known temporary out-of-source build directory.

## Lifecycle and threading model

- `httpq_init()` performs process-global libcurl initialization exactly once
  with `pthread_once` before creating the calling thread's easy handle.
- Global initialization uses `CURL_GLOBAL_DEFAULT`, verifies the runtime
  `CURL_VERSION_THREADSAFE` feature, and registers one matching
  `curl_global_cleanup()` at process exit.
- Mutable request state and the easy handle are `_Thread_local`. There is one
  implicit logical client per thread; the API does not expose multiple client
  handles within one thread.
- Every thread that successfully calls `httpq_init()` should call
  `httpq_cleanup()` before the thread exits. Never share an easy handle or the
  implicit client state between threads.
- Do not move global cleanup into `httpq_cleanup()`: libcurl global state is
  process-wide and must outlive every per-thread client.
- Preserve the simultaneous first-ever initialization regression test. It is
  specifically intended to detect initialization publication races.

## API behavior and ownership

- Public functions return libcurl `CURLcode` values through the historical
  `long` API; success is `HTTPQ_OK`/`CURLE_OK`.
- Call `httpq_init()` before any setter or request function.
- `httpq_request_post()` expects valid `errorCode` and `httpCode` output
  pointers. On success it returns a heap allocation owned by the caller, who
  must release it with `free()`.
- URL, credentials, raw/encoded POST storage, limits, timeout, and retry policy
  remain part of the current thread's client state until replaced, reset, or
  cleaned up.
- Header lists and multipart forms are detached and freed after each request;
  callers must set them again for a later request.
- `httpq_reset()` resets libcurl options and restores the 4 MiB response limit,
  20-second timeout, and `rpNoRetry` policy.
- Automatic POST retry is deliberately disabled. `rpRetryOnTimeoutError` is an
  explicit opt-in because repeating a non-idempotent POST can duplicate an
  operation.

## Safety invariants

Do not regress these properties:

- The write callback copies exactly `size * nmemb` bytes with `memcpy`, handles
  embedded NUL bytes, and appends one terminator.
- Response limits apply to received body bytes (`len + incoming`), including
  limits smaller than the initial buffer and exact-limit responses.
- Initial allocation and every growth allocation are checked. Use a temporary
  pointer for `realloc` so failure preserves the original allocation.
- A zero-length successful response is an initialized empty C string.
- Encoded POST storage is initialized even for a sentinel-only item array.
- Headers and multipart forms must be detached from the easy handle before
  their backing allocations are freed, both when replacing them and after a
  request.
- Before an explicit retry, discard all partial response bytes and restore an
  empty terminated buffer.
- `httpq_set_user_pwd()` uses `CURLOPT_PASSWORD`; username and password are
  separate libcurl options.
- Request state must stay isolated across concurrent threads.

## Known remaining work

The following review findings are not resolved by the current hardening and
should not be mistaken for regressions introduced by new changes:

- `sample.c` constructs `post_data` with `argv[2]` before checking `argc`, which
  is undefined behavior when arguments are missing.

There is currently no CTest integration; `tests/run_tests.py` is the canonical
regression entry point.

## Code style

- Match the existing C style: four spaces, Allman braces, short functions, and
  minimal comments that explain ownership or non-obvious safety constraints.
- Prefer small, local patches over broad formatting or unrelated cleanup.
- Preserve public API compatibility unless the user explicitly authorizes an
  API redesign.
- Add a focused regression test for every bug fix, and verify the actual
  failure mode rather than only compiling the code.
