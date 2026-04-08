/*
 * 파일 목적:
 *   SQL 텍스트 또는 SQL 파일을 공통 파이프라인으로 실행하는 helper를 구현한다.
 *
 * 전체 흐름에서 위치:
 *   main의 각 입력 모드는 SQL 문자열을 직접 만들거나 파일에서 읽은 뒤,
 *   이 모듈에 실행을 위임한다.
 *
 * 주요 내용:
 *   - run_sql_text
 *   - run_sql_file
 *
 * 초보자 포인트:
 *   tokenizer, parser, executor를 한 함수로 묶어 두면
 *   "입력 확보"와 "SQL 실행"을 분리해서 생각하기 쉬워진다.
 *
 * 메모리 / 문자열 주의점:
 *   성공과 실패 모두에서 TokenArray, Program, 파일 버퍼를 정리해야 한다.
 */

#include "sql_runner.h"

#include <stdlib.h>

#include "ast.h"
#include "executor.h"
#include "file_reader.h"
#include "parser.h"
#include "token.h"
#include "tokenizer.h"

/*
 * 목적:
 *   이미 메모리에 있는 SQL 문자열을 토큰화, 파싱, 실행까지 한 번에 수행한다.
 *
 * 입력:
 *   sql_text - 실행할 SQL 전체 문자열.
 *   schema_dir - schema 디렉터리 경로.
 *   data_dir - data 디렉터리 경로.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   tokenize_sql -> parse_program -> execute_program 순서로 실행한다.
 *
 * 초보자 주의:
 *   이 함수는 결과를 직접 출력하지 않고, 정상 실행 중 필요한 출력은 executor가 한다.
 *   실패 메시지는 caller가 sql_error_print로 출력한다.
 */
int run_sql_text(const char *sql_text, const char *schema_dir, const char *data_dir, SqlError *error) {
    TokenArray tokens;
    Program program;
    int success;

    tokens.items = NULL;
    tokens.count = 0;
    tokens.capacity = 0;
    program.statements = NULL;
    program.count = 0;
    program.capacity = 0;

    if (sql_text == NULL || schema_dir == NULL || data_dir == NULL) {
        sql_error_set(error, "ERROR: SQL runner received invalid input");
        return 0;
    }

    success = tokenize_sql(sql_text, &tokens, error) &&
              parse_program(&tokens, &program, error) &&
              execute_program(&program, schema_dir, data_dir, error);

    free_token_array(&tokens);
    free_program(&program);
    return success;
}

/*
 * 목적:
 *   SQL 파일을 읽어 그 내용을 실행한다.
 *
 * 입력:
 *   path - SQL 파일 경로.
 *   schema_dir - schema 디렉터리 경로.
 *   data_dir - data 디렉터리 경로.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   read_entire_file로 SQL 파일을 읽고, run_sql_text로 공통 실행 경로에 넘긴다.
 *
 * 초보자 주의:
 *   파일 모드는 결국 "파일을 읽어 SQL 문자열을 확보하는 단계"만 추가된 것이라고 이해하면 된다.
 */
int run_sql_file(const char *path, const char *schema_dir, const char *data_dir, SqlError *error) {
    char *sql_text;
    int success;

    sql_text = NULL;

    if (!read_entire_file(path, &sql_text, error)) {
        return 0;
    }

    success = run_sql_text(sql_text, schema_dir, data_dir, error);
    free(sql_text);
    return success;
}
