/*
 * 파일 목적:
 *   SQL 원문 문자열을 토큰 배열(TokenArray)로 바꾸는 tokenizer를 구현한다.
 *
 * 전체 흐름에서 위치:
 *   main이 SQL 파일 내용을 읽은 직후 호출하는 첫 번째 문법 처리 단계다.
 *
 * 주요 내용:
 *   - keyword / identifier / literal / operator 토큰 분리
 *   - 줄 번호, 열 번호 기록
 *   - 문자열 리터럴 제한 검사
 *
 * 초보자 포인트:
 *   tokenizer는 아직 SQL 의미를 해석하지 않는다.
 *   단지 "글자를 어떤 종류의 조각(token)으로 나눌지"만 결정한다.
 *
 * 메모리 / 문자열 주의점:
 *   토큰마다 lexeme 복사본을 heap에 저장하므로,
 *   실패 시 중간에 만든 토큰도 free_token_array로 정리해야 한다.
 */

#include "tokenizer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

static int is_keyword(const char *text);
static int push_token(
    TokenArray *tokens,
    TokenType type,
    char *owned_lexeme,
    int line,
    int column,
    SqlError *error
);

/*
 * 목적:
 *   현재 문자열이 지원하는 SQL keyword인지 검사한다.
 *
 * 입력:
 *   text - 검사할 문자열.
 *
 * 반환:
 *   keyword면 1, 아니면 0.
 *
 * 동작 요약:
 *   이 프로젝트에서 지원하는 여섯 개 keyword와 대소문자 무시 비교를 한다.
 *
 * 초보자 주의:
 *   tokenizer 단계에서 keyword와 identifier를 구분해 두면 parser 코드가 단순해진다.
 */
static int is_keyword(const char *text) {
    return sql_case_equals(text, "INSERT") || sql_case_equals(text, "INTO") ||
           sql_case_equals(text, "VALUES") || sql_case_equals(text, "SELECT") ||
           sql_case_equals(text, "FROM") || sql_case_equals(text, "WHERE");
}

/*
 * 목적:
 *   TokenArray 끝에 새 토큰 하나를 추가한다.
 *
 * 입력:
 *   tokens - 토큰 배열.
 *   type - 추가할 토큰 종류.
 *   owned_lexeme - 이미 할당된 lexeme 문자열 소유권.
 *   line, column - 토큰 시작 위치.
 *   error - 메모리 실패 시 메시지를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   필요 시 배열을 realloc으로 늘린 뒤 새 Token을 끝에 기록한다.
 *
 * 초보자 주의:
 *   realloc 실패 시 기존 배열은 그대로 살아 있으므로,
 *   새 포인터를 임시 변수에 받아 성공했을 때만 교체해야 안전하다.
 */
static int push_token(
    TokenArray *tokens,
    TokenType type,
    char *owned_lexeme,
    int line,
    int column,
    SqlError *error
) {
    Token *resized_items;
    size_t new_capacity;

    if (tokens->count == tokens->capacity) {
        new_capacity = tokens->capacity == 0 ? 16 : tokens->capacity * 2;
        resized_items = (Token *)realloc(tokens->items, new_capacity * sizeof(Token));
        if (resized_items == NULL) {
            free(owned_lexeme);
            sql_error_set(error, "ERROR: out of memory while growing token array");
            return 0;
        }

        tokens->items = resized_items;
        tokens->capacity = new_capacity;
    }

    tokens->items[tokens->count].type = type;
    tokens->items[tokens->count].lexeme = owned_lexeme;
    tokens->items[tokens->count].line = line;
    tokens->items[tokens->count].column = column;
    tokens->count += 1;
    return 1;
}

/*
 * 목적:
 *   SQL 원문 전체를 토큰 배열로 나눈다.
 *
 * 입력:
 *   sql_text - 파일에서 읽은 SQL 전체 문자열.
 *   out_tokens - 성공 시 결과 토큰 배열을 받을 포인터.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   문자열을 왼쪽에서 오른쪽으로 읽으며 공백은 건너뛰고,
 *   각 글자 패턴에 맞춰 토큰을 만든다.
 *
 * 초보자 주의:
 *   line / column을 같이 기록해 두면 문법 오류가 났을 때 훨씬 친절한 메시지를 만들 수 있다.
 */
int tokenize_sql(const char *sql_text, TokenArray *out_tokens, SqlError *error) {
    size_t index;
    int line;
    int column;

    if (sql_text == NULL || out_tokens == NULL) {
        sql_error_set(error, "ERROR: tokenizer received invalid input");
        return 0;
    }

    out_tokens->items = NULL;
    out_tokens->count = 0;
    out_tokens->capacity = 0;

    index = 0;
    line = 1;
    column = 1;

    while (sql_text[index] != '\0') {
        char current = sql_text[index];
        int token_line = line;
        int token_column = column;

        if (isspace((unsigned char)current)) {
            if (current == '\n') {
                line += 1;
                column = 1;
            } else {
                column += 1;
            }
            index += 1;
            continue;
        }

        if (sql_is_identifier_start(current)) {
            size_t start = index;
            char *raw;
            char *normalized;

            while (sql_is_identifier_part(sql_text[index])) {
                index += 1;
                column += 1;
            }

            raw = sql_strndup(sql_text + start, index - start);
            if (raw == NULL) {
                sql_error_set(error, "ERROR: out of memory while reading identifier");
                free_token_array(out_tokens);
                return 0;
            }

            if (is_keyword(raw)) {
                normalized = sql_uppercase_copy(raw);
                free(raw);
                raw = normalized;
                if (!push_token(out_tokens, TOKEN_KEYWORD, raw, token_line, token_column, error)) {
                    free_token_array(out_tokens);
                    return 0;
                }
            } else {
                normalized = sql_lowercase_copy(raw);
                free(raw);
                raw = normalized;
                if (!push_token(out_tokens, TOKEN_IDENTIFIER, raw, token_line, token_column, error)) {
                    free_token_array(out_tokens);
                    return 0;
                }
            }

            continue;
        }

        if (current == '\'' ) {
            size_t start;
            char *text;

            index += 1;
            column += 1;
            start = index;

            while (sql_text[index] != '\0' && sql_text[index] != '\'') {
                if (sql_text[index] == '\n') {
                    sql_error_set(error, "ERROR: string literal cannot contain newline at line %d", token_line);
                    free_token_array(out_tokens);
                    return 0;
                }
                if (sql_text[index] == ',') {
                    sql_error_set(error, "ERROR: TEXT literal cannot contain comma");
                    free_token_array(out_tokens);
                    return 0;
                }
                index += 1;
                column += 1;
            }

            if (sql_text[index] != '\'') {
                sql_error_set(error, "ERROR: unterminated string literal at line %d, column %d", token_line, token_column);
                free_token_array(out_tokens);
                return 0;
            }

            text = sql_strndup(sql_text + start, index - start);
            if (text == NULL) {
                sql_error_set(error, "ERROR: out of memory while reading string literal");
                free_token_array(out_tokens);
                return 0;
            }

            index += 1;
            column += 1;

            if (!push_token(out_tokens, TOKEN_STRING_LITERAL, text, token_line, token_column, error)) {
                free_token_array(out_tokens);
                return 0;
            }

            continue;
        }

        if (isdigit((unsigned char)current) || (current == '-' && isdigit((unsigned char)sql_text[index + 1]))) {
            size_t start = index;
            char *number;

            index += 1;
            column += 1;
            while (isdigit((unsigned char)sql_text[index])) {
                index += 1;
                column += 1;
            }

            number = sql_strndup(sql_text + start, index - start);
            if (number == NULL) {
                sql_error_set(error, "ERROR: out of memory while reading integer literal");
                free_token_array(out_tokens);
                return 0;
            }

            if (!push_token(out_tokens, TOKEN_INT_LITERAL, number, token_line, token_column, error)) {
                free_token_array(out_tokens);
                return 0;
            }

            continue;
        }

        if (current == ',') {
            if (!push_token(out_tokens, TOKEN_COMMA, sql_strdup(","), token_line, token_column, error)) {
                free_token_array(out_tokens);
                return 0;
            }
            index += 1;
            column += 1;
            continue;
        }

        if (current == '(') {
            if (!push_token(out_tokens, TOKEN_LPAREN, sql_strdup("("), token_line, token_column, error)) {
                free_token_array(out_tokens);
                return 0;
            }
            index += 1;
            column += 1;
            continue;
        }

        if (current == ')') {
            if (!push_token(out_tokens, TOKEN_RPAREN, sql_strdup(")"), token_line, token_column, error)) {
                free_token_array(out_tokens);
                return 0;
            }
            index += 1;
            column += 1;
            continue;
        }

        if (current == ';') {
            if (!push_token(out_tokens, TOKEN_SEMICOLON, sql_strdup(";"), token_line, token_column, error)) {
                free_token_array(out_tokens);
                return 0;
            }
            index += 1;
            column += 1;
            continue;
        }

        if (current == '*') {
            if (!push_token(out_tokens, TOKEN_ASTERISK, sql_strdup("*"), token_line, token_column, error)) {
                free_token_array(out_tokens);
                return 0;
            }
            index += 1;
            column += 1;
            continue;
        }

        if (current == '!' || current == '<' || current == '>' || current == '=') {
            char operator_buffer[3];

            operator_buffer[0] = current;
            operator_buffer[1] = '\0';
            operator_buffer[2] = '\0';

            if ((current == '!' || current == '<' || current == '>') && sql_text[index + 1] == '=') {
                operator_buffer[1] = '=';
                operator_buffer[2] = '\0';
                index += 2;
                column += 2;
            } else if (current == '=') {
                index += 1;
                column += 1;
            } else if (current == '<' || current == '>') {
                index += 1;
                column += 1;
            } else {
                sql_error_set(error, "ERROR: unsupported operator starting with ! at line %d, column %d", token_line, token_column);
                free_token_array(out_tokens);
                return 0;
            }

            if (!push_token(out_tokens, TOKEN_OPERATOR, sql_strdup(operator_buffer), token_line, token_column, error)) {
                free_token_array(out_tokens);
                return 0;
            }
            continue;
        }

        sql_error_set(error, "ERROR: unexpected character '%c' at line %d, column %d", current, token_line, token_column);
        free_token_array(out_tokens);
        return 0;
    }

    if (!push_token(out_tokens, TOKEN_EOF, sql_strdup("EOF"), line, column, error)) {
        free_token_array(out_tokens);
        return 0;
    }

    return 1;
}
