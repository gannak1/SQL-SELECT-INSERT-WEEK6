/*
 * 파일 목적:
 *   외부 테스트 프레임워크 없이도 간단한 단위 테스트를 작성할 수 있도록 helper를 제공한다.
 *
 * 전체 흐름에서 위치:
 *   tokenizer / parser / executor 테스트 파일이 공통으로 include해 assertion과 요약 출력을 재사용한다.
 *
 * 주요 내용:
 *   - TestContext 구조체
 *   - 기본 assertion 매크로
 *   - suite 결과 출력 함수
 *
 * 초보자 포인트:
 *   C에서는 JUnit 같은 기본 테스트 프레임워크가 없기 때문에,
 *   작은 프로젝트에서는 이런 미니 helper를 직접 만드는 일이 많다.
 *
 * 메모리 / 문자열 주의점:
 *   이 헤더는 문자열을 소유하지 않고 오류 메시지를 그대로 출력만 한다.
 */

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdio.h>
#include <string.h>

typedef struct {
    int assertions;
    int failures;
} TestContext;

static inline void test_context_init(TestContext *context) {
    context->assertions = 0;
    context->failures = 0;
}

static inline void test_fail_message(
    TestContext *context,
    const char *file,
    int line,
    const char *message
) {
    context->failures += 1;
    fprintf(stderr, "  FAIL %s:%d: %s\n", file, line, message);
}

static inline int test_finish_suite(const char *suite_name, const TestContext *context) {
    if (context->failures == 0) {
        printf("[PASS] %s (%d assertions)\n", suite_name, context->assertions);
    } else {
        printf("[FAIL] %s (%d failures, %d assertions)\n", suite_name, context->failures, context->assertions);
    }
    return context->failures;
}

#define TEST_ASSERT(context, condition, message)                                                \
    do {                                                                                        \
        (context)->assertions += 1;                                                             \
        if (!(condition)) {                                                                     \
            test_fail_message((context), __FILE__, __LINE__, (message));                        \
        }                                                                                       \
    } while (0)

#define TEST_ASSERT_INT_EQ(context, expected, actual, message)                                  \
    do {                                                                                        \
        long test_expected_value = (long)(expected);                                            \
        long test_actual_value = (long)(actual);                                                \
        (context)->assertions += 1;                                                             \
        if (test_expected_value != test_actual_value) {                                         \
            fprintf(                                                                            \
                stderr,                                                                         \
                "  FAIL %s:%d: %s (expected=%ld, actual=%ld)\n",                                \
                __FILE__,                                                                       \
                __LINE__,                                                                       \
                (message),                                                                      \
                test_expected_value,                                                            \
                test_actual_value                                                               \
            );                                                                                  \
            (context)->failures += 1;                                                           \
        }                                                                                       \
    } while (0)

#define TEST_ASSERT_STR_EQ(context, expected, actual, message)                                  \
    do {                                                                                        \
        const char *test_expected_text = (expected);                                            \
        const char *test_actual_text = (actual);                                                \
        (context)->assertions += 1;                                                             \
        if (test_expected_text == NULL || test_actual_text == NULL ||                           \
            strcmp(test_expected_text, test_actual_text) != 0) {                                \
            fprintf(                                                                            \
                stderr,                                                                         \
                "  FAIL %s:%d: %s (expected=%s, actual=%s)\n",                                  \
                __FILE__,                                                                       \
                __LINE__,                                                                       \
                (message),                                                                      \
                test_expected_text == NULL ? "(null)" : test_expected_text,                     \
                test_actual_text == NULL ? "(null)" : test_actual_text                          \
            );                                                                                  \
            (context)->failures += 1;                                                           \
        }                                                                                       \
    } while (0)

#endif
