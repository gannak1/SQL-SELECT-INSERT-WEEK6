/*
 * 파일 목적:
 *   프로젝트 전체에서 공통으로 쓰는 에러 저장 구조와 출력 함수를 선언한다.
 *
 * 전체 흐름에서 위치:
 *   각 모듈은 실패 이유를 SqlError에 기록하고,
 *   main은 마지막에 그 메시지를 stderr로 출력한 뒤 종료 코드를 결정한다.
 *
 * 주요 내용:
 *   - 에러 메시지 버퍼를 가진 SqlError 구조체
 *   - 에러 초기화, 설정, 출력 함수
 *
 * 초보자 포인트:
 *   C에는 예외(exception)가 없으므로,
 *   함수 반환값과 별도의 에러 버퍼를 함께 쓰는 패턴이 자주 등장한다.
 *
 * 메모리 / 문자열 주의점:
 *   에러 메시지는 고정 길이 배열에 저장되므로,
 *   vsnprintf로 넘치는 문자열을 잘라서 안전하게 기록한다.
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>

#define SQL_ERROR_MESSAGE_SIZE 256

typedef struct {
    int is_set;
    char message[SQL_ERROR_MESSAGE_SIZE];
} SqlError;

void sql_error_clear(SqlError *error);
void sql_error_set(SqlError *error, const char *format, ...);
int sql_error_has(const SqlError *error);
void sql_error_print(const SqlError *error);

#endif
