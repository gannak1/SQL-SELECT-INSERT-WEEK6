/*
 * 파일 목적:
 *   INSERT와 SELECT 결과를 테스트하기 쉬운 고정 포맷으로 출력한다.
 *
 * 전체 흐름에서 위치:
 *   executor가 실제 계산을 끝낸 뒤 stdout 형식은 이 모듈을 통해 맞춘다.
 *
 * 주요 내용:
 *   - INSERT 성공 메시지
 *   - SELECT header / row / footer 출력
 *
 * 초보자 포인트:
 *   출력 포맷이 한 모듈에 모여 있으면 "비즈니스 로직"과 "화면 표현"이 섞이지 않는다.
 *
 * 메모리 / 문자열 주의점:
 *   이 모듈은 전달받은 포인터를 읽기만 하고 소유권을 가져오지 않는다.
 */

#include "printer.h"

#include <stdio.h>

/*
 * 목적:
 *   INSERT 성공 메시지를 스펙에 맞춰 출력한다.
 *
 * 입력:
 *   table_name - 삽입이 성공한 테이블 이름.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   "OK: 1 row inserted into ..." 형식으로 한 줄 출력한다.
 *
 * 초보자 주의:
 *   성공 메시지도 테스트 대상이므로 띄어쓰기와 대소문자를 일정하게 유지해야 한다.
 */
void print_insert_success(const char *table_name) {
    printf("OK: 1 row inserted into %s\n", table_name);
}

/*
 * 목적:
 *   SELECT 결과의 첫 두 줄(RESULT와 header)을 출력한다.
 *
 * 입력:
 *   table_name - 결과가 나온 테이블 이름.
 *   headers - 출력할 컬럼 이름 배열.
 *   header_count - 컬럼 개수.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   RESULT 줄과 쉼표로 이어진 header 줄을 순서대로 출력한다.
 *
 * 초보자 주의:
 *   row가 0개여도 header는 항상 출력해야 사람이 결과 구조를 알 수 있다.
 */
void print_result_header(const char *table_name, char **headers, size_t header_count) {
    size_t index;

    printf("RESULT: %s\n", table_name);
    for (index = 0; index < header_count; ++index) {
        if (index > 0) {
            putchar(',');
        }
        fputs(headers[index], stdout);
    }
    putchar('\n');
}

/*
 * 목적:
 *   SELECT 결과의 실제 데이터 행 하나를 출력한다.
 *
 * 입력:
 *   values - 이미 projection이 적용된 셀 배열.
 *   value_count - 셀 개수.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   각 셀을 쉼표로 이어 한 줄로 출력한다.
 *
 * 초보자 주의:
 *   이번 프로젝트는 TEXT 안에 쉼표를 허용하지 않기 때문에 단순 join이 가능하다.
 */
void print_result_row(char **values, size_t value_count) {
    size_t index;

    for (index = 0; index < value_count; ++index) {
        if (index > 0) {
            putchar(',');
        }
        fputs(values[index], stdout);
    }
    putchar('\n');
}

/*
 * 목적:
 *   SELECT 결과의 마지막 줄(ROWS)을 출력한다.
 *
 * 입력:
 *   row_count - 실제 출력한 데이터 행 수.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   스펙의 "ROWS: N" 형식으로 마무리한다.
 *
 * 초보자 주의:
 *   row_count는 WHERE 필터와 projection 이후 실제 남은 행 수여야 한다.
 */
void print_result_footer(size_t row_count) {
    printf("ROWS: %zu\n", row_count);
}
