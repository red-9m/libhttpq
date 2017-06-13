#ifndef _LIBHTTPQ_H_
#define _LIBHTTPQ_H_

/** @brief Prepare POST string to be passed to httpq_prepare_post()
 *
 *  @param postData Array of post data
 *  @param postCount Item count in postData
 *  @param outPost Pointer to a buffer where POST string will be stored
 *  @param postLen Length of outPost
 *  @return 0 - ok; other - CURL error
 */
extern long httpq_prepare_post(const char *postData[][2], long postCount, char *outPost, long postLen);

/** @brief Make HTTP/HTTPS POST request
 *
 *  @param aURL HTTP/HTTPS URL
 *  @param aPost POST string prepared with httpq_prepare_post()
 *  @param headerData Array of header data
 *  @param headerCount Item count in headerData
 *  @param aResp Pointer to a buffer where response string will be stored
 *  @param respLen Length of aResp
 *  @return 100 and greater - HTTP code; less than 100 - CURL error
 */
extern long httpq_request_post(const char *aURL, const char *aPost,
                               const char *headerData[], long headerCount,
                               char *aResp, long respLen);

// Converts error code to string
/** @brief Converts any error code produced with above functions to string
 *
 *  @param aError Error code
 *  @return Error string
 */
const char* httpq_error(long aError);

#endif
