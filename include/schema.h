/*
 * 파일 목적:
 *   schema/<table>.schema 파일을 읽어 컬럼 이름과 타입 정보를 구조체로 만든다.
 *
 * 전체 흐름에서 위치:
 *   executor는 INSERT/SELECT를 실행하기 전에 반드시 schema를 읽어
 *   컬럼 이름과 타입이 맞는지 확인한다.
 *
 * 주요 내용:
 *   - ColumnSchema, TableSchema 구조체
 *   - schema 로딩 함수
 *   - 컬럼 검색 함수
 *
 * 초보자 포인트:
 *   SQL 문장만으로는 컬럼 타입을 알 수 없으므로,
 *   schema는 실행기에게 "정답표" 역할을 한다.
 *
 * 메모리 / 문자열 주의점:
 *   schema 파일을 읽으며 컬럼 이름 배열을 동적 생성하므로,
 *   사용 후 free_schema로 정리해야 한다.
 */

#ifndef SCHEMA_H
#define SCHEMA_H

#include <stddef.h>

#include "common.h"
#include "error.h"

typedef struct {
    char *name;
    DataType type;
} ColumnSchema;

typedef struct {
    char *table_name;
    ColumnSchema *columns;
    size_t column_count;
} TableSchema;

int load_schema(const char *schema_dir, const char *table_name, TableSchema *out_schema, SqlError *error);
int find_column_index(const TableSchema *schema, const char *column_name);
void free_schema(TableSchema *schema);

#endif
