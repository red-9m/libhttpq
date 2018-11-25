# libHTTPQ ver. 0.0.8
libHTTPQ - Simple HTTP query C library based on CURL. Supports http headers, username/password, POST, key/value POST, multipart POST, file transfer in multipart POST requests.

# Configure as shared library:
cmake .

# Configure as static library:
cmake . -DBUILD_SHARED_LIBS=OFF

# Compile library:
cmake --build .

# Clean:
./clean-dir

# Run sample - send Telegram bot message
> ./httpq-sample BOT_KEY CHAT_ID
