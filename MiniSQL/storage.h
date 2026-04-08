#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>

#include "types.h"

int append_insert_to_csv(const InsertCommand* insert_command, char* error_message, size_t error_size);
int execute_select_query(const Query* query, char* error_message, size_t error_size);

#endif
