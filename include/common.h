/*
 * 파일 목적:
 *   여러 모듈에서 공통으로 쓰는 자료형과 문자열 유틸리티 함수를 선언한다.
 *
 * 전체 흐름에서 위치:
 *   tokenizer, parser, schema, storage, executor가 모두 이 헤더를 포함해
 *   같은 데이터 타입과 문자열 규칙을 공유한다.
 *
 * 주요 내용:
 *   - INT / TEXT 공통 타입 정의
 *   - 문자열 복사, 대소문자 변환, trim 유틸리티
 *   - identifier 판별 도우미
 *
 * 초보자 포인트:
 *   C 표준 라이브러리에는 자주 쓰는 문자열 편의 함수들이 부족하므로,
 *   프로젝트 안에서 작은 helper를 직접 만드는 일이 흔하다.
 *
 * 메모리 / 문자열 주의점:
 *   이 헤더의 문자열 함수들은 새 메모리를 할당해 반환한다.
 *   호출한 쪽이 free로 해제해야 한다.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>

#define SQL_PATH_LENGTH 512
#define SQL_LINE_BUFFER_SIZE 4096

typedef enum {
    DATA_TYPE_INT,
    DATA_TYPE_TEXT
} DataType;

const char *sql_data_type_name(DataType type);
char *sql_strdup(const char *source);
char *sql_strndup(const char *source, size_t length);
char *sql_lowercase_copy(const char *source);
char *sql_uppercase_copy(const char *source);
int sql_case_equals(const char *left, const char *right);
char *sql_trim_copy(const char *source);
bool sql_is_identifier_start(char ch);
bool sql_is_identifier_part(char ch);
void sql_strip_trailing_newline(char *text);

#endif
