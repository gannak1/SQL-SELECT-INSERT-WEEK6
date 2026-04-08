#include "utils.h"

#include <ctype.h>
#include <string.h>

/* trim_newline()은 문자열 끝에 붙은 \n, \r을 제거해서
   입력 한 줄을 깔끔한 C 문자열로 정리한다. */
void trim_newline(char *str) {
    /* fgets는 줄 끝 개행을 같이 읽기 때문에 먼저 제거해 준다. */
    /* len은 현재 문자열 길이이고,
       뒤에서부터 \n, \r를 지울 때 계속 줄어든다. */
    size_t len;

    if (str == NULL) {
        return;
    }

    len = strlen(str);
    /* 줄 끝에 \n, \r 가 남아 있는 동안 반복해서 제거한다. */
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

/* trim_spaces()는 문자열 앞뒤 공백을 잘라내
   parser가 토큰을 읽기 쉬운 형태로 만든다. */
void trim_spaces(char *str) {
    /* 문자열 앞뒤 공백을 제거하는 helper 함수다. */
    /* start는 "앞 공백이 끝난 실제 문자열 시작 위치"를 가리킨다. */
    char *start;
    /* len은 뒤 공백을 지울 때 현재 문자열 길이를 기억한다. */
    size_t len;

    if (str == NULL) {
        return;
    }

    start = str;
    /* 앞 공백이 끝나는 위치까지 이동 */
    while (*start != '\0' && isspace((unsigned char) *start)) {
        start++;
    }

    /* 앞쪽 공백이 있었다면 문자열 전체를 앞으로 당긴다. */
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    len = strlen(str);
    /* 이번에는 뒤 공백을 끝에서부터 지운다. */
    while (len > 0 && isspace((unsigned char) str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

/* strip_matching_quotes()는 양끝이 같은 따옴표로 감싸진 문자열에서
   바깥 따옴표 한 쌍만 제거한다. */
void strip_matching_quotes(char *str) {
    /* 양끝이 같은 따옴표라면 바깥 따옴표만 제거한다. */
    /* len은 문자열 전체 길이이며,
       앞뒤 따옴표를 제거할 수 있는지 판단할 때 사용한다. */
    size_t len;

    if (str == NULL) {
        return;
    }

    len = strlen(str);
    if (len >= 2 && ((str[0] == '\'' && str[len - 1] == '\'') || (str[0] == '"' && str[len - 1] == '"'))) {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

/* starts_with_ignore_case()는 대소문자를 무시하고
   str이 prefix로 시작하는지 확인한다. */
int starts_with_ignore_case(const char *str, const char *prefix) {
    /* 대소문자를 무시한 startsWith 비교 */
    /* i는 prefix의 몇 번째 글자를 비교 중인지 나타낸다. */
    size_t i;

    if (str == NULL || prefix == NULL) {
        return 0;
    }

    /* prefix 끝까지 한 글자씩 비교 */
    for (i = 0; prefix[i] != '\0'; i++) {
        if (toupper((unsigned char) str[i]) != toupper((unsigned char) prefix[i])) {
            return 0;
        }
    }

    return 1;
}

/* is_quoted_string()은 문자열이 'abc' 또는 "abc"처럼
   양끝 따옴표가 맞게 닫힌 형태인지 검사한다. */
int is_quoted_string(const char *str) {
    /* "abc" 또는 'abc'처럼 양끝이 따옴표로 닫혔는지 검사 */
    /* len은 첫 글자와 마지막 글자를 확인하기 위한 문자열 길이다. */
    size_t len;

    if (str == NULL) {
        return 0;
    }

    len = strlen(str);
    if (len < 2) {
        return 0;
    }

    return (str[0] == '\'' && str[len - 1] == '\'')
        || (str[0] == '"' && str[len - 1] == '"');
}

/* is_blank_string()은 문자열이 비어 있거나 공백뿐인지 확인한다. */
int is_blank_string(const char *str) {
    /* 공백만 있거나 빈 문자열이면 true */
    if (str == NULL) {
        return 1;
    }

    /* 공백이 아닌 문자를 만나면 빈 문자열이 아니다. */
    while (*str != '\0') {
        if (!isspace((unsigned char) *str)) {
            return 0;
        }
        str++;
    }

    return 1;
}

/* is_integer_string()은 문자열이 정수 형태인지 검사해서
   id, age 같은 숫자 칼럼 검증에 사용한다. */
int is_integer_string(const char *str) {
    /* id, age처럼 숫자여야 하는 값 검사용 helper */
    if (str == NULL || *str == '\0') {
        return 0;
    }

    /* 맨 앞의 +, - 부호는 한 번 허용 */
    if (*str == '+' || *str == '-') {
        str++;
    }

    if (*str == '\0') {
        return 0;
    }

    /* 남은 모든 문자가 숫자인지 검사 */
    while (*str != '\0') {
        if (!isdigit((unsigned char) *str)) {
            return 0;
        }
        str++;
    }

    return 1;
}

/* copy_trimmed()는 src의 앞뒤 공백을 제거한 내용을
   dest 버퍼 크기 안에서 안전하게 복사한다. */
size_t copy_trimmed(char *dest, size_t dest_size, const char *src) {
    /* src의 앞뒤 공백을 제거한 뒤 dest에 복사한다. */
    /* start는 앞 공백이 끝나는 위치,
       end는 뒤 공백을 제외한 마지막 위치 바로 다음 칸이다. */
    size_t start = 0;
    size_t end;
    /* len은 실제로 dest에 복사할 글자 수다. */
    size_t len;

    if (dest == NULL || dest_size == 0) {
        return 0;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return 0;
    }

    /* 앞 공백 시작점과 뒤 공백 끝점을 찾는다. */
    end = strlen(src);
    while (src[start] != '\0' && isspace((unsigned char) src[start])) {
        start++;
    }
    while (end > start && isspace((unsigned char) src[end - 1])) {
        end--;
    }

    /* dest 버퍼보다 길면 잘라서 복사 */
    len = end - start;
    if (len >= dest_size) {
        len = dest_size - 1;
    }

    memcpy(dest, src + start, len);
    dest[len] = '\0';
    return len;
}
