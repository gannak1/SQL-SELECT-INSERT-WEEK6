#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

void trim_newline(char *str);
void trim_spaces(char *str);
void strip_matching_quotes(char *str);
int starts_with_ignore_case(const char *str, const char *prefix);
int is_quoted_string(const char *str);
int is_blank_string(const char *str);
int is_integer_string(const char *str);
size_t copy_trimmed(char *dest, size_t dest_size, const char *src);

#endif
