#ifndef _LIBHTTPQ_H_
#define _LIBHTTPQ_H_

// Do HTTP/HTTPS POST request. You need to provide response buffer in aResp and buffer length
// Return: 100 and greater - HTTP code; less than 100 - CURL error
extern long httpq_post(const char *aURL, const char* postData[][2], long postCount, char *aResp, long respLen);

// Converts error code to string
const char* httpq_error(long aError);

#endif
