#ifndef PRINTER_H
#define PRINTER_H

#include "types.h"

const char* query_type_to_string(QueryType type);
const char* value_type_to_string(ValueType type);
void print_query(const Query* query);
void print_insert_command(const InsertCommand* insert_command);

#endif
