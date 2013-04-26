#ifndef URL_H_
#define URL_H_

// URL fields

// http://user@host:port/path?query#fragment
// ^^^^
#define URL_SCHEME 1

// http://user@host:port/path?query#fragment
//                            ^^^^^
#define URL_QUERY 2

// http://user@host:port/path?query#fragment
//                                  ^^^^^^^^
#define URL_FRAGMENT 4

// http://user@host:port/path?query#fragment
//        ^^^^
#define URL_USERINFO 8

// http://user@host:port/path?query#fragment
//             ^^^^
#define URL_HOST 32

// http://user@host:port/path?query#fragment
//                  ^^^^
#define URL_PORT 64

// http://user@host:port/path?query#fragment
//             ^^^^^^^^^
#define URL_HOSTPORT (URL_HOST | URL_PORT)

// http://user@host:port/path?query#fragment
//        ^^^^^^^^^^^^^^
#define URL_AUTHORITY (URL_USERINFO | URL_HOSTPORT)

// http://user@host:port/path?query#fragment
//                      ^^^^^
#define URL_PATH 128

// http://user@host:port/path?query#fragment
//        ^^^^^^^^^^^^^^^^^^^
#define URL_HIER_PART (URL_AUTHORITY | URL_PATH)

// http://user@host:port/path?query#fragment
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define URL_ABSOLUTE 256

/**
 * Get a field from a URL. See the #defines above for field specifications.
 * @param url URL string to parse
 * @param field Field to get
 * @return The given field, allocated with malloc, or NULL if parsing failed.
 */
char *url_get_field(const char *url, int field);

#endif // URL_H_
