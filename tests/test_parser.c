/*
 * 파일 목적:
 *   parser가 토큰 배열을 올바른 Program / Statement 구조로 바꾸는지 검증한다.
 *
 * 전체 흐름에서 위치:
 *   tokenizer 다음 단계에서 구조화가 제대로 되는지를 확인하는 단위 테스트다.
 *
 * 주요 내용:
 *   - INSERT + SELECT 복합 프로그램 파싱
 *   - WHERE 조건 구조화
 *   - 세미콜론 누락 문법 오류
 *
 * 초보자 포인트:
 *   parser 테스트는 "토큰이 맞다"가 아니라 "의미 있는 구조체가 만들어졌는가"를 본다.
 *
 * 메모리 / 문자열 주의점:
 *   tokenize와 parse가 성공하면 tokens와 program 둘 다 별도로 정리해야 한다.
 */

#include "test_helpers.h"

#include <string.h>

#include "ast.h"
#include "error.h"
#include "parser.h"
#include "token.h"
#include "tokenizer.h"

static int parse_sql_text(const char *sql, TokenArray *tokens, Program *program, SqlError *error);
static void test_parses_insert_and_select_program(TestContext *context);
static void test_reports_missing_semicolon(TestContext *context);

/*
 * 목적:
 *   문자열 SQL을 tokenize와 parse까지 한 번에 수행하는 작은 helper다.
 *
 * 입력:
 *   sql - 파싱할 SQL 문자열.
 *   tokens - 성공 시 토큰 배열.
 *   program - 성공 시 Program 구조체.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   tokenize_sql 성공 후 parse_program까지 이어 호출한다.
 *
 * 초보자 주의:
 *   parser 테스트라고 해도 입력은 토큰 배열이므로 tokenizer 단계가 먼저 필요하다.
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
 *   INSERT와 SELECT가 한 파일 안에 함께 있을 때 Program 구조가 제대로 만들어지는지 확인한다.
 *
 * 입력:
 *   context - assertion 개수를 기록할 테스트 컨텍스트.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   두 statement를 가진 SQL 문자열을 파싱한 뒤 구조체 필드를 하나씩 검사한다.
 *
 * 초보자 주의:
 *   parser가 여러 문장을 읽는지 확인하려면 statement count를 꼭 검사해야 한다.
 */
static void test_parses_insert_and_select_program(TestContext *context) {
    const char *sql;
    TokenArray tokens;
    Program program;
    SqlError error;

    sql =
        "INSERT INTO users VALUES (1, 'Alice', 30);"
        "SELECT id, name FROM users WHERE age >= 20;";

    TEST_ASSERT(context, parse_sql_text(sql, &tokens, &program, &error), error.message);
    if (program.count < 2) {
        free_token_array(&tokens);
        free_program(&program);
        return;
    }

    TEST_ASSERT_INT_EQ(context, 2, program.count, "program should contain two statements");
    TEST_ASSERT_INT_EQ(context, STMT_INSERT, program.statements[0].type, "first statement should be INSERT");
    TEST_ASSERT_STR_EQ(
        context,
        "users",
        program.statements[0].as.insert.table_name,
        "INSERT table name should be captured"
    );
    TEST_ASSERT_INT_EQ(context, 3, program.statements[0].as.insert.value_count, "INSERT should contain three values");
    TEST_ASSERT_INT_EQ(
        context,
        DATA_TYPE_TEXT,
        program.statements[0].as.insert.values[1].type,
        "second INSERT value should be TEXT"
    );

    TEST_ASSERT_INT_EQ(context, STMT_SELECT, program.statements[1].type, "second statement should be SELECT");
    TEST_ASSERT_INT_EQ(
        context,
        2,
        program.statements[1].as.select.selected_column_count,
        "SELECT should capture two projected columns"
    );
    TEST_ASSERT(context, program.statements[1].as.select.has_where, "SELECT should include WHERE clause");
    TEST_ASSERT_STR_EQ(
        context,
        "age",
        program.statements[1].as.select.where.column_name,
        "WHERE column should be captured"
    );
    TEST_ASSERT_INT_EQ(
        context,
        OP_GTE,
        program.statements[1].as.select.where.op,
        "WHERE operator should be parsed as >="
    );

    free_token_array(&tokens);
    free_program(&program);
}

/*
 * 목적:
 *   세미콜론이 빠진 SELECT 문을 parser가 문법 오류로 거부하는지 확인한다.
 *
 * 입력:
 *   context - assertion 개수를 기록할 테스트 컨텍스트.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   tokenize는 성공하지만 parse는 실패해야 함을 확인한다.
 *
 * 초보자 주의:
 *   tokenizer와 parser는 역할이 다르므로,
 *   세미콜론 누락은 tokenizer가 아니라 parser에서 잡히는 것이 정상이다.
 */
static void test_reports_missing_semicolon(TestContext *context) {
    TokenArray tokens;
    Program program;
    SqlError error;

    tokens.items = NULL;
    tokens.count = 0;
    tokens.capacity = 0;
    program.statements = NULL;
    program.count = 0;
    program.capacity = 0;
    sql_error_clear(&error);

    TEST_ASSERT(
        context,
        tokenize_sql("SELECT * FROM users", &tokens, &error),
        "tokenizer should accept SELECT without semicolon"
    );
    TEST_ASSERT(context, !parse_program(&tokens, &program, &error), "parser should reject missing semicolon");
    TEST_ASSERT(context, strstr(error.message, "expected ';'") != NULL, "error message should mention semicolon");

    free_token_array(&tokens);
    free_program(&program);
}

/*
 * 목적:
 *   parser 단위 테스트 suite 전체를 실행한다.
 *
 * 입력:
 *   없음.
 *
 * 반환:
 *   실패 개수.
 *
 * 동작 요약:
 *   두 개의 parser 테스트를 실행하고 결과를 요약 출력한다.
 *
 * 초보자 주의:
 *   parser suite가 통과하면 이후 executor 테스트에서 AST를 믿고 사용할 수 있다.
 */
int run_parser_tests(void) {
    TestContext context;

    test_context_init(&context);
    test_parses_insert_and_select_program(&context);
    test_reports_missing_semicolon(&context);
    return test_finish_suite("parser", &context);
}
