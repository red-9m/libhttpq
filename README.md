# libHTTPQ ver. 0.0.4a
libHTTPQ - Simple HTTP query C library based on CURL

# Configure as shared library:
cmake .

# Configure as static library:
cmake . -DBUILD_SHARED_LIBS=OFF

# Compile:
cmake --build .

# Run sample - send Telegram bot message
> ./sample BOT_KEY CHAT_ID
