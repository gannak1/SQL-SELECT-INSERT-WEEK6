/*
 * 파일 목적:
 *   parser가 만든 Program을 실제로 실행하는 executor 인터페이스를 선언한다.
 *
 * 전체 흐름에서 위치:
 *   main은 parse_program 다음에 execute_program을 호출한다.
 *   이 함수 안에서 schema 검증, CSV I/O, WHERE 비교, projection, 출력이 이어진다.
 *
 * 주요 내용:
 *   - execute_program 함수 선언
 *
 * 초보자 포인트:
 *   executor는 "문법적으로 맞는 문장"을 받아
 *   "의미적으로도 맞는지" 검사하고 실제 데이터를 다룬다.
 *
 * 메모리 / 문자열 주의점:
 *   executor는 주로 parser와 storage가 만든 구조를 읽기 전용으로 사용하며,
 *   내부에서 만든 임시 배열은 함수 종료 전에 직접 해제한다.
 */

#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ast.h"
#include "error.h"

int execute_program(const Program *program, const char *schema_dir, const char *data_dir, SqlError *error);

#endif
