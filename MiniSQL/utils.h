#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

void trim_in_place(char* text);
void to_upper_copy(char* dest, size_t dest_size, const char* src);
int is_integer_text(const char* text);
int is_wrapped_with_single_quotes(const char* text);
void strip_single_quotes(char* dest, size_t dest_size, const char* src);
int equals_ignore_case(const char* left, const char* right);

#endif