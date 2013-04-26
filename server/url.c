// url.h
//
// Functions for parsing and unparsing URLs

#define _BSD_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "url.h"

char *url_get_scheme(const char *base) {
	int scheme_begin = -1;
	int scheme_end = -1;
	sscanf(base, " %n%*[-+.a-zA-Z0-9]:%n", &scheme_begin, &scheme_end);
	// Scheme found and it is of canonical format
	if (scheme_end >= 0 && isalpha(base[scheme_begin])) {
		return strndup(base + scheme_begin, scheme_end - scheme_begin - 1);
	}
	return NULL;
}

char *url_get_userinfo(const char *base) {
	int userinfo_begin = -1;
	int userinfo_end = -1;
	sscanf(base, " %n%*[^@]@%n", &userinfo_begin, &userinfo_end);
	// Scheme found and it is of canonical format
	if (userinfo_end >= 0 && isalpha(base[userinfo_begin])) {
		return strndup(base + userinfo_begin, userinfo_end - userinfo_begin - 1);
	}
	return NULL;
}

char *url_get_authority(const char *base, int field) {
	if (field == URL_AUTHORITY) {
		int authority_end = -1;
		sscanf(base, "%*[^/?#]%n", &authority_end);
		return strndup(base, authority_end);
	}
	const char *next_field = base;

	// Parse userinfo@
	int userinfo_end = -1;
	int gteq_zero_if_userinfo = sscanf(next_field, " %*[^@]@%n", &userinfo_end);
	if (field & URL_USERINFO) { // Requested parsing userinfo
		return url_get_userinfo(next_field);
	}
	if (gteq_zero_if_userinfo >= 0) { // Url had a scheme
		next_field += userinfo_end;
	}

	// Parse hostname
	int host_end = -1;
	sscanf(next_field, "%*[^:/?#]%n", &host_end);
	if (field & URL_HOST) {
		return strndup(next_field, host_end);
	}
	next_field += host_end;

	// Parse :port, if exists
	if (*next_field == ':') {
		int port_end = -1;
		sscanf(next_field, "%*[^/?#]%n", &port_end);
		if (field & URL_PORT) {
			return strndup(next_field + 1, port_end - 1);
		}
		next_field += port_end;
	}

	return NULL;
}

char *url_get_path(const char *base) {
	int path_end = -1;
	sscanf(base, "%*[^?#]%n", &path_end);
	if (path_end > 0) {
		return strndup(base, path_end);	
	}
	else {
		return strdup("/");
	}
}

char *url_get_hier_part(const char *base, int field) {
	// Parse the whole hier-part
	if (field == URL_HIER_PART) {
		int hier_part_end = -1;
		sscanf(base, "%*[^?#]%n", &hier_part_end);
		return strndup(base, hier_part_end);
	}

	// Parse some subfield of authority
	int leading_slashes = -1;
	sscanf(base, "%*[/]%n", &leading_slashes);
	// Full authority + path
	if (leading_slashes == 2) {
		int authority_end = -1;
		sscanf(base + leading_slashes, "%*[^/]%n", &authority_end);
		if (field & URL_AUTHORITY) {
			return url_get_authority(base + leading_slashes, field);
		}
		if (field & URL_PATH) {
			return url_get_path(base + leading_slashes + authority_end);
		}
	// Only path component
	} else if (leading_slashes == -1 && (field & URL_PATH)) {
		return url_get_path(base);
	}

	return NULL;
}

char *url_get_query(const char *base) {
	int query_end = -1;
	sscanf(base, "%*[^#]%n", &query_end);
	if (query_end > 0) {
		return strndup(base, query_end);
	}
	else {
		return strdup("");
	}
}

char *url_get_fragment(const char *base) {
	int fragment_end = -1;
	sscanf(base, "%*s%n", &fragment_end);
	return strndup(base, fragment_end);
}

// Looks up the url for a given field and then returns a String
char *url_get_field(const char *url, int field) {
	assert(url);
	assert(field);

	const char *next_field = url;

	// Parse scheme:
	int scheme_end = -1;
	int minus_one_if_no_scheme = sscanf(next_field, " %*[^:]:%n", &scheme_end);
	if (field & URL_SCHEME) { // Requested parsing scheme
		return url_get_scheme(next_field);
	}
	if (minus_one_if_no_scheme >= 0) { // Url had a scheme
		next_field += scheme_end;
	}

	// Parse //hier-part
	int hier_part_end = -1;
	sscanf(next_field, "%*[^?#]%n", &hier_part_end);
	if (field & URL_HIER_PART) {
		return url_get_hier_part(next_field, field);
	}
	next_field += hier_part_end;

	// Parse ?query, if exists
	if (*next_field == '?') {
		int query_end = -1;
		sscanf(next_field, "%*[^#]%n", &query_end);
		if (field & URL_QUERY) {
			return url_get_query(next_field + 1);
		}
		next_field += query_end;
	}

	// Special field value for stripping the fragment
	if (field == URL_ABSOLUTE) {
		return strndup(url, next_field - url);
	}

	// Parse #fragment, if exists
	if (*next_field == '#') {
		int query_end = -1;
		sscanf(next_field, "%*s%n", &query_end);
		if (field & URL_FRAGMENT) {
			return url_get_fragment(next_field + 1);
		}
	}

	return NULL;
}

/*
int main(int argc, char *argv[]) {
	(void) argc;

	char *absolute = url_get_field(argv[1], URL_ABSOLUTE);
	char *scheme = url_get_field(argv[1], URL_SCHEME);
	char *hier_part = url_get_field(argv[1], URL_HIER_PART);
	char *authority = url_get_field(argv[1], URL_AUTHORITY);
	char *userinfo = url_get_field(argv[1], URL_USERINFO);
	char *host = url_get_field(argv[1], URL_HOST);
	char *port = url_get_field(argv[1], URL_PORT);
	char *path = url_get_field(argv[1], URL_PATH);
	char *query = url_get_field(argv[1], URL_QUERY);
	char *fragment = url_get_field(argv[1], URL_FRAGMENT);

	printf("absolute:  '%s'\n", absolute);
	printf("scheme:    '%s'\n", scheme);
	printf("hier_part: '%s'\n", hier_part);
	printf("authority: '%s'\n", authority);
	printf("userinfo:  '%s'\n", userinfo);
	printf("host:      '%s'\n", host);
	printf("port:      '%s'\n", port);
	printf("path:      '%s'\n", path);
	printf("query:     '%s'\n", query);
	printf("fragment:  '%s'\n", fragment);

	free(fragment);
	free(query);
	free(path);
	free(port);
	free(host);
	free(userinfo);
	free(authority);
	free(hier_part);
	free(scheme);
	free(absolute);

	//printf("query:     '%.*s'\n", (int) url.query_size, url.query);
	//printf("fragment:  '%.*s'\n", (int) url.fragment_size, url.fragment);
}
*/
