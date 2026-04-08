/*
 * 파일 목적:
 *   SQL 파일이나 schema 파일처럼 "파일 전체를 한 번에 읽고 싶을 때" 쓰는 함수를 선언한다.
 *
 * 전체 흐름에서 위치:
 *   main은 SQL 파일을 읽을 때 이 모듈을 사용하고,
 *   schema 모듈도 schema 파일 내용을 읽을 때 같은 함수를 재사용한다.
 *
 * 주요 내용:
 *   - 파일 전체 내용을 동적 메모리로 읽어 반환하는 함수
 *
 * 초보자 포인트:
 *   파일 크기를 먼저 구한 뒤 한 번에 읽으면 구현이 단순해진다.
 *   다만 파일을 다 읽은 뒤에는 꼭 free를 해줘야 한다.
 *
 * 메모리 / 문자열 주의점:
 *   반환 문자열은 널 종료 문자를 포함하도록 한 칸 더 할당한다.
 */

#ifndef FILE_READER_H
#define FILE_READER_H

#include "error.h"

int read_entire_file(const char *path, char **out_contents, SqlError *error);

#endif
