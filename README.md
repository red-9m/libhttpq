# libHTTPQ ver. 0.0.7
libHTTPQ - Simple HTTP query C library based on CURL. Supports http headers, username/password, POST, multipart POST, file transfer in multipart POST requests.

# Configure as shared library:
cmake .

# Configure as static library:
cmake . -DBUILD_SHARED_LIBS=OFF

# Compile library:
cmake --build .

# Compile library and sample:
cmake --build . --target sample

# Clean:
./clean-dir

# Run sample - send Telegram bot message
> ./sample BOT_KEY CHAT_ID
