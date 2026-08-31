#!/usr/bin/env python3

import collections
import http.server
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import threading
import time


ROOT = Path(__file__).resolve().parent.parent


class TestHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    counters = collections.Counter()
    counter_lock = threading.Lock()

    def log_message(self, format, *args):
        pass

    def _send(self, body, status=200, headers=None):
        self.send_response(status)
        for name, value in headers or []:
            self.send_header(name, value)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        if body:
            self.wfile.write(body)
            self.wfile.flush()

    def _handle(self):
        length = int(self.headers.get("Content-Length", "0"))
        request_body = self.rfile.read(length)

        if self.path == "/binary":
            self._send(b"A\0B")
        elif self.path == "/empty-post":
            if self.command == "POST" and request_body == b"":
                self._send(b"ok")
            else:
                self._send(b"expected empty POST", status=400)
        elif self.path == "/header":
            self._send(self.headers.get("X-Httpq-Test", "").encode())
        elif self.path == "/multipart":
            self._send(b"ok")
        elif self.path == "/multipart-file":
            content_type = self.headers.get("Content-Type", "")
            if (
                content_type.startswith("multipart/form-data;")
                and b'name="sender"' in request_body
                and b"John" in request_body
                and b'name="pic"; filename="' in request_body
                and b'#include <curl/curl.h>' in request_body
            ):
                self._send(b"ok")
            else:
                self._send(b"invalid multipart file", status=400)
        elif self.path == "/large":
            self._send(b"A" * 1000)
        elif self.path == "/auth":
            authorization = self.headers.get("Authorization")
            if authorization is None:
                self._send(
                    b"",
                    status=401,
                    headers=[("WWW-Authenticate", 'Basic realm="httpq-test"')],
                )
            else:
                self._send(authorization.encode())
        elif self.path == "/timeout-default":
            with self.counter_lock:
                self.counters[self.path] += 1
            time.sleep(1.5)
            try:
                self._send(b"LATE")
            except (BrokenPipeError, ConnectionResetError):
                pass
        elif self.path == "/timeout-partial":
            with self.counter_lock:
                self.counters[self.path] += 1
                attempt = self.counters[self.path]
            if attempt == 1:
                self.send_response(200)
                self.send_header("Content-Length", "12")
                self.end_headers()
                self.wfile.write(b"FIRST-")
                self.wfile.flush()
                time.sleep(1.5)
                try:
                    self.wfile.write(b"FAILED")
                    self.wfile.flush()
                except (BrokenPipeError, ConnectionResetError):
                    pass
            else:
                self._send(b"SECOND")
        elif self.path in ("/thread-a", "/thread-b"):
            self._send(self.path.encode() + b":" + request_body)
        else:
            self._send(b"not found", status=404)

    do_GET = _handle
    do_POST = _handle


def compile_tests(build_dir):
    sanitizers = os.environ.get("SANITIZERS", "address,undefined")
    curl_flags = shlex.split(
        subprocess.check_output(
            ["pkg-config", "--cflags", "--libs", "libcurl"], text=True
        )
    )
    common = [
        os.environ.get("CC", "cc"),
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        f"-fsanitize={sanitizers}",
        "-fno-omit-frame-pointer",
        f"-I{ROOT}",
    ]
    integration = Path(build_dir) / "test_httpq"
    allocation = Path(build_dir) / "test_alloc"
    subprocess.run(
        common
        + ["-D_XOPEN_SOURCE=700", str(ROOT / "httpq.c"), str(ROOT / "tests/test_httpq.c")]
        + curl_flags
        + ["-pthread", "-o", str(integration)],
        check=True,
    )
    subprocess.run(
        common
        + [str(ROOT / "tests/test_alloc.c")]
        + curl_flags
        + ["-pthread", "-Wl,--wrap=malloc", "-Wl,--wrap=realloc", "-o", str(allocation)],
        check=True,
    )
    return integration, allocation


def main():
    with tempfile.TemporaryDirectory(prefix="libhttpq-tests-") as build_dir:
        integration, allocation = compile_tests(build_dir)
        environment = dict(os.environ)
        if "address" in os.environ.get("SANITIZERS", "address,undefined"):
            environment["ASAN_OPTIONS"] = "abort_on_error=1:detect_leaks=1"
        if "thread" in os.environ.get("SANITIZERS", ""):
            environment["TSAN_OPTIONS"] = "halt_on_error=1"
        subprocess.run([allocation], check=True, env=environment)

        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), TestHandler)
        server_thread = threading.Thread(target=server.serve_forever, daemon=True)
        server_thread.start()
        try:
            base_url = f"http://127.0.0.1:{server.server_port}"
            subprocess.run([integration, base_url], check=True, env=environment)
            if TestHandler.counters["/timeout-default"] != 1:
                raise AssertionError("default timeout policy retried a POST")
            if TestHandler.counters["/timeout-partial"] != 2:
                raise AssertionError("explicit timeout retry did not run exactly twice")
        finally:
            server.shutdown()
            server.server_close()
            server_thread.join()


if __name__ == "__main__":
    main()
