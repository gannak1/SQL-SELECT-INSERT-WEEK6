#ifndef PRINTER_H
#define PRINTER_H

#include "constants.h"

void print_user_row(char *const values[USER_COLUMN_COUNT]);
void print_select_header(void);
void print_rows_selected(int count);
void print_message(const char *message);
void print_error(const char *message);

#endif
