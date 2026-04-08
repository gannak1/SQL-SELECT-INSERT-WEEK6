#include "utils.h"

#include <ctype.h>
#include <string.h>

/* 문자열 앞뒤 공백을 제거해서 이후 파싱을 단순하게 만든다. 
*/
void trim_in_place(char* text) {
    // 앞쪽에서 처음으로 의미 있는 문자가 시작되는 위치를 찾는다.
    size_t start = 0;
    // 뒤쪽 공백 제거를 위해 현재 문자열 길이를 구한다.
    size_t end = strlen(text);

    // 맨 앞 공백들을 건너뛴다.
    while (text[start] != '\0' && isspace((unsigned char)text[start])) {
        start++;
    }

    // 맨 뒤 공백들을 뒤에서부터 줄여 나간다.
    while (end > start && isspace((unsigned char)text[end - 1])) {
        end--;
    }

    // 앞 공백이 있었다면 의미 있는 문자열을 맨 앞으로 당긴다.
    if (start > 0) {
        memmove(text, text + start, end - start);
    }

    // 잘라낸 새 문자열의 끝에 널 문자를 넣는다.
    text[end - start] = '\0';
}

// 명령어 비교를 쉽게 하기 위해 대소문자 구분 없이 대문자로 복사한다.
void to_upper_copy(char* dest, size_t dest_size, const char* src) {
    // src와 dest를 순회할 인덱스이다.
    size_t i = 0;

    // 복사할 공간이 전혀 없으면 바로 종료한다.
    if (dest_size == 0) {
        return;
    }

    // 목적지 버퍼를 넘지 않는 범위에서 한 글자씩 대문자로 복사한다.
    for (; src[i] != '\0' && i + 1 < dest_size; ++i) {
        dest[i] = (char)toupper((unsigned char)src[i]);
    }

    // C 문자열 종료를 위해 마지막에 널 문자를 붙인다.
    dest[i] = '\0';
}

// 따옴표 없는 정수인지 확인해 숫자 타입 판별에 사용한다.
int is_integer_text(const char* text) {
    // 맨 앞 부호를 건너뛸 때 사용할 인덱스이다.
    size_t index = 0;

    // 빈 문자열은 정수가 아니다.
    if (text[0] == '\0') {
        return 0;
    }

    // 맨 앞의 부호는 허용한다.
    if (text[0] == '-' || text[0] == '+') {
        index = 1;
    }

    // 부호만 있고 숫자가 없으면 정수가 아니다.
    if (text[index] == '\0') {
        return 0;
    }

    // 남은 모든 문자가 숫자여야 정수로 본다.
    while (text[index] != '\0') {
        if (!isdigit((unsigned char)text[index])) {
            return 0;
        }
        index++;
    }

    return 1;
}

// 작은따옴표로 감싼 문자열인지 확인한다.
int is_wrapped_with_single_quotes(const char* text) {
    // 문자열 전체 길이를 구해 양끝 문자를 검사한다.
    size_t length = strlen(text);

    // 최소 길이 2 이상이고 양끝이 작은따옴표여야 한다.
    return length >= 2 && text[0] == '\'' && text[length - 1] == '\'';
}

// 입력 문자열 양끝의 작은따옴표를 제거해 실제 값만 남긴다.
void strip_single_quotes(char* dest, size_t dest_size, const char* src) {
    // 문자열 길이를 구해 마지막 따옴표 위치를 계산한다.
    size_t length = strlen(src);

    // 작은따옴표 형식이 아니면 그대로 복사한다.
    if (!is_wrapped_with_single_quotes(src)) {
        strncpy_s(dest, dest_size, src, _TRUNCATE);
        return;
    }

    // 앞뒤 작은따옴표를 제외한 실제 값만 복사한다.
    strncpy_s(dest, dest_size, src + 1, length - 2);
}

// 대소문자 차이 없이 같은 문자열인지 확인한다.
int equals_ignore_case(const char* left, const char* right) {
    // 비교 전에 대문자로 정규화할 버퍼들이다.
    char left_upper[128];
    char right_upper[128];

    // 양쪽 문자열을 같은 규칙으로 대문자화한다.
    to_upper_copy(left_upper, sizeof(left_upper), left);
    to_upper_copy(right_upper, sizeof(right_upper), right);

    return strcmp(left_upper, right_upper) == 0;
}
