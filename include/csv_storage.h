/*
 * 파일 목적:
 *   data/<table>.csv 파일을 생성, append, read 하는 저장소 모듈 인터페이스를 선언한다.
 *
 * 전체 흐름에서 위치:
 *   executor는 schema 검증이 끝난 뒤 이 모듈을 통해 실제 데이터를 파일에 저장하거나 읽는다.
 *
 * 주요 내용:
 *   - CsvRow, CsvTable 구조체
 *   - csv_append_row, csv_load_table 함수
 *   - 해제 함수
 *
 * 초보자 포인트:
 *   이번 프로젝트는 "진짜 DB 파일 포맷" 대신 단순한 CSV를 택해서
 *   SQL 흐름 자체에 집중하도록 했다.
 *
 * 메모리 / 문자열 주의점:
 *   CSV를 읽으면 각 셀 문자열을 따로 복사해 저장하므로,
 *   free_csv_table로 행과 셀을 모두 해제해야 한다.
 */

#ifndef CSV_STORAGE_H
#define CSV_STORAGE_H

#include <stddef.h>

#include "ast.h"
#include "error.h"
#include "schema.h"

typedef struct {
    char **values;
    size_t value_count;
} CsvRow;

typedef struct {
    char **header;
    size_t column_count;
    CsvRow *rows;
    size_t row_count;
    size_t row_capacity;
} CsvTable;

int csv_append_row(
    const char *data_dir,
    const TableSchema *schema,
    const LiteralValue *values,
    size_t value_count,
    SqlError *error
);

int csv_load_table(const char *data_dir, const TableSchema *schema, CsvTable *out_table, SqlError *error);
void free_csv_table(CsvTable *table);

#endif
