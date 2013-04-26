#ifndef STRING_H_
#define STRING_H_

#include <stddef.h>

typedef struct {
	char *c_str;
	size_t size;
} String;

/**
 * Create a new string from a C string
 */
String *string_new(const char *c_string);

/**
 * Create a new string from a C string.
 * Alternative version, takes two pointers.
 */
String *string_new_from_range(const char *begin, const char *end);

/**
 * Free an existing string
 */
void string_delete(String *string);

/**
 * Free a NULL-terminated array of Strings
 */
void string_delete_array(String **array);

/**
 * Copy a string
 */
String *string_copy(const String *string);

/**
 * Append to a string
 */
String *string_append(String *target, const String *source);
String *string_append_c(String *target, const char *source);

/**
 * Split a string
 */
String **string_split(const String *source, const char *delimiter);

#endif

