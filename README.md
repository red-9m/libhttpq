# libHTTPQ ver. 0.0.5a
libHTTPQ - Simple HTTP query C library based on CURL. Supports http headers, username/password, POST and multipart POST requests.

# Configure as shared library:
cmake .

# Configure as static library:
cmake . -DBUILD_SHARED_LIBS=OFF

# Compile:
cmake --build .

# Run sample - send Telegram bot message
> ./sample BOT_KEY CHAT_ID
