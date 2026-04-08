/*
 * 파일 목적:
 *   단순 CSV 포맷으로 테이블 데이터를 저장하고 읽는 로직을 구현한다.
 *
 * 전체 흐름에서 위치:
 *   executor는 INSERT에서 append, SELECT에서 load를 호출해 실제 파일 작업을 수행한다.
 *
 * 주요 내용:
 *   - CSV header 생성
 *   - row append
 *   - row scan 및 메모리 적재
 *
 * 초보자 포인트:
 *   이번 CSV는 쉼표/개행 escape를 지원하지 않는 아주 작은 subset이다.
 *   그래서 split 로직을 직접 써도 구조를 설명하기 쉽다.
 *
 * 메모리 / 문자열 주의점:
 *   CSV를 읽을 때 각 셀을 별도 문자열로 복사하므로,
 *   free_csv_table에서 행 단위와 셀 단위를 모두 정리해야 한다.
 */

#include "csv_storage.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

static int copy_schema_header(CsvTable *table, const TableSchema *schema, SqlError *error);
static int append_csv_row(CsvTable *table, char **owned_values, size_t value_count, SqlError *error);
static int split_csv_line(const char *line, char ***out_values, size_t *out_count, SqlError *error);

/*
 * 목적:
 *   schema 컬럼 이름을 복사해 CsvTable.header를 채운다.
 *
 * 입력:
 *   table - 채울 CSV 테이블.
 *   schema - 기준이 되는 schema.
 *   error - 메모리 실패 시 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   컬럼 개수만큼 문자열 배열을 만들고 컬럼 이름을 복사한다.
 *
 * 초보자 주의:
 *   SELECT에서 data 파일이 없어도 header는 schema로부터 만들 수 있어야 한다.
 */
static int copy_schema_header(CsvTable *table, const TableSchema *schema, SqlError *error) {
    size_t index;

    table->header = (char **)calloc(schema->column_count, sizeof(char *));
    if (table->header == NULL) {
        sql_error_set(error, "ERROR: out of memory while allocating CSV header");
        return 0;
    }

    table->column_count = schema->column_count;

    for (index = 0; index < schema->column_count; ++index) {
        table->header[index] = sql_strdup(schema->columns[index].name);
        if (table->header[index] == NULL) {
            sql_error_set(error, "ERROR: out of memory while copying CSV header");
            return 0;
        }
    }

    return 1;
}

/*
 * 목적:
 *   CsvTable 끝에 행 하나를 추가한다.
 *
 * 입력:
 *   table - 행을 추가할 CSV 테이블.
 *   owned_values - 이미 할당된 셀 문자열 배열 소유권.
 *   value_count - 셀 개수.
 *   error - 메모리 실패 시 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   rows 배열을 필요 시 늘리고 새 CsvRow를 끝에 붙인다.
 *
 * 초보자 주의:
 *   owned_values를 그대로 넘겨받아 소유권을 가져오므로,
 *   성공 후에는 호출자가 그 배열을 다시 free하면 안 된다.
 */
static int append_csv_row(CsvTable *table, char **owned_values, size_t value_count, SqlError *error) {
    CsvRow *resized_rows;
    size_t new_capacity;

    if (table->row_count == table->row_capacity) {
        new_capacity = table->row_capacity == 0 ? 8 : table->row_capacity * 2;
        resized_rows = (CsvRow *)realloc(table->rows, new_capacity * sizeof(CsvRow));
        if (resized_rows == NULL) {
            sql_error_set(error, "ERROR: out of memory while growing CSV rows");
            return 0;
        }
        table->rows = resized_rows;
        table->row_capacity = new_capacity;
    }

    table->rows[table->row_count].values = owned_values;
    table->rows[table->row_count].value_count = value_count;
    table->row_count += 1;
    return 1;
}

/*
 * 목적:
 *   단순 CSV 한 줄을 쉼표 기준으로 나눠 문자열 배열로 만든다.
 *
 * 입력:
 *   line - 줄바꿈이 제거된 한 줄 문자열.
 *   out_values - 성공 시 셀 문자열 배열.
 *   out_count - 성공 시 셀 개수.
 *   error - 메모리 실패 시 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   쉼표 위치를 직접 탐색해 구간별로 sql_strndup 복사본을 만든다.
 *
 * 초보자 주의:
 *   strtok는 빈 셀을 다루기 까다롭고 원본 문자열을 파괴하므로,
 *   여기서는 길이 기반 split을 직접 구현한다.
 */
static int split_csv_line(const char *line, char ***out_values, size_t *out_count, SqlError *error) {
    size_t length;
    size_t index;
    size_t start;
    size_t value_count;
    char **values;
    size_t value_index;

    if (line == NULL || out_values == NULL || out_count == NULL) {
        sql_error_set(error, "ERROR: invalid CSV split input");
        return 0;
    }

    length = strlen(line);
    value_count = 1;
    for (index = 0; index < length; ++index) {
        if (line[index] == ',') {
            value_count += 1;
        }
    }

    values = (char **)calloc(value_count, sizeof(char *));
    if (values == NULL) {
        sql_error_set(error, "ERROR: out of memory while splitting CSV line");
        return 0;
    }

    start = 0;
    value_index = 0;
    for (index = 0; index <= length; ++index) {
        if (line[index] == ',' || line[index] == '\0') {
            values[value_index] = sql_strndup(line + start, index - start);
            if (values[value_index] == NULL) {
                size_t cleanup_index;
                for (cleanup_index = 0; cleanup_index < value_index; ++cleanup_index) {
                    free(values[cleanup_index]);
                }
                free(values);
                sql_error_set(error, "ERROR: out of memory while copying CSV cell");
                return 0;
            }
            value_index += 1;
            start = index + 1;
        }
    }

    *out_values = values;
    *out_count = value_count;
    return 1;
}

/*
 * 목적:
 *   schema에 맞는 header를 쓰고 새 row를 CSV 파일에 append한다.
 *
 * 입력:
 *   data_dir - data 디렉터리 경로.
 *   schema - 대상 테이블 schema.
 *   values - INSERT literal 배열.
 *   value_count - 값 개수.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   파일이 없으면 header부터 쓰고, 이후 현재 row를 쉼표로 이어 붙여 저장한다.
 *
 * 초보자 주의:
 *   새 파일일 때만 header를 쓰도록 분기해야 SELECT가 읽을 수 있는 정상 CSV가 된다.
 */
int csv_append_row(
    const char *data_dir,
    const TableSchema *schema,
    const LiteralValue *values,
    size_t value_count,
    SqlError *error
) {
    char path[SQL_PATH_LENGTH];
    FILE *file;
    int file_exists;
    size_t index;

    if (data_dir == NULL || schema == NULL || values == NULL) {
        sql_error_set(error, "ERROR: csv_append_row received invalid input");
        return 0;
    }

    snprintf(path, sizeof(path), "%s/%s.csv", data_dir, schema->table_name);
    file_exists = access(path, F_OK) == 0;

    file = fopen(path, file_exists ? "a" : "w");
    if (file == NULL) {
        sql_error_set(error, "ERROR: cannot open data file: %s", path);
        return 0;
    }

    if (!file_exists) {
        for (index = 0; index < schema->column_count; ++index) {
            if (index > 0) {
                fputc(',', file);
            }
            fputs(schema->columns[index].name, file);
        }
        fputc('\n', file);
    }

    for (index = 0; index < value_count; ++index) {
        if (index > 0) {
            fputc(',', file);
        }

        if (values[index].type == DATA_TYPE_INT) {
            fprintf(file, "%ld", values[index].int_value);
        } else {
            fputs(values[index].text_value, file);
        }
    }
    fputc('\n', file);

    if (fclose(file) != 0) {
        sql_error_set(error, "ERROR: failed to close data file: %s", path);
        return 0;
    }

    return 1;
}

/*
 * 목적:
 *   테이블 CSV 파일을 읽어 header와 row들을 메모리로 적재한다.
 *
 * 입력:
 *   data_dir - data 디렉터리 경로.
 *   schema - 기대하는 schema.
 *   out_table - 성공 시 채워질 CSV 테이블.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   파일이 없으면 빈 테이블을 만들고,
 *   파일이 있으면 header를 검증한 뒤 모든 row를 읽는다.
 *
 * 초보자 주의:
 *   data 파일이 없더라도 SELECT는 빈 결과를 돌려줄 수 있어야 하므로,
 *   "파일 없음"을 에러가 아니라 정상 빈 테이블로 처리한다.
 */
int csv_load_table(const char *data_dir, const TableSchema *schema, CsvTable *out_table, SqlError *error) {
    char path[SQL_PATH_LENGTH];
    FILE *file;
    char line_buffer[SQL_LINE_BUFFER_SIZE];
    size_t line_number;

    if (data_dir == NULL || schema == NULL || out_table == NULL) {
        sql_error_set(error, "ERROR: csv_load_table received invalid input");
        return 0;
    }

    memset(out_table, 0, sizeof(*out_table));
    if (!copy_schema_header(out_table, schema, error)) {
        free_csv_table(out_table);
        return 0;
    }

    snprintf(path, sizeof(path), "%s/%s.csv", data_dir, schema->table_name);
    file = fopen(path, "r");
    if (file == NULL) {
        if (errno == ENOENT) {
            return 1;
        }
        sql_error_set(error, "ERROR: cannot open data file: %s", path);
        free_csv_table(out_table);
        return 0;
    }

    line_number = 0;
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) {
        char **cells;
        size_t cell_count;

        line_number += 1;
        sql_strip_trailing_newline(line_buffer);

        if (line_number == 1) {
            size_t header_index;

            if (!split_csv_line(line_buffer, &cells, &cell_count, error)) {
                fclose(file);
                free_csv_table(out_table);
                return 0;
            }

            if (cell_count != schema->column_count) {
                size_t cleanup_index;
                for (cleanup_index = 0; cleanup_index < cell_count; ++cleanup_index) {
                    free(cells[cleanup_index]);
                }
                free(cells);
                fclose(file);
                free_csv_table(out_table);
                sql_error_set(error, "ERROR: CSV header column count mismatch for table %s", schema->table_name);
                return 0;
            }

            for (header_index = 0; header_index < cell_count; ++header_index) {
                if (strcmp(cells[header_index], schema->columns[header_index].name) != 0) {
                    size_t cleanup_index;
                    for (cleanup_index = 0; cleanup_index < cell_count; ++cleanup_index) {
                        free(cells[cleanup_index]);
                    }
                    free(cells);
                    fclose(file);
                    free_csv_table(out_table);
                    sql_error_set(error, "ERROR: CSV header mismatch for table %s", schema->table_name);
                    return 0;
                }
            }

            for (header_index = 0; header_index < cell_count; ++header_index) {
                free(cells[header_index]);
            }
            free(cells);
            continue;
        }

        if (line_buffer[0] == '\0') {
            continue;
        }

        if (!split_csv_line(line_buffer, &cells, &cell_count, error)) {
            fclose(file);
            free_csv_table(out_table);
            return 0;
        }

        if (cell_count != schema->column_count) {
            size_t cleanup_index;
            for (cleanup_index = 0; cleanup_index < cell_count; ++cleanup_index) {
                free(cells[cleanup_index]);
            }
            free(cells);
            fclose(file);
            free_csv_table(out_table);
            sql_error_set(error, "ERROR: CSV row column count mismatch for table %s", schema->table_name);
            return 0;
        }

        if (!append_csv_row(out_table, cells, cell_count, error)) {
            size_t cleanup_index;
            for (cleanup_index = 0; cleanup_index < cell_count; ++cleanup_index) {
                free(cells[cleanup_index]);
            }
            free(cells);
            fclose(file);
            free_csv_table(out_table);
            return 0;
        }
    }

    fclose(file);
    return 1;
}

/*
 * 목적:
 *   CsvTable 안의 header와 row 메모리를 모두 해제한다.
 *
 * 입력:
 *   table - 정리할 CSV 테이블.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   header 문자열 배열, 각 row의 셀 문자열 배열, rows 배열을 순서대로 free한다.
 *
 * 초보자 주의:
 *   2차원 배열을 다룰 때는 가장 안쪽 문자열부터 차례대로 정리해야 한다.
 */
void free_csv_table(CsvTable *table) {
    size_t column_index;
    size_t row_index;

    if (table == NULL) {
        return;
    }

    for (column_index = 0; column_index < table->column_count; ++column_index) {
        free(table->header[column_index]);
    }
    free(table->header);

    for (row_index = 0; row_index < table->row_count; ++row_index) {
        size_t value_index;
        for (value_index = 0; value_index < table->rows[row_index].value_count; ++value_index) {
            free(table->rows[row_index].values[value_index]);
        }
        free(table->rows[row_index].values);
    }

    free(table->rows);
    table->header = NULL;
    table->rows = NULL;
    table->column_count = 0;
    table->row_count = 0;
    table->row_capacity = 0;
}
