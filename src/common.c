/*
 * 파일 목적:
 *   문자열 복사, 대소문자 변환, trim 같은 공통 helper를 구현한다.
 *
 * 전체 흐름에서 위치:
 *   거의 모든 모듈이 이 파일의 작은 유틸리티를 사용해
 *   문자열을 안전하게 복사하고 정규화한다.
 *
 * 주요 내용:
 *   - 동적 문자열 복사
 *   - 대소문자 변환
 *   - 공백 trim
 *   - identifier 문자 판별
 *
 * 초보자 포인트:
 *   C에서는 문자열이 객체가 아니라 char 배열이므로,
 *   "복사"가 필요할 때 직접 메모리를 할당하고 내용을 옮겨야 한다.
 *
 * 메모리 / 문자열 주의점:
 *   반환 포인터는 모두 heap에 있으므로 호출자가 free해야 한다.
 */

#include "common.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*
 * 목적:
 *   공통 DataType enum을 사람이 읽기 좋은 문자열로 바꾼다.
 *
 * 입력:
 *   type - DATA_TYPE_INT 또는 DATA_TYPE_TEXT 값.
 *
 * 반환:
 *   대응되는 문자열 상수.
 *
 * 동작 요약:
 *   switch로 각 타입을 문자열에 대응시킨다.
 *
 * 초보자 주의:
 *   반환값은 문자열 상수이므로 free하면 안 된다.
 */
const char *sql_data_type_name(DataType type) {
    switch (type) {
        case DATA_TYPE_INT:
            return "INT";
        case DATA_TYPE_TEXT:
            return "TEXT";
        default:
            return "UNKNOWN";
    }
}

/*
 * 목적:
 *   널 종료된 C 문자열 전체를 새 메모리에 복사한다.
 *
 * 입력:
 *   source - 복사할 원본 문자열.
 *
 * 반환:
 *   성공 시 새로 할당된 문자열 포인터, 실패 시 NULL.
 *
 * 동작 요약:
 *   길이를 구하고 그 길이 + 1 만큼 할당한 뒤 memcpy로 복사한다.
 *
 * 초보자 주의:
 *   source가 NULL이면 그대로 NULL을 반환해 호출자가 예외 상황을 처리하게 한다.
 */
char *sql_strdup(const char *source) {
    size_t length;
    char *copy;

    if (source == NULL) {
        return NULL;
    }

    length = strlen(source);
    copy = (char *)malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, source, length + 1);
    return copy;
}

/*
 * 목적:
 *   문자열의 앞부분 일부만 잘라 새 메모리에 복사한다.
 *
 * 입력:
 *   source - 원본 문자열 시작 위치.
 *   length - 복사할 문자 수.
 *
 * 반환:
 *   성공 시 널 종료된 새 문자열, 실패 시 NULL.
 *
 * 동작 요약:
 *   정확히 length + 1 바이트를 할당하고 마지막에 널 문자를 직접 넣는다.
 *
 * 초보자 주의:
 *   strncpy는 사용법이 헷갈리기 쉬워서, 여기서는 길이를 명시적으로 다루는 helper를 둔다.
 */
char *sql_strndup(const char *source, size_t length) {
    char *copy;

    if (source == NULL) {
        return NULL;
    }

    copy = (char *)malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, source, length);
    copy[length] = '\0';
    return copy;
}

/*
 * 목적:
 *   문자열을 소문자로 정규화한 복사본을 만든다.
 *
 * 입력:
 *   source - 변환할 문자열.
 *
 * 반환:
 *   성공 시 소문자 문자열 복사본, 실패 시 NULL.
 *
 * 동작 요약:
 *   먼저 복사본을 만들고 각 문자를 tolower로 바꾼다.
 *
 * 초보자 주의:
 *   ctype 계열 함수에는 unsigned char로 캐스팅해서 넘기는 습관이 안전하다.
 */
char *sql_lowercase_copy(const char *source) {
    char *copy;
    size_t index;

    copy = sql_strdup(source);
    if (copy == NULL) {
        return NULL;
    }

    for (index = 0; copy[index] != '\0'; ++index) {
        copy[index] = (char)tolower((unsigned char)copy[index]);
    }

    return copy;
}

/*
 * 목적:
 *   문자열을 대문자로 정규화한 복사본을 만든다.
 *
 * 입력:
 *   source - 변환할 문자열.
 *
 * 반환:
 *   성공 시 대문자 문자열 복사본, 실패 시 NULL.
 *
 * 동작 요약:
 *   복사본을 만든 뒤 각 문자를 toupper로 바꾼다.
 *
 * 초보자 주의:
 *   원본 문자열을 직접 바꾸지 않기 때문에 caller가 원본을 계속 재사용할 수 있다.
 */
char *sql_uppercase_copy(const char *source) {
    char *copy;
    size_t index;

    copy = sql_strdup(source);
    if (copy == NULL) {
        return NULL;
    }

    for (index = 0; copy[index] != '\0'; ++index) {
        copy[index] = (char)toupper((unsigned char)copy[index]);
    }

    return copy;
}

/*
 * 목적:
 *   두 문자열이 대소문자만 다르고 내용은 같은지 비교한다.
 *
 * 입력:
 *   left, right - 비교할 두 문자열.
 *
 * 반환:
 *   같으면 1, 다르면 0.
 *
 * 동작 요약:
 *   각 문자마다 tolower를 적용해 끝까지 비교한다.
 *
 * 초보자 주의:
 *   한쪽이 NULL이면 비교할 수 없으므로 0을 반환한다.
 */
int sql_case_equals(const char *left, const char *right) {
    size_t index;

    if (left == NULL || right == NULL) {
        return 0;
    }

    for (index = 0; left[index] != '\0' || right[index] != '\0'; ++index) {
        if (tolower((unsigned char)left[index]) != tolower((unsigned char)right[index])) {
            return 0;
        }
    }

    return 1;
}

/*
 * 목적:
 *   문자열 양끝 공백을 제거한 새 복사본을 만든다.
 *
 * 입력:
 *   source - trim 대상 문자열.
 *
 * 반환:
 *   성공 시 trim된 문자열, 실패 시 NULL.
 *
 * 동작 요약:
 *   앞뒤 공백 구간을 건너뛴 뒤 필요한 부분만 복사한다.
 *
 * 초보자 주의:
 *   원본 버퍼를 직접 수정하지 않으므로 strtok 결과나 파일 버퍼에도 안전하게 쓸 수 있다.
 */
char *sql_trim_copy(const char *source) {
    const char *start;
    const char *end;

    if (source == NULL) {
        return NULL;
    }

    start = source;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        ++start;
    }

    end = source + strlen(source);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        --end;
    }

    return sql_strndup(start, (size_t)(end - start));
}

/*
 * 목적:
 *   SQL 식별자의 첫 글자로 허용되는 문자인지 검사한다.
 *
 * 입력:
 *   ch - 검사할 문자 하나.
 *
 * 반환:
 *   허용되면 true, 아니면 false.
 *
 * 동작 요약:
 *   영문자 또는 underscore만 허용한다.
 *
 * 초보자 주의:
 *   숫자는 식별자 중간에는 올 수 있지만 첫 글자로는 허용하지 않는다.
 */
bool sql_is_identifier_start(char ch) {
    return isalpha((unsigned char)ch) || ch == '_';
}

/*
 * 목적:
 *   SQL 식별자의 두 번째 이후 글자로 허용되는 문자인지 검사한다.
 *
 * 입력:
 *   ch - 검사할 문자 하나.
 *
 * 반환:
 *   허용되면 true, 아니면 false.
 *
 * 동작 요약:
 *   영문자, 숫자, underscore를 허용한다.
 *
 * 초보자 주의:
 *   tokenizer가 이 규칙을 공유해야 parser에서 식별자 오류가 줄어든다.
 */
bool sql_is_identifier_part(char ch) {
    return isalnum((unsigned char)ch) || ch == '_';
}

/*
 * 목적:
 *   fgets로 읽은 줄 끝의 개행 문자를 제거한다.
 *
 * 입력:
 *   text - 수정할 문자열 버퍼.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   문자열 끝에서 '\n' 또는 '\r'을 만나면 널 문자로 바꾼다.
 *
 * 초보자 주의:
 *   Windows 스타일 CRLF도 고려해 두 번 검사한다.
 */
void sql_strip_trailing_newline(char *text) {
    size_t length;

    if (text == NULL) {
        return;
    }

    length = strlen(text);
    while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[length - 1] = '\0';
        --length;
    }
}
