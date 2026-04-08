#ifndef STORAGE_H
#define STORAGE_H

#include <stdio.h>

#include "constants.h"
#include "types.h"

typedef struct {
    char **rows;
    int count;
    int capacity;
} RowArray;

int append_user_record(const InsertCommand *insert_cmd);
int read_all_users(RowArray *row_array);
int read_user_row_by_id(int id, char **row_out);
int split_csv_row(const char *row, char *values[USER_COLUMN_COUNT]);
int row_matches_condition(char *const values[USER_COLUMN_COUNT], const Condition *condition);
void storage_shutdown(void);
void write_csv_field(FILE *fp, const char *value, int quote);
void free_row_array(RowArray *row_array);
void free_csv_values(char *values[USER_COLUMN_COUNT]);

#endif
