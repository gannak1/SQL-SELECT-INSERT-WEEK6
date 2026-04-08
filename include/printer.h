/*
 * 파일 목적:
 *   INSERT 성공 메시지와 SELECT 결과를 일정한 형식으로 출력하는 함수를 선언한다.
 *
 * 전체 흐름에서 위치:
 *   executor가 실제 결과를 계산한 뒤,
 *   마지막 표준 출력 형식은 이 모듈을 통해 맞춘다.
 *
 * 주요 내용:
 *   - INSERT 성공 출력
 *   - SELECT 헤더, 행, footer 출력
 *
 * 초보자 포인트:
 *   출력 형식을 한 곳에 모아두면 테스트가 쉬워지고,
 *   나중에 포맷을 바꿔도 executor 전체를 뒤지지 않아도 된다.
 *
 * 메모리 / 문자열 주의점:
 *   이 모듈은 문자열을 소유하지 않고 읽기만 하므로 free를 하지 않는다.
 */

#ifndef PRINTER_H
#define PRINTER_H

#include <stddef.h>

void print_insert_success(const char *table_name);
void print_result_header(const char *table_name, char **headers, size_t header_count);
void print_result_row(char **values, size_t value_count);
void print_result_footer(size_t row_count);

#endif
