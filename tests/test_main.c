/*
 * 파일 목적:
 *   모든 단위 테스트 suite를 한 번에 실행하는 테스트 바이너리의 진입점을 제공한다.
 *
 * 전체 흐름에서 위치:
 *   make test가 unit_tests 바이너리를 실행하면 가장 먼저 이 main이 호출된다.
 *
 * 주요 내용:
 *   - tokenizer / parser / executor suite 호출
 *   - 전체 성공/실패 요약
 *
 * 초보자 포인트:
 *   각 테스트 파일을 독립적인 suite 함수로 만들면,
 *   나중에 실패한 영역을 빠르게 찾기 쉽다.
 *
 * 메모리 / 문자열 주의점:
 *   여기서는 별도 동적 메모리를 다루지 않지만,
 *   각 suite 내부에서 할당한 자원은 그 suite 안에서 정리해야 한다.
 */

#include <stdio.h>

int run_tokenizer_tests(void);
int run_parser_tests(void);
int run_executor_tests(void);

/*
 * 목적:
 *   모든 단위 테스트 suite를 순서대로 실행한다.
 *
 * 입력:
 *   없음.
 *
 * 반환:
 *   모든 suite 성공 시 0, 하나라도 실패하면 1.
 *
 * 동작 요약:
 *   세 suite의 실패 개수를 합산해 최종 종료 코드를 결정한다.
 *
 * 초보자 주의:
 *   테스트 바이너리는 CI나 make test에서 종료 코드로 성공/실패를 판단한다.
 */
int main(void) {
    int total_failures;

    total_failures = 0;
    total_failures += run_tokenizer_tests();
    total_failures += run_parser_tests();
    total_failures += run_executor_tests();

    if (total_failures == 0) {
        printf("All unit tests passed.\n");
        return 0;
    }

    printf("Unit tests failed: %d\n", total_failures);
    return 1;
}
