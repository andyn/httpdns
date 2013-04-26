#ifndef HTTP_H_
#define HTTP_H_

#include <stddef.h>

/**
 * HTTP PUT buffer contents to a remote URL
 * @param url URL to PUT the contents to
 * @param buf Buffer whose contents to send
 * @param count Size of buffer (number of bytes to send)
 * @return HTTP status code, 200 for OK. See the HTTP RFC for details.
 */
int http_put_buf(const char *url, const void *buf, size_t count);

#endif
