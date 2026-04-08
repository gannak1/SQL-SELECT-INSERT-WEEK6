/*
 * 파일 목적:
 *   schema/<table>.schema 파일을 읽고 TableSchema 구조체로 변환한다.
 *
 * 전체 흐름에서 위치:
 *   executor는 INSERT와 SELECT를 실행하기 전에 항상 이 모듈을 통해 schema를 읽는다.
 *
 * 주요 내용:
 *   - schema 파일 로딩
 *   - columns=... 줄 파싱
 *   - 컬럼 이름 검색
 *
 * 초보자 포인트:
 *   SQL 자체에는 컬럼 타입 정보가 없기 때문에,
 *   schema 파일은 실행기가 타입 검사를 하는 근거가 된다.
 *
 * 메모리 / 문자열 주의점:
 *   줄 파싱 과정에서 복사 문자열을 여러 번 만들므로,
 *   실패 경로마다 free_schema를 호출해 누수를 막아야 한다.
 */

#include "schema.h"

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "file_reader.h"

static int append_column(TableSchema *schema, char *owned_name, DataType type, SqlError *error);
static int parse_type_text(const char *text, DataType *out_type);

/*
 * 목적:
 *   TableSchema 끝에 새 컬럼을 하나 추가한다.
 *
 * 입력:
 *   schema - 채우는 중인 schema 구조체.
 *   owned_name - 이미 할당된 컬럼 이름 문자열 소유권.
 *   type - 컬럼 타입.
 *   error - 메모리 실패 시 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   columns 배열을 realloc으로 늘리고 새 name/type을 저장한다.
 *
 * 초보자 주의:
 *   실패 시 owned_name을 잊지 않도록 직접 free한다.
 */
static int append_column(TableSchema *schema, char *owned_name, DataType type, SqlError *error) {
    ColumnSchema *resized_columns;
    size_t new_count;

    new_count = schema->column_count + 1;
    resized_columns = (ColumnSchema *)realloc(schema->columns, new_count * sizeof(ColumnSchema));
    if (resized_columns == NULL) {
        free(owned_name);
        sql_error_set(error, "ERROR: out of memory while growing schema columns");
        return 0;
    }

    schema->columns = resized_columns;
    schema->columns[schema->column_count].name = owned_name;
    schema->columns[schema->column_count].type = type;
    schema->column_count = new_count;
    return 1;
}

/*
 * 목적:
 *   schema 파일의 타입 문자열(INT, TEXT)을 enum으로 변환한다.
 *
 * 입력:
 *   text - 타입 문자열.
 *   out_type - 성공 시 결과 enum.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   대소문자 무시 비교로 지원 타입 두 개만 허용한다.
 *
 * 초보자 주의:
 *   지원하지 않는 타입을 조용히 무시하지 않고 바로 에러로 처리해야 디버깅이 쉽다.
 */
static int parse_type_text(const char *text, DataType *out_type) {
    if (sql_case_equals(text, "INT")) {
        *out_type = DATA_TYPE_INT;
        return 1;
    }
    if (sql_case_equals(text, "TEXT")) {
        *out_type = DATA_TYPE_TEXT;
        return 1;
    }
    return 0;
}

/*
 * 목적:
 *   schema 디렉터리에서 테이블 schema 파일을 읽어 TableSchema를 만든다.
 *
 * 입력:
 *   schema_dir - schema 디렉터리 경로.
 *   table_name - 찾을 테이블 이름.
 *   out_schema - 성공 시 결과 구조체.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   파일을 읽고 table=, columns= 줄을 찾아 파싱한 뒤 컬럼 배열을 만든다.
 *
 * 초보자 주의:
 *   요청한 table_name과 schema 파일 안의 table= 값이 다르면,
 *   파일이 잘못된 것이므로 바로 실패시키는 편이 안전하다.
 */
int load_schema(const char *schema_dir, const char *table_name, TableSchema *out_schema, SqlError *error) {
    char path[SQL_PATH_LENGTH];
    char *contents;
    char *line;
    char *saveptr;
    char *table_line_value;
    char *columns_line_value;

    if (schema_dir == NULL || table_name == NULL || out_schema == NULL) {
        sql_error_set(error, "ERROR: schema loader received invalid input");
        return 0;
    }

    memset(out_schema, 0, sizeof(*out_schema));
    contents = NULL;
    table_line_value = NULL;
    columns_line_value = NULL;

    snprintf(path, sizeof(path), "%s/%s.schema", schema_dir, table_name);
    if (!read_entire_file(path, &contents, error)) {
        return 0;
    }

    for (line = strtok_r(contents, "\n", &saveptr); line != NULL; line = strtok_r(NULL, "\n", &saveptr)) {
        char *trimmed = sql_trim_copy(line);

        if (trimmed == NULL) {
            sql_error_set(error, "ERROR: out of memory while trimming schema line");
            free(contents);
            free(table_line_value);
            free(columns_line_value);
            free_schema(out_schema);
            return 0;
        }

        if (strncmp(trimmed, "table=", 6) == 0) {
            free(table_line_value);
            table_line_value = sql_lowercase_copy(trimmed + 6);
        } else if (strncmp(trimmed, "columns=", 8) == 0) {
            free(columns_line_value);
            columns_line_value = sql_strdup(trimmed + 8);
        }

        free(trimmed);
    }

    if (table_line_value == NULL || columns_line_value == NULL) {
        sql_error_set(error, "ERROR: invalid schema format for table %s", table_name);
        free(contents);
        free(table_line_value);
        free(columns_line_value);
        free_schema(out_schema);
        return 0;
    }

    if (!sql_case_equals(table_line_value, table_name)) {
        sql_error_set(error, "ERROR: schema table name mismatch: %s", table_name);
        free(contents);
        free(table_line_value);
        free(columns_line_value);
        free_schema(out_schema);
        return 0;
    }

    out_schema->table_name = sql_strdup(table_name);
    if (out_schema->table_name == NULL) {
        sql_error_set(error, "ERROR: out of memory while copying schema table name");
        free(contents);
        free(table_line_value);
        free(columns_line_value);
        return 0;
    }

    {
        char *columns_copy = columns_line_value;
        char *column_entry;
        char *column_saveptr;

        for (column_entry = strtok_r(columns_copy, ",", &column_saveptr);
             column_entry != NULL;
             column_entry = strtok_r(NULL, ",", &column_saveptr)) {
            char *colon;
            char *name_copy;
            char *type_copy;
            char *normalized_name;
            DataType type;

            colon = strchr(column_entry, ':');
            if (colon == NULL) {
                sql_error_set(error, "ERROR: invalid schema column definition: %s", column_entry);
                free(contents);
                free(table_line_value);
                free(columns_line_value);
                free_schema(out_schema);
                return 0;
            }

            *colon = '\0';
            name_copy = sql_trim_copy(column_entry);
            type_copy = sql_trim_copy(colon + 1);
            normalized_name = sql_lowercase_copy(name_copy);
            free(name_copy);

            if (type_copy == NULL || normalized_name == NULL) {
                free(type_copy);
                free(normalized_name);
                sql_error_set(error, "ERROR: out of memory while parsing schema columns");
                free(contents);
                free(table_line_value);
                free(columns_line_value);
                free_schema(out_schema);
                return 0;
            }

            if (!parse_type_text(type_copy, &type)) {
                free(type_copy);
                free(normalized_name);
                sql_error_set(error, "ERROR: unsupported schema type: %s", colon + 1);
                free(contents);
                free(table_line_value);
                free(columns_line_value);
                free_schema(out_schema);
                return 0;
            }

            free(type_copy);

            if (!append_column(out_schema, normalized_name, type, error)) {
                free(contents);
                free(table_line_value);
                free(columns_line_value);
                free_schema(out_schema);
                return 0;
            }
        }
    }

    if (out_schema->column_count == 0) {
        sql_error_set(error, "ERROR: schema has no columns: %s", table_name);
        free(contents);
        free(table_line_value);
        free(columns_line_value);
        free_schema(out_schema);
        return 0;
    }

    free(contents);
    free(table_line_value);
    free(columns_line_value);
    return 1;
}

/*
 * 목적:
 *   schema에서 특정 컬럼 이름의 인덱스를 찾는다.
 *
 * 입력:
 *   schema - 검색 대상 schema.
 *   column_name - 찾을 컬럼 이름.
 *
 * 반환:
 *   찾으면 0 이상 인덱스, 없으면 -1.
 *
 * 동작 요약:
 *   columns 배열을 선형 탐색한다.
 *
 * 초보자 주의:
 *   현재 프로젝트는 작은 데모이므로 선형 탐색으로도 충분하다.
 */
int find_column_index(const TableSchema *schema, const char *column_name) {
    size_t index;

    if (schema == NULL || column_name == NULL) {
        return -1;
    }

    for (index = 0; index < schema->column_count; ++index) {
        if (strcmp(schema->columns[index].name, column_name) == 0) {
            return (int)index;
        }
    }

    return -1;
}

/*
 * 목적:
 *   TableSchema가 소유한 메모리를 모두 해제한다.
 *
 * 입력:
 *   schema - 정리할 schema 구조체.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   각 컬럼 이름 문자열과 columns 배열, table_name을 순서대로 free한다.
 *
 * 초보자 주의:
 *   구조체를 0으로 초기화해 두면 부분 생성 후 실패한 경우에도 안전하게 호출할 수 있다.
 */
void free_schema(TableSchema *schema) {
    size_t index;

    if (schema == NULL) {
        return;
    }

    for (index = 0; index < schema->column_count; ++index) {
        free(schema->columns[index].name);
    }

    free(schema->columns);
    free(schema->table_name);

    schema->columns = NULL;
    schema->table_name = NULL;
    schema->column_count = 0;
}
