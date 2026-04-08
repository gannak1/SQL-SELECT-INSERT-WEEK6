/*
 * 파일 목적:
 *   TokenArray 해제와 토큰 종류 문자열 변환처럼 토큰 공통 유틸리티를 구현한다.
 *
 * 전체 흐름에서 위치:
 *   tokenizer가 만든 토큰 배열을 parser 이후 정리할 때 사용된다.
 *
 * 주요 내용:
 *   - token_type_name
 *   - free_token_array
 *
 * 초보자 포인트:
 *   배열 안에 문자열 포인터가 들어 있으면,
 *   배열만 free해서는 안 되고 각 원소의 내부 메모리도 함께 해제해야 한다.
 *
 * 메모리 / 문자열 주의점:
 *   free_token_array는 count만큼 순회하므로,
 *   tokenizer가 count를 정확히 관리해야 안전하다.
 */

#include "token.h"

#include <stdlib.h>

/*
 * 목적:
 *   디버깅이나 에러 메시지에서 토큰 종류를 읽기 쉽게 보여준다.
 *
 * 입력:
 *   type - 문자열로 바꿀 TokenType 값.
 *
 * 반환:
 *   토큰 종류 이름 문자열 상수.
 *
 * 동작 요약:
 *   switch 문으로 각 enum 값을 고정 문자열에 대응시킨다.
 *
 * 초보자 주의:
 *   default를 두면 예상하지 못한 값이 들어와도 최소한 메시지는 남길 수 있다.
 */
const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD:
            return "KEYWORD";
        case TOKEN_IDENTIFIER:
            return "IDENTIFIER";
        case TOKEN_INT_LITERAL:
            return "INT_LITERAL";
        case TOKEN_STRING_LITERAL:
            return "STRING_LITERAL";
        case TOKEN_COMMA:
            return "COMMA";
        case TOKEN_LPAREN:
            return "LPAREN";
        case TOKEN_RPAREN:
            return "RPAREN";
        case TOKEN_SEMICOLON:
            return "SEMICOLON";
        case TOKEN_OPERATOR:
            return "OPERATOR";
        case TOKEN_ASTERISK:
            return "ASTERISK";
        case TOKEN_EOF:
            return "EOF";
        default:
            return "UNKNOWN_TOKEN";
    }
}

/*
 * 목적:
 *   TokenArray와 그 안의 lexeme 문자열을 모두 해제한다.
 *
 * 입력:
 *   tokens - 정리할 토큰 배열.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   각 토큰의 lexeme을 free한 뒤 items 배열도 free하고 구조체를 0으로 비운다.
 *
 * 초보자 주의:
 *   해제 후 count와 capacity를 0으로 돌려놓으면 이중 해제 위험을 줄일 수 있다.
 */
void free_token_array(TokenArray *tokens) {
    size_t index;

    if (tokens == NULL) {
        return;
    }

    for (index = 0; index < tokens->count; ++index) {
        free(tokens->items[index].lexeme);
    }

    free(tokens->items);
    tokens->items = NULL;
    tokens->count = 0;
    tokens->capacity = 0;
}
