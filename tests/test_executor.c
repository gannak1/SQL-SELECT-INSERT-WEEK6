/*
 * 파일 목적:
 *   schema / CSV storage / executor의 핵심 동작과 의미 오류 처리를 검증한다.
 *
 * 전체 흐름에서 위치:
 *   parser 이후 단계에서 실제 파일 저장, schema 검증, WHERE 타입 검사까지 확인하는 단위 테스트다.
 *
 * 주요 내용:
 *   - schema 로딩과 CSV round-trip
 *   - WHERE 타입 불일치 에러
 *   - TEXT 부등호 비교 미지원 에러
 *
 * 초보자 포인트:
 *   executor 테스트는 "문법이 맞는 SQL"이 실제 데이터와 어떻게 연결되는지 확인해준다.
 *
 * 메모리 / 문자열 주의점:
 *   테스트용 임시 디렉터리와 파일을 직접 만들기 때문에,
 *   각 테스트가 끝나면 구조체 메모리도 함께 정리해야 한다.
 */

#include "test_helpers.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ast.h"
#include "csv_storage.h"
#include "error.h"
#include "executor.h"
#include "parser.h"
#include "schema.h"
#include "token.h"
#include "tokenizer.h"

static int ensure_directory(const char *path);
static int write_text_file(const char *path, const char *contents);
static int prepare_runtime_dirs(
    const char *name,
    char *schema_dir,
    size_t schema_dir_size,
    char *data_dir,
    size_t data_dir_size
);
static int parse_sql_text(const char *sql, TokenArray *tokens, Program *program, SqlError *error);
static void test_schema_and_csv_round_trip(TestContext *context);
static void test_execute_where_type_mismatch(TestContext *context);
static void test_execute_unsupported_text_operator(TestContext *context);

/*
 * 목적:
 *   디렉터리가 없으면 만들고, 이미 있으면 그대로 통과시킨다.
 *
 * 입력:
 *   path - 만들 디렉터리 경로.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   mkdir 결과가 EEXIST면 정상으로 간주한다.
 *
 * 초보자 주의:
 *   테스트는 여러 번 반복 실행될 수 있으므로 기존 디렉터리도 허용해야 편하다.
 */
static int ensure_directory(const char *path) {
    if (mkdir(path, 0777) == 0) {
        return 1;
    }
    return errno == EEXIST;
}

/*
 * 목적:
 *   테스트용 텍스트 파일을 간단히 작성한다.
 *
 * 입력:
 *   path - 쓸 파일 경로.
 *   contents - 파일 내용 문자열.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   fopen("w") 후 fputs로 내용을 기록한다.
 *
 * 초보자 주의:
 *   schema 파일처럼 작은 고정 문자열은 테스트 안에서 직접 만드는 편이 이해하기 쉽다.
 */
static int write_text_file(const char *path, const char *contents) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }
    fputs(contents, file);
    return fclose(file) == 0;
}

/*
 * 목적:
 *   suite별 테스트용 schema/data 디렉터리를 준비한다.
 *
 * 입력:
 *   name - 하위 디렉터리 이름.
 *   schema_dir, data_dir - 결과 경로를 받을 버퍼.
 *   schema_dir_size, data_dir_size - 버퍼 길이.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   tests/tmp/<name>/schema 와 data 디렉터리를 만든다.
 *
 * 초보자 주의:
 *   실제 프로젝트의 schema/data를 건드리지 않도록 테스트용 별도 경로를 쓴다.
 */
static int prepare_runtime_dirs(
    const char *name,
    char *schema_dir,
    size_t schema_dir_size,
    char *data_dir,
    size_t data_dir_size
) {
    char base_dir[SQL_PATH_LENGTH];
    char cleanup_path[SQL_PATH_LENGTH];

    if (!ensure_directory("tests")) {
        return 0;
    }
    if (!ensure_directory("tests/tmp")) {
        return 0;
    }

    snprintf(base_dir, sizeof(base_dir), "tests/tmp/%s", name);
    if (!ensure_directory(base_dir)) {
        return 0;
    }

    snprintf(schema_dir, schema_dir_size, "%s/schema", base_dir);
    snprintf(data_dir, data_dir_size, "%s/data", base_dir);

    if (!ensure_directory(schema_dir) || !ensure_directory(data_dir)) {
        return 0;
    }

    snprintf(cleanup_path, sizeof(cleanup_path), "%s/users.schema", schema_dir);
    remove(cleanup_path);
    snprintf(cleanup_path, sizeof(cleanup_path), "%s/users.csv", data_dir);
    remove(cleanup_path);

    return 1;
}

/*
 * 목적:
 *   문자열 SQL을 tokenize와 parse까지 수행하는 helper다.
 *
 * 입력:
 *   sql - 파싱할 SQL 문자열.
 *   tokens - 성공 시 토큰 배열.
 *   program - 성공 시 Program.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   tokenizer와 parser를 순서대로 호출한다.
 *
 * 초보자 주의:
 *   executor 테스트도 parsed Program을 입력으로 받기 때문에 이런 helper가 편하다.
 */
static int parse_sql_text(const char *sql, TokenArray *tokens, Program *program, SqlError *error) {
    tokens->items = NULL;
    tokens->count = 0;
    tokens->capacity = 0;
    program->statements = NULL;
    program->count = 0;
    program->capacity = 0;
    sql_error_clear(error);

    if (!tokenize_sql(sql, tokens, error)) {
        return 0;
    }

    if (!parse_program(tokens, program, error)) {
        return 0;
    }

    return 1;
}

/*
 * 목적:
 *   schema를 읽고 CSV append/load가 정상 round-trip 되는지 확인한다.
 *
 * 입력:
 *   context - assertion 개수를 기록할 테스트 컨텍스트.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   임시 schema를 만들고 두 행을 저장한 뒤 다시 읽어 값이 유지되는지 검사한다.
 *
 * 초보자 주의:
 *   storage 테스트는 실제 파일을 한 번 썼다가 다시 읽는 round-trip이 가장 확실하다.
 */
static void test_schema_and_csv_round_trip(TestContext *context) {
    char schema_dir[SQL_PATH_LENGTH];
    char data_dir[SQL_PATH_LENGTH];
    char schema_path[SQL_PATH_LENGTH];
    TableSchema schema;
    CsvTable table;
    SqlError error;
    LiteralValue row1[3];
    LiteralValue row2[3];

    memset(&schema, 0, sizeof(schema));
    memset(&table, 0, sizeof(table));
    sql_error_clear(&error);

    TEST_ASSERT(context, prepare_runtime_dirs("storage_round_trip", schema_dir, sizeof(schema_dir), data_dir, sizeof(data_dir)), "should prepare runtime directories");
    snprintf(schema_path, sizeof(schema_path), "%s/users.schema", schema_dir);
    TEST_ASSERT(
        context,
        write_text_file(schema_path, "table=users\ncolumns=id:INT,name:TEXT,age:INT\n"),
        "should write users schema"
    );

    TEST_ASSERT(context, load_schema(schema_dir, "users", &schema, &error), error.message);
    TEST_ASSERT_INT_EQ(context, 3, schema.column_count, "users schema should contain three columns");

    memset(row1, 0, sizeof(row1));
    row1[0].type = DATA_TYPE_INT;
    row1[0].int_value = 1;
    row1[1].type = DATA_TYPE_TEXT;
    row1[1].text_value = "Alice";
    row1[2].type = DATA_TYPE_INT;
    row1[2].int_value = 30;

    memset(row2, 0, sizeof(row2));
    row2[0].type = DATA_TYPE_INT;
    row2[0].int_value = 2;
    row2[1].type = DATA_TYPE_TEXT;
    row2[1].text_value = "Bob";
    row2[2].type = DATA_TYPE_INT;
    row2[2].int_value = 24;

    TEST_ASSERT(context, csv_append_row(data_dir, &schema, row1, 3, &error), error.message);
    TEST_ASSERT(context, csv_append_row(data_dir, &schema, row2, 3, &error), error.message);
    TEST_ASSERT(context, csv_load_table(data_dir, &schema, &table, &error), error.message);

    TEST_ASSERT_INT_EQ(context, 2, table.row_count, "CSV should contain two stored rows");
    TEST_ASSERT_STR_EQ(context, "Alice", table.rows[0].values[1], "first row name should be Alice");
    TEST_ASSERT_STR_EQ(context, "24", table.rows[1].values[2], "second row age should be 24");

    free_csv_table(&table);
    free_schema(&schema);
}

/*
 * 목적:
 *   age INT 컬럼에 TEXT literal을 비교할 때 의미 오류를 반환하는지 확인한다.
 *
 * 입력:
 *   context - assertion 개수를 기록할 테스트 컨텍스트.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   WHERE age = 'Alice' SQL을 실행해 non-success와 에러 메시지를 검사한다.
 *
 * 초보자 주의:
 *   parser는 문법상 허용해도 executor는 schema를 보고 타입 불일치를 거부해야 한다.
 */
static void test_execute_where_type_mismatch(TestContext *context) {
    char schema_dir[SQL_PATH_LENGTH];
    char data_dir[SQL_PATH_LENGTH];
    char schema_path[SQL_PATH_LENGTH];
    TokenArray tokens;
    Program program;
    SqlError error;

    TEST_ASSERT(context, prepare_runtime_dirs("where_type_mismatch", schema_dir, sizeof(schema_dir), data_dir, sizeof(data_dir)), "should prepare runtime directories");
    snprintf(schema_path, sizeof(schema_path), "%s/users.schema", schema_dir);
    TEST_ASSERT(
        context,
        write_text_file(schema_path, "table=users\ncolumns=id:INT,name:TEXT,age:INT\n"),
        "should write users schema"
    );

    TEST_ASSERT(
        context,
        parse_sql_text("SELECT * FROM users WHERE age = 'Alice';", &tokens, &program, &error),
        error.message
    );
    TEST_ASSERT(
        context,
        !execute_program(&program, schema_dir, data_dir, &error),
        "executor should reject WHERE type mismatch"
    );
    TEST_ASSERT_STR_EQ(
        context,
        "ERROR: type mismatch in WHERE for column age",
        error.message,
        "type mismatch error should match spec"
    );

    free_token_array(&tokens);
    free_program(&program);
}

/*
 * 목적:
 *   TEXT 컬럼에 > 비교를 사용할 때 의미 오류를 반환하는지 확인한다.
 *
 * 입력:
 *   context - assertion 개수를 기록할 테스트 컨텍스트.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   WHERE name > 'Bob' SQL을 실행해 unsupported TEXT operator 에러를 확인한다.
 *
 * 초보자 주의:
 *   요구사항상 TEXT는 = 와 != 만 허용되므로 executor가 직접 막아야 한다.
 */
static void test_execute_unsupported_text_operator(TestContext *context) {
    char schema_dir[SQL_PATH_LENGTH];
    char data_dir[SQL_PATH_LENGTH];
    char schema_path[SQL_PATH_LENGTH];
    TokenArray tokens;
    Program program;
    SqlError error;

    TEST_ASSERT(context, prepare_runtime_dirs("text_operator", schema_dir, sizeof(schema_dir), data_dir, sizeof(data_dir)), "should prepare runtime directories");
    snprintf(schema_path, sizeof(schema_path), "%s/users.schema", schema_dir);
    TEST_ASSERT(
        context,
        write_text_file(schema_path, "table=users\ncolumns=id:INT,name:TEXT,age:INT\n"),
        "should write users schema"
    );

    TEST_ASSERT(
        context,
        parse_sql_text("SELECT * FROM users WHERE name > 'Bob';", &tokens, &program, &error),
        error.message
    );
    TEST_ASSERT(
        context,
        !execute_program(&program, schema_dir, data_dir, &error),
        "executor should reject TEXT comparison with >"
    );
    TEST_ASSERT_STR_EQ(
        context,
        "ERROR: unsupported TEXT comparison operator: >",
        error.message,
        "TEXT operator error should match spec"
    );

    free_token_array(&tokens);
    free_program(&program);
}

/*
 * 목적:
 *   executor / storage 단위 테스트 suite 전체를 실행한다.
 *
 * 입력:
 *   없음.
 *
 * 반환:
 *   실패 개수.
 *
 * 동작 요약:
 *   round-trip, type mismatch, unsupported operator 세 테스트를 실행한다.
 *
 * 초보자 주의:
 *   이 suite는 stdout 출력보다 "실행 결과와 에러 규칙"에 더 초점을 둔다.
 */
int run_executor_tests(void) {
    TestContext context;

    test_context_init(&context);
    test_schema_and_csv_round_trip(&context);
    test_execute_where_type_mismatch(&context);
    test_execute_unsupported_text_operator(&context);
    return test_finish_suite("executor", &context);
}
