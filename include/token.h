/*
 * 파일 목적:
 *   tokenizer가 만들어내는 토큰의 종류와 토큰 배열 자료구조를 정의한다.
 *
 * 전체 흐름에서 위치:
 *   SQL 원문은 먼저 TokenArray로 바뀌고,
 *   parser는 이 토큰 배열을 왼쪽에서 오른쪽으로 읽어 statement를 만든다.
 *
 * 주요 내용:
 *   - 토큰 종류 enum
 *   - 토큰 구조체
 *   - 토큰 배열 해제 함수
 *
 * 초보자 포인트:
 *   parser가 문자열 전체를 직접 해석하지 않고 토큰 배열을 읽으면,
 *   문법 처리가 훨씬 단순하고 설명 가능해진다.
 *
 * 메모리 / 문자열 주의점:
 *   각 토큰의 lexeme은 동적 할당된 문자열이므로 배열 해제 시 함께 free해야 한다.
 */

#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_COMMA,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_SEMICOLON,
    TOKEN_OPERATOR,
    TOKEN_ASTERISK,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    int line;
    int column;
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} TokenArray;

const char *token_type_name(TokenType type);
void free_token_array(TokenArray *tokens);

#endif
