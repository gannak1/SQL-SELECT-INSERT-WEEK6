/*
 * 파일 목적:
 *   SQL 문자열을 토큰 배열로 바꾸는 tokenizer 함수의 인터페이스를 제공한다.
 *
 * 전체 흐름에서 위치:
 *   main이 SQL 파일을 읽은 직후 이 함수를 호출하고,
 *   parser는 그 결과를 입력으로 받는다.
 *
 * 주요 내용:
 *   - tokenize_sql 함수 선언
 *
 * 초보자 포인트:
 *   tokenizer는 "문장의 의미"를 아직 모른다.
 *   지금 단계에서는 글자를 종류별 토큰으로만 나눈다.
 *
 * 메모리 / 문자열 주의점:
 *   성공하면 out_tokens 안에 새로 할당된 lexeme들이 들어가므로,
 *   호출자는 free_token_array로 정리해야 한다.
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "error.h"
#include "token.h"

int tokenize_sql(const char *sql_text, TokenArray *out_tokens, SqlError *error);

#endif
