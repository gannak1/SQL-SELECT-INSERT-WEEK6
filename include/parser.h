/*
 * 파일 목적:
 *   TokenArray를 읽어 Program 구조체로 바꾸는 parser 인터페이스를 선언한다.
 *
 * 전체 흐름에서 위치:
 *   tokenizer 다음 단계이며,
 *   executor가 이해하기 쉬운 statement 구조를 만들어주는 관문이다.
 *
 * 주요 내용:
 *   - parse_program 함수 선언
 *
 * 초보자 포인트:
 *   parser는 "이 토큰이 맞는 순서로 왔는가?"를 검사한다.
 *   즉 문법 오류를 가장 먼저 잡아내는 곳이다.
 *
 * 메모리 / 문자열 주의점:
 *   성공 시 Program 내부에 새 메모리가 채워지므로,
 *   호출자는 free_program으로 정리해야 한다.
 */

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "error.h"
#include "token.h"

int parse_program(const TokenArray *tokens, Program *out_program, SqlError *error);

#endif
