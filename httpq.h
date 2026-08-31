#ifndef _LIBHTTPQ_H_
#define _LIBHTTPQ_H_

extern const int HTTPQ_OK;

enum httpq_retry_policy
{
    rpNoRetry,              // No retry for request
    rpRetryOnTimeoutError   // Retry request on timeout error only (see httpq_set_max_time())
};

/** @brief Initialize HTTPQ library
 *
 *         Requires libcurl 8.11.1 or newer with CURL_VERSION_THREADSAFE.
 *         Request state is isolated per calling thread. Each thread that calls
 *         httpq_init() must call httpq_cleanup() before it exits.
 *
 *  @return CURL error code
 */
extern long httpq_init(void);

/** @brief Release HTTPQ resources owned by the calling thread
 *
 */
extern void httpq_cleanup(void);

/** @brief Set URL data
 *
 *         Default value: no value
 *
 *  @param aURL URL string
 *  @return CURL error code
 */
extern long httpq_set_url(const char *aURL);

/** @brief Set POST data. `postData` does not processed with curl_easy_escape()
 *
 *         Default value: no value
 *
 *  @param postData POST data
 *  @return CURL error code
 */
extern long httpq_set_post(const char *postData);

/** @brief Set POST data. Process values with curl_easy_escape(). Keys stay untouched
 *
 *         Default value: no value
 *
 *  @param postData Array of POST key/value
 *                  Ex.: const char *pdata[][2] = {{"key1", "value1"}, {"key2", "value2"}, {NULL, NULL}};
 *                  Array must be ended with {NULL, NULL} element
 *  @return CURL error code
 */
extern long httpq_set_key_post(const char *postData[][2]);

/** @brief Set multipart POST data
 *
 *         Default value: no value
 *
 *  @param postData Array of POST name/value/file-marker rows
 *                  Ex.: const char *pdata[][3] = {{"sender", "John", NULL}, {"pic", "/home/john/mypic.jpg", "file"}, {NULL, NULL, NULL}};
 *                  Array must be ended with {NULL, NULL, NULL} element
 *                  The file marker must be NULL for a regular field and any
 *                  non-NULL string for a file whose path is given in `value`.
 *                  The marker string's contents are ignored.
 *                  Multipart data applies to one request and must be set again
 *                  before a later request.
 *  @return CURL error code
 */
extern long httpq_set_key_http_post(const char *postData[][3]);

/** @brief Set header data
 *
 *         Default value: no value
 *
 *  @param headerData Array of header values
 *                    Ex.: const char *hdata[] = {"header1", "header2", NULL};
 *                    Array must be ended with NULL element
 *                    Headers apply to one request and must be set again before
 *                    a later request.
 *  @return CURL error code
 */
extern long httpq_set_headers(const char *headerData[]);

/** @brief Set username
 *
 *         Default value: no value
 *
 *  @param userName Username string
 *  @return CURL error code
 */
extern long httpq_set_user_name(const char *userName);

/** @brief Set user password
 *
 *         Default value: no value
 *
 *  @param userPwd Password string
 *  @return CURL error code
 */
extern long httpq_set_user_pwd(const char *userPwd);

/** @brief Set desired response limit
 *
 *         Response buffer size grows as it receives data from a server
 *         Default value: 4121440 (4Mb)
 *
 *  @param respLimit New response limit in bytes
 *  @return CURL error code
 */
extern long httpq_set_limit_resp(long respLimit);

/** @brief Set maximal time limit for request
 *
 *         Default value: 20 seconds
 *         With a synchronous libcurl resolver, DNS lookup time cannot be
 *         bounded because signal handling is disabled for thread safety.
 *
 *  @param maxTime New time limit for request in seconds. 0 - for unlimited
 *  @return CURL error code
 */
extern long httpq_set_max_time(long maxTime);

/** @brief Set retry policy
 *
 *         Default value: rpNoRetry. Retrying a POST request can repeat a
 *         non-idempotent operation and must be enabled explicitly.
 *
 *  @param retryPolicy See enum httpq_retry_policy
 *  @return CURL error code
 */
extern long httpq_set_retry(enum httpq_retry_policy retryPolicy);

/** @brief Make HTTP/HTTPS POST request
 *
 *  @param errorCode Non-NULL output for the CURL error code
 *  @param httpCode Non-NULL output for the HTTP code (available if
 *                  errorCode is CURLE_OK)
 *  @return Allocated buffer with HTTP/HTTPS response or NULL in case of error
 *          You must delete buffer later with free()
 *          Configured headers and multipart data are cleared after the request.
 */
extern char* httpq_request_post(long* errorCode, long* httpCode);

/** @brief Reset all options that was set by httpq_set_xxx() calls to default
 *
 */
extern void httpq_reset(void);

/** @brief Converts CURL error code to string
 *
 *  @param errorCode CURL error code
 *  @return Error string
 */
const char* httpq_error(long errorCode);

#endif // _LIBHTTPQ_H_
