/*
 * 파일 목적:
 *   parsed Program을 실제로 실행해 schema 검증, CSV I/O, WHERE 평가, projection을 수행한다.
 *
 * 전체 흐름에서 위치:
 *   parser가 구조화한 statement를 받아 저장소와 출력기 사이를 조정하는 실행 단계다.
 *
 * 주요 내용:
 *   - INSERT 실행
 *   - SELECT 실행
 *   - WHERE 타입 검사와 비교
 *   - projection 컬럼 선택
 *
 * 초보자 포인트:
 *   executor는 "문법이 맞다"를 넘어 "의미도 맞는가?"를 확인한다.
 *   예를 들어 age 컬럼에 문자열을 비교하는지, 없는 컬럼을 찾는지 같은 문제를 여기서 잡는다.
 *
 * 메모리 / 문자열 주의점:
 *   schema와 CSV 테이블은 statement마다 새로 열어 읽고 함수 끝에서 정리한다.
 *   임시 projection 배열도 출력 직후 free해야 누수가 없다.
 */

#include "executor.h"

#include <stdlib.h>
#include <string.h>

#include "csv_storage.h"
#include "printer.h"
#include "schema.h"

static int execute_insert_statement(
    const InsertStatement *statement,
    const char *schema_dir,
    const char *data_dir,
    SqlError *error
);
static int execute_select_statement(
    const SelectStatement *statement,
    const char *schema_dir,
    const char *data_dir,
    SqlError *error
);
static int build_projection(
    const SelectStatement *statement,
    const TableSchema *schema,
    size_t **out_indices,
    size_t *out_count,
    SqlError *error
);
static int validate_where_clause(
    const SelectStatement *statement,
    const TableSchema *schema,
    int *out_where_index,
    SqlError *error
);
static int row_matches_where(
    const SelectStatement *statement,
    const TableSchema *schema,
    const CsvRow *row,
    int where_index,
    int *out_matches,
    SqlError *error
);

/*
 * 목적:
 *   SELECT 결과에 필요한 projection 인덱스 배열을 만든다.
 *
 * 입력:
 *   statement - SELECT statement.
 *   schema - 대상 테이블 schema.
 *   out_indices - 성공 시 컬럼 인덱스 배열.
 *   out_count - 성공 시 컬럼 개수.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   SELECT * 이면 모든 컬럼 인덱스를 넣고,
 *   아니면 선택된 컬럼 이름을 schema에서 찾아 인덱스로 바꾼다.
 *
 * 초보자 주의:
 *   executor는 실제 데이터 row를 인덱스로 읽기 때문에,
 *   문자열 컬럼 이름을 숫자 인덱스로 바꾸는 작업이 필요하다.
 */
static int build_projection(
    const SelectStatement *statement,
    const TableSchema *schema,
    size_t **out_indices,
    size_t *out_count,
    SqlError *error
) {
    size_t *indices;
    size_t index;
    size_t count;

    if (statement->select_all) {
        count = schema->column_count;
    } else {
        count = statement->selected_column_count;
    }

    indices = (size_t *)malloc(count * sizeof(size_t));
    if (indices == NULL) {
        sql_error_set(error, "ERROR: out of memory while building projection");
        return 0;
    }

    if (statement->select_all) {
        for (index = 0; index < count; ++index) {
            indices[index] = index;
        }
    } else {
        for (index = 0; index < count; ++index) {
            int column_index = find_column_index(schema, statement->selected_columns[index]);
            if (column_index < 0) {
                free(indices);
                sql_error_set(error, "ERROR: column not found: %s", statement->selected_columns[index]);
                return 0;
            }
            indices[index] = (size_t)column_index;
        }
    }

    *out_indices = indices;
    *out_count = count;
    return 1;
}

/*
 * 목적:
 *   WHERE 절의 컬럼 존재 여부와 타입 규칙을 검사한다.
 *
 * 입력:
 *   statement - SELECT statement.
 *   schema - 대상 schema.
 *   out_where_index - 성공 시 WHERE 대상 컬럼 인덱스.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   컬럼 존재 여부, literal 타입 일치 여부, TEXT에서 허용 연산자 여부를 검사한다.
 *
 * 초보자 주의:
 *   이 단계에서 의미 오류를 잡아두면 실제 row를 읽기 전에 바로 실패시킬 수 있다.
 */
static int validate_where_clause(
    const SelectStatement *statement,
    const TableSchema *schema,
    int *out_where_index,
    SqlError *error
) {
    int where_index;
    DataType expected_type;

    if (!statement->has_where) {
        *out_where_index = -1;
        return 1;
    }

    where_index = find_column_index(schema, statement->where.column_name);
    if (where_index < 0) {
        sql_error_set(error, "ERROR: column not found: %s", statement->where.column_name);
        return 0;
    }

    expected_type = schema->columns[where_index].type;
    if (expected_type != statement->where.value.type) {
        sql_error_set(error, "ERROR: type mismatch in WHERE for column %s", statement->where.column_name);
        return 0;
    }

    if (expected_type == DATA_TYPE_TEXT &&
        statement->where.op != OP_EQ &&
        statement->where.op != OP_NEQ) {
        sql_error_set(error, "ERROR: unsupported TEXT comparison operator: %s", operator_type_name(statement->where.op));
        return 0;
    }

    *out_where_index = where_index;
    return 1;
}

/*
 * 목적:
 *   CSV row 하나가 WHERE 조건을 만족하는지 검사한다.
 *
 * 입력:
 *   statement - SELECT statement.
 *   schema - 대상 schema.
 *   row - 검사할 CSV row.
 *   where_index - WHERE 대상 컬럼 인덱스.
 *   out_matches - 성공 시 참/거짓 결과.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   INT면 strtol 후 숫자 비교, TEXT면 strcmp 후 = / != 비교를 수행한다.
 *
 * 초보자 주의:
 *   CSV는 모두 문자열로 읽히므로 INT 비교 전에는 문자열을 정수로 다시 바꿔야 한다.
 */
static int row_matches_where(
    const SelectStatement *statement,
    const TableSchema *schema,
    const CsvRow *row,
    int where_index,
    int *out_matches,
    SqlError *error
) {
    const char *cell;
    DataType column_type;

    if (!statement->has_where) {
        *out_matches = 1;
        return 1;
    }

    cell = row->values[where_index];
    column_type = schema->columns[where_index].type;

    if (column_type == DATA_TYPE_INT) {
        char *end_ptr;
        long row_value = strtol(cell, &end_ptr, 10);
        long target = statement->where.value.int_value;

        if (*end_ptr != '\0') {
            sql_error_set(error, "ERROR: invalid INT value in CSV for column %s", statement->where.column_name);
            return 0;
        }

        switch (statement->where.op) {
            case OP_EQ:
                *out_matches = row_value == target;
                return 1;
            case OP_NEQ:
                *out_matches = row_value != target;
                return 1;
            case OP_LT:
                *out_matches = row_value < target;
                return 1;
            case OP_GT:
                *out_matches = row_value > target;
                return 1;
            case OP_LTE:
                *out_matches = row_value <= target;
                return 1;
            case OP_GTE:
                *out_matches = row_value >= target;
                return 1;
            default:
                sql_error_set(error, "ERROR: unsupported operator in INT WHERE");
                return 0;
        }
    }

    if (statement->where.op == OP_EQ) {
        *out_matches = strcmp(cell, statement->where.value.text_value) == 0;
    } else {
        *out_matches = strcmp(cell, statement->where.value.text_value) != 0;
    }

    return 1;
}

/*
 * 목적:
 *   INSERT statement를 실행한다.
 *
 * 입력:
 *   statement - 실행할 INSERT 문.
 *   schema_dir - schema 디렉터리 경로.
 *   data_dir - data 디렉터리 경로.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   schema를 읽고 값 개수/타입을 검사한 뒤 CSV에 row를 append한다.
 *
 * 초보자 주의:
 *   parser가 문법만 확인했기 때문에,
 *   실제 컬럼 개수와 타입 일치는 executor에서 별도로 검사해야 한다.
 */
static int execute_insert_statement(
    const InsertStatement *statement,
    const char *schema_dir,
    const char *data_dir,
    SqlError *error
) {
    TableSchema schema;
    size_t index;

    memset(&schema, 0, sizeof(schema));

    if (!load_schema(schema_dir, statement->table_name, &schema, error)) {
        return 0;
    }

    if (statement->value_count != schema.column_count) {
        free_schema(&schema);
        sql_error_set(error, "ERROR: value count mismatch for table %s", statement->table_name);
        return 0;
    }

    for (index = 0; index < schema.column_count; ++index) {
        if (statement->values[index].type != schema.columns[index].type) {
            free_schema(&schema);
            sql_error_set(error, "ERROR: type mismatch for column %s in INSERT", schema.columns[index].name);
            return 0;
        }
    }

    if (!csv_append_row(data_dir, &schema, statement->values, statement->value_count, error)) {
        free_schema(&schema);
        return 0;
    }

    print_insert_success(statement->table_name);
    free_schema(&schema);
    return 1;
}

/*
 * 목적:
 *   SELECT statement를 실행한다.
 *
 * 입력:
 *   statement - 실행할 SELECT 문.
 *   schema_dir - schema 디렉터리 경로.
 *   data_dir - data 디렉터리 경로.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   schema와 CSV를 읽고 projection/WHERE를 적용한 뒤 결과를 출력한다.
 *
 * 초보자 주의:
 *   출력 헤더는 실제 projection 결과에 맞춰 만들어야 하므로,
 *   SELECT * 와 SELECT id,name을 분기해 다뤄야 한다.
 */
static int execute_select_statement(
    const SelectStatement *statement,
    const char *schema_dir,
    const char *data_dir,
    SqlError *error
) {
    TableSchema schema;
    CsvTable table;
    size_t *projection_indices;
    size_t projection_count;
    int where_index;
    size_t row_index;
    size_t output_row_count;

    memset(&schema, 0, sizeof(schema));
    memset(&table, 0, sizeof(table));
    projection_indices = NULL;
    projection_count = 0;
    where_index = -1;
    output_row_count = 0;

    if (!load_schema(schema_dir, statement->table_name, &schema, error)) {
        return 0;
    }

    if (!build_projection(statement, &schema, &projection_indices, &projection_count, error)) {
        free_schema(&schema);
        return 0;
    }

    if (!validate_where_clause(statement, &schema, &where_index, error)) {
        free(projection_indices);
        free_schema(&schema);
        return 0;
    }

    if (!csv_load_table(data_dir, &schema, &table, error)) {
        free(projection_indices);
        free_schema(&schema);
        return 0;
    }

    {
        char **headers;
        size_t header_index;

        headers = (char **)malloc(projection_count * sizeof(char *));
        if (headers == NULL) {
            free_csv_table(&table);
            free(projection_indices);
            free_schema(&schema);
            sql_error_set(error, "ERROR: out of memory while building output headers");
            return 0;
        }

        for (header_index = 0; header_index < projection_count; ++header_index) {
            headers[header_index] = schema.columns[projection_indices[header_index]].name;
        }

        print_result_header(statement->table_name, headers, projection_count);
        free(headers);
    }

    for (row_index = 0; row_index < table.row_count; ++row_index) {
        int matches;
        char **projected_values;
        size_t projection_index;

        if (!row_matches_where(statement, &schema, &table.rows[row_index], where_index, &matches, error)) {
            free_csv_table(&table);
            free(projection_indices);
            free_schema(&schema);
            return 0;
        }

        if (!matches) {
            continue;
        }

        projected_values = (char **)malloc(projection_count * sizeof(char *));
        if (projected_values == NULL) {
            free_csv_table(&table);
            free(projection_indices);
            free_schema(&schema);
            sql_error_set(error, "ERROR: out of memory while projecting row");
            return 0;
        }

        for (projection_index = 0; projection_index < projection_count; ++projection_index) {
            projected_values[projection_index] = table.rows[row_index].values[projection_indices[projection_index]];
        }

        print_result_row(projected_values, projection_count);
        free(projected_values);
        output_row_count += 1;
    }

    print_result_footer(output_row_count);

    free_csv_table(&table);
    free(projection_indices);
    free_schema(&schema);
    return 1;
}

/*
 * 목적:
 *   Program 안의 statement들을 순서대로 실행한다.
 *
 * 입력:
 *   program - parser가 만든 Program.
 *   schema_dir - schema 디렉터리 경로.
 *   data_dir - data 디렉터리 경로.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   각 statement 타입에 따라 INSERT 또는 SELECT 실행 함수로 위임한다.
 *
 * 초보자 주의:
 *   이 프로젝트는 첫 에러에서 바로 멈추는 단순 정책을 택한다.
 */
int execute_program(const Program *program, const char *schema_dir, const char *data_dir, SqlError *error) {
    size_t index;

    if (program == NULL || schema_dir == NULL || data_dir == NULL) {
        sql_error_set(error, "ERROR: executor received invalid input");
        return 0;
    }

    for (index = 0; index < program->count; ++index) {
        if (program->statements[index].type == STMT_INSERT) {
            if (!execute_insert_statement(&program->statements[index].as.insert, schema_dir, data_dir, error)) {
                return 0;
            }
        } else if (program->statements[index].type == STMT_SELECT) {
            if (!execute_select_statement(&program->statements[index].as.select, schema_dir, data_dir, error)) {
                return 0;
            }
        } else {
            sql_error_set(error, "ERROR: unsupported statement type in executor");
            return 0;
        }
    }

    return 1;
}
