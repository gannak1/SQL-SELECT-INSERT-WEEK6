/*
 * 파일 목적:
 *   tokenizer가 SQL 문자열을 올바른 토큰 시퀀스로 나누는지 검증한다.
 *
 * 전체 흐름에서 위치:
 *   가장 앞단의 문법 처리기가 안정적인지 빠르게 확인하는 단위 테스트다.
 *
 * 주요 내용:
 *   - SELECT + WHERE 토큰화
 *   - 음수 정수 / 문자열 literal 처리
 *   - 쉼표가 들어간 TEXT literal 거부
 *
 * 초보자 포인트:
 *   tokenizer가 흔들리면 parser와 executor도 연쇄적으로 오작동하기 때문에,
 *   가장 먼저 작은 문자열 케이스를 검사해두는 것이 좋다.
 *
 * 메모리 / 문자열 주의점:
 *   성공한 tokenize 결과는 각 테스트가 끝날 때 free_token_array로 정리한다.
 */

#include "test_helpers.h"

#include <string.h>

#include "error.h"
#include "token.h"
#include "tokenizer.h"

static void test_select_where_tokenization(TestContext *context);
static void test_insert_negative_and_text_literals(TestContext *context);
static void test_rejects_comma_inside_text_literal(TestContext *context);

/*
 * 목적:
 *   SELECT ... WHERE ... 문장이 기대한 토큰 순서로 나뉘는지 확인한다.
 *
 * 입력:
 *   context - assertion 개수를 기록할 테스트 컨텍스트.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   sample SELECT 문자열을 tokenize한 뒤 토큰 타입과 lexeme을 몇 개 골라 검사한다.
 *
 * 초보자 주의:
 *   전체 토큰 수와 중간 연산자/리터럴 위치를 함께 확인하면 tokenizer 회귀를 잘 잡을 수 있다.
 */
static void test_select_where_tokenization(TestContext *context) {
    const char *sql;
    TokenArray tokens;
    SqlError error;

    sql = "SELECT id, name FROM users WHERE age >= 20;";
    tokens.items = NULL;
    tokens.count = 0;
    tokens.capacity = 0;
    sql_error_clear(&error);

    TEST_ASSERT(context, tokenize_sql(sql, &tokens, &error), error.message);
    if (tokens.count == 0) {
        return;
    }

    TEST_ASSERT_INT_EQ(context, 12, tokens.count, "SELECT WHERE token count should include EOF");
    TEST_ASSERT_INT_EQ(context, TOKEN_KEYWORD, tokens.items[0].type, "first token should be SELECT keyword");
    TEST_ASSERT_STR_EQ(context, "SELECT", tokens.items[0].lexeme, "keyword should be normalized to uppercase");
    TEST_ASSERT_INT_EQ(context, TOKEN_IDENTIFIER, tokens.items[1].type, "second token should be identifier");
    TEST_ASSERT_STR_EQ(context, "id", tokens.items[1].lexeme, "identifier should be normalized to lowercase");
    TEST_ASSERT_STR_EQ(context, ">=", tokens.items[8].lexeme, "operator token should keep original operator");
    TEST_ASSERT_STR_EQ(context, "20", tokens.items[9].lexeme, "integer literal should be captured");
    TEST_ASSERT_INT_EQ(context, TOKEN_EOF, tokens.items[11].type, "last token should be EOF");

    free_token_array(&tokens);
}

/*
 * 목적:
 *   INSERT 문에서 음수 정수와 문자열 literal이 올바르게 토큰화되는지 확인한다.
 *
 * 입력:
 *   context - assertion 개수를 기록할 테스트 컨텍스트.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   INSERT 예시를 tokenize한 뒤 -7과 Alice가 각각 INT / STRING 토큰인지 검사한다.
 *
 * 초보자 주의:
 *   음수 기호 '-'를 연산자가 아니라 정수 literal 일부로 읽는지가 중요하다.
 */
static void test_insert_negative_and_text_literals(TestContext *context) {
    const char *sql;
    TokenArray tokens;
    SqlError error;

    sql = "INSERT INTO users VALUES (-7, 'Alice', 30);";
    tokens.items = NULL;
    tokens.count = 0;
    tokens.capacity = 0;
    sql_error_clear(&error);

    TEST_ASSERT(context, tokenize_sql(sql, &tokens, &error), error.message);
    if (tokens.count == 0) {
        return;
    }

    TEST_ASSERT_STR_EQ(context, "-7", tokens.items[5].lexeme, "negative number should stay in one token");
    TEST_ASSERT_INT_EQ(context, TOKEN_STRING_LITERAL, tokens.items[7].type, "Alice should be string literal");
    TEST_ASSERT_STR_EQ(context, "Alice", tokens.items[7].lexeme, "string literal should not include quotes");

    free_token_array(&tokens);
}

/*
 * 목적:
 *   프로젝트 제한에 따라 TEXT literal 안의 쉼표를 거부하는지 확인한다.
 *
 * 입력:
 *   context - assertion 개수를 기록할 테스트 컨텍스트.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   쉼표가 들어간 문자열을 tokenize해 실패와 에러 메시지를 확인한다.
 *
 * 초보자 주의:
 *   CSV 단순화 규칙 때문에 문자열 제한도 tokenizer 단계에서 미리 막아두었다.
 */
static void test_rejects_comma_inside_text_literal(TestContext *context) {
    TokenArray tokens;
    SqlError error;

    tokens.items = NULL;
    tokens.count = 0;
    tokens.capacity = 0;
    sql_error_clear(&error);

    TEST_ASSERT(
        context,
        !tokenize_sql("INSERT INTO users VALUES (1, 'A,B', 30);", &tokens, &error),
        "tokenizer should reject comma inside TEXT literal"
    );
    TEST_ASSERT(context, sql_error_has(&error), "comma inside TEXT literal should produce error message");
    TEST_ASSERT(context, strstr(error.message, "comma") != NULL, "error message should mention comma restriction");
}

/*
 * 목적:
 *   tokenizer 단위 테스트 suite 전체를 실행한다.
 *
 * 입력:
 *   없음.
 *
 * 반환:
 *   실패 개수.
 *
 * 동작 요약:
 *   세 개의 tokenizer 테스트를 호출한 뒤 suite 결과를 출력한다.
 *
 * 초보자 주의:
 *   suite 함수는 다른 파일의 main에서 호출되므로 외부에 노출되는 entry 역할을 한다.
 */
int run_tokenizer_tests(void) {
    TestContext context;

    test_context_init(&context);
    test_select_where_tokenization(&context);
    test_insert_negative_and_text_literals(&context);
    test_rejects_comma_inside_text_literal(&context);
    return test_finish_suite("tokenizer", &context);
}
