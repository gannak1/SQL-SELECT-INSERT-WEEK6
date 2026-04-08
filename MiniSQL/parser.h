#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#include "types.h"

int parse_query(const char* input, Query* query);
int parse_insert_command(const Query* query, InsertCommand* insert_command, char* error_message, size_t error_size);
int is_exit_command(const char* input);

#endif
