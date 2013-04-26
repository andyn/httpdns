#include "common.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "string.h"

/**
 * Create a new string from a C string
 */
String *string_new(const char *c_string) {
	String *string = malloc(sizeof(*string));
	if (string) {
		string->c_str = strdup(c_string);
		string->size = strlen(string->c_str);
	}
	return string;
}

/**
 * Create a new string from a C string.
 * Alternative version, takes two pointers.
 */
String *string_new_from_range(const char *begin, const char *end) {
	if (end < begin) {
		return NULL;
	}

	String *string = malloc(sizeof(*string));
	if (string) {
		string->c_str = malloc(sizeof(*string->c_str) * (end - begin + 1));
		string->size = end - begin;
		memcpy(string->c_str, begin, string->size);
		string->c_str[string->size] = '\0';
	}
	return string;
}

/**
 * Free an existing String
 */
void string_delete(String *string) {
	if (string) {
		free(string->c_str);
		free(string);
	}
}

/**
 * Free a NULL-terminated array of Strings
 */
void string_delete_array(String **array) {
	if (array) {
		for (String **array_iter = array; *array_iter; ++array_iter) {
			string_delete(*array_iter);
		}
		free(array);
	}
}

/**
 * Copy a string
 */
String *string_copy(const String *string) {
	if (!string) {
		return NULL;
	}

	return string_new(string->c_str);
}

/**
 * Append to a string
 */
String *string_append(String *target, const String *source) {
	if (!target) {
		return NULL;
	}

	if (source) {
		size_t new_size = target->size + source->size;
		char *new_c_str = realloc(target->c_str, sizeof(*new_c_str) * (new_size + 1));
		if (new_c_str) {
			memcpy(new_c_str + target->size, source->c_str, source->size);
			new_c_str[new_size] = '\0';
			target->c_str = new_c_str;
			target->size = new_size;
		} else {
			target = NULL;
		}
	}

	return target;
}

/**
 * Append to a string
 */
String *string_append_c(String *target, const char *source) {
	if (!target || !source) {
		return NULL;
	}

	String str;
	str.c_str = (char *) source;
	str.size = strlen(source);

	return string_append(target, &str);
}

/**
 * Split a string
 * @return A NULL-terminated array of pointers to string, split by the delimiter string
 */
String **string_split(const String *source, const char *delimiter) {
	if (!source || !delimiter) {
		return NULL;
	}

	String **string_array = malloc(sizeof(*string_array));
	if (!string_array) {
		return NULL;
	}
	size_t string_array_size = 0;
	string_array[string_array_size] = '\0';

	size_t delimiter_length = strlen(delimiter);
	ssize_t chars_remaining = source->size;

	const char *field_begin = source->c_str;
	while (chars_remaining > 0) {
		// Find next delimiter
		const char *field_end = strstr(field_begin, delimiter);
		if (!field_end) {
			// Delimiter not found -> assume last field
			field_end = field_begin + chars_remaining;
		}
		// Append to the array
		String **string_array_tmp = realloc(string_array, sizeof(*string_array_tmp) * (string_array_size + 2)); // Current plus one plus NULL
		if (string_array_tmp) {
			string_array = string_array_tmp;
			string_array[string_array_size] = string_new_from_range(field_begin, field_end);
			string_array[string_array_size + 1] = NULL;
			++string_array_size;
		}
		else {
			// Malloc error, cleanup.
			string_delete_array(string_array);
			string_array = NULL;
			break;
		}
		chars_remaining -= (field_end - field_begin) + delimiter_length;
		field_begin = field_end + delimiter_length;
	}

	return string_array;
}
