/*
 * 파일 목적:
 *   "SQL 텍스트를 받아 실제로 실행하는 공통 파이프라인"을 선언한다.
 *
 * 전체 흐름에서 위치:
 *   파일 모드, -e 문자열 실행 모드, REPL 모드가 모두 이 모듈을 통해
 *   같은 tokenize -> parse -> execute 경로를 재사용한다.
 *
 * 주요 내용:
 *   - SQL 텍스트 실행 함수
 *   - SQL 파일 실행 함수
 *
 * 초보자 포인트:
 *   입력 방식이 달라도, 결국 SQL 문자열만 확보되면 이후 파이프라인은 같다는 점을
 *   코드 구조로 보여주기 위해 이 모듈을 둔다.
 *
 * 메모리 / 문자열 주의점:
 *   run_sql_text는 내부에서 토큰과 Program을 할당하고 함수 종료 전에 정리한다.
 *   run_sql_file은 파일 내용을 읽은 뒤 그 버퍼도 함께 정리한다.
 */

#ifndef SQL_RUNNER_H
#define SQL_RUNNER_H

#include "error.h"

int run_sql_text(const char *sql_text, const char *schema_dir, const char *data_dir, SqlError *error);
int run_sql_file(const char *path, const char *schema_dir, const char *data_dir, SqlError *error);

#endif
