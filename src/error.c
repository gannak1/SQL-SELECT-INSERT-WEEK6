/*
 * 파일 목적:
 *   SqlError를 초기화하고 메시지를 기록/출력하는 공통 함수를 구현한다.
 *
 * 전체 흐름에서 위치:
 *   tokenizer, parser, schema, executor 등 어디서든 실패 이유를 한 형식으로 기록한다.
 *
 * 주요 내용:
 *   - error clear
 *   - formatted error set
 *   - stderr 출력
 *
 * 초보자 포인트:
 *   여러 함수가 깊게 호출되는 C 코드에서는,
 *   상세한 실패 이유를 위로 올리기 위해 이런 구조가 특히 유용하다.
 *
 * 메모리 / 문자열 주의점:
 *   가변 인자를 쓸 때는 vsnprintf를 사용해 버퍼 넘침을 막는다.
 */

#include "error.h"

#include <stdarg.h>
#include <string.h>

/*
 * 목적:
 *   SqlError 구조체를 "에러 없음" 상태로 초기화한다.
 *
 * 입력:
 *   error - 초기화할 SqlError 포인터.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   플래그를 0으로 두고 메시지 버퍼 첫 글자를 널 문자로 만든다.
 *
 * 초보자 주의:
 *   함수 진입 전에 error를 항상 clear해두면 오래된 메시지가 남지 않는다.
 */
void sql_error_clear(SqlError *error) {
    if (error == NULL) {
        return;
    }

    error->is_set = 0;
    error->message[0] = '\0';
}

/*
 * 목적:
 *   printf 스타일 포맷 문자열로 에러 메시지를 기록한다.
 *
 * 입력:
 *   error - 메시지를 저장할 구조체.
 *   format - printf와 같은 포맷 문자열.
 *   ... - 포맷에 들어갈 값들.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   va_list를 이용해 message 버퍼에 안전하게 문자열을 쓴다.
 *
 * 초보자 주의:
 *   error가 NULL이면 기록할 곳이 없으므로 조용히 반환한다.
 */
void sql_error_set(SqlError *error, const char *format, ...) {
    va_list args;

    if (error == NULL) {
        return;
    }

    error->is_set = 1;

    va_start(args, format);
    vsnprintf(error->message, sizeof(error->message), format, args);
    va_end(args);
}

/*
 * 목적:
 *   현재 SqlError에 메시지가 설정되어 있는지 검사한다.
 *
 * 입력:
 *   error - 검사할 SqlError 포인터.
 *
 * 반환:
 *   메시지가 있으면 1, 없으면 0.
 *
 * 동작 요약:
 *   NULL 검사 후 is_set 플래그를 그대로 돌려준다.
 *
 * 초보자 주의:
 *   단순해 보여도 NULL 방어를 한 곳에 모아두면 호출 코드가 깔끔해진다.
 */
int sql_error_has(const SqlError *error) {
    return error != NULL && error->is_set;
}

/*
 * 목적:
 *   저장된 에러 메시지를 stderr로 출력한다.
 *
 * 입력:
 *   error - 출력할 SqlError 포인터.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   메시지가 설정된 경우에만 줄바꿈과 함께 출력한다.
 *
 * 초보자 주의:
 *   stdout과 stderr를 분리해 두면 테스트에서 정상 출력과 에러 출력을 구분하기 쉽다.
 */
void sql_error_print(const SqlError *error) {
    if (error == NULL || !error->is_set) {
        return;
    }

    fprintf(stderr, "%s\n", error->message);
}
