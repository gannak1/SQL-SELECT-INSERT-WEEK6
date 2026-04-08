#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

static QueryType parse_query_type(const char* command) {
    char normalized[MAX_ITEM_LENGTH];

    // 대문자 변환
    to_upper_copy(normalized, sizeof(normalized), command);

    // select면 select
    if (strcmp(normalized, "SELECT") == 0) {
        return QUERY_SELECT;
    }

    // insert면 insert
    if (strcmp(normalized, "INSERT") == 0) {
        return QUERY_INSERT;
    }

    return QUERY_UNKNOWN;
}

static int split_items(const char* payload, Query* query) {
    char buffer[MAX_INPUT_LENGTH];
    char* context = NULL;
    char* token = NULL;

    strncpy_s(buffer, sizeof(buffer), payload, _TRUNCATE);

    token = strtok_s(buffer, ",", &context);
    while (token != NULL) {
        if (query->item_count >= MAX_ITEM_COUNT) {
            return 0;
        }

        trim_in_place(token);
        if (token[0] != '\0') {
            strncpy_s(
                query->items[query->item_count],
                sizeof(query->items[query->item_count]),
                token,
                _TRUNCATE
            );
            query->item_count++;
        }

        token = strtok_s(NULL, ",", &context);
    }

    return 1;
}

static int matches_keyword_at(const char* text, const char* keyword, const char* base) {
    size_t keyword_length = strlen(keyword);
    size_t index = 0;

    if (text != base && !isspace((unsigned char)text[-1])) {
        return 0;
    }

    for (index = 0; index < keyword_length; ++index) {
        if (text[index] == '\0') {
            return 0;
        }

        if (toupper((unsigned char)text[index]) != toupper((unsigned char)keyword[index])) {
            return 0;
        }
    }

    return text[keyword_length] == '\0' || isspace((unsigned char)text[keyword_length]);
}

static char* find_keyword_position(char* text, const char* keyword) {
    char* cursor = text;

    while (*cursor != '\0') {
        if (matches_keyword_at(cursor, keyword, text)) {
            return cursor;
        }
        cursor++;
    }

    return NULL;
}

static int parse_insert_value(const char* item, ParsedValue* value, char* error_message, size_t error_size) {
    char normalized[MAX_ITEM_LENGTH];

    strncpy_s(normalized, sizeof(normalized), item, _TRUNCATE);
    trim_in_place(normalized);

    if (normalized[0] == '\0') {
        snprintf(error_message, error_size, "빈 값은 INSERT에 사용할 수 없습니다.");
        return 0;
    }

    strncpy_s(value->raw, sizeof(value->raw), normalized, _TRUNCATE);

    if (is_wrapped_with_single_quotes(normalized)) {
        value->type = VALUE_STRING;
        strip_single_quotes(value->text, sizeof(value->text), normalized);
        return 1;
    }

    if (equals_ignore_case(normalized, "TRUE") || equals_ignore_case(normalized, "FALSE")) {
        value->type = VALUE_BOOLEAN;
        to_upper_copy(value->text, sizeof(value->text), normalized);
        return 1;
    }

    if (is_integer_text(normalized)) {
        value->type = VALUE_INTEGER;
        strncpy_s(value->text, sizeof(value->text), normalized, _TRUNCATE);
        return 1;
    }

    snprintf(
        error_message,
        error_size,
        "문자열은 작은따옴표로 감싸고, 숫자나 TRUE/FALSE만 따옴표 없이 입력할 수 있습니다."
    );
    return 0;
}

static int parse_condition_value(const char* text, ParsedValue* value, char* error_message, size_t error_size) {
    return parse_insert_value(text, value, error_message, error_size);
}

static char* find_condition_operator(char* text, ComparisonOperator* where_operator) {
    char* equal_position = strchr(text, '=');
    char* greater_position = strchr(text, '>');
    char* less_position = strchr(text, '<');

    if (equal_position != NULL) {
        *where_operator = OPERATOR_EQUAL;
        return equal_position;
    }

    if (greater_position != NULL) {
        *where_operator = OPERATOR_GREATER;
        return greater_position;
    }

    if (less_position != NULL) {
        *where_operator = OPERATOR_LESS;
        return less_position;
    }

    return NULL;
}

// payload는 "name FROM table" 형태의 문자열, query에 파싱 결과 저장
static int parse_select_payload(char* payload, Query* query) {
    char* from_position = NULL;
    char* where_position = NULL;
    char columns_part[MAX_INPUT_LENGTH];
    char table_part[MAX_ITEM_LENGTH];
    char condition_part[MAX_INPUT_LENGTH];
    char* operator_position = NULL;
    char error_message[256];

    // payload에서 FROM 키워드 위치 찾기
    from_position = find_keyword_position(payload, "FROM");
    // FROM 없으면 잘못된 쿼리
    if (from_position == NULL) {
        return 0;
    }

    // payload 시작부터 FROM 직전까지 = 컬럼 목록
    // ex) "name " 또는 "id, name "
    strncpy_s(columns_part, sizeof(columns_part), payload, (size_t)(from_position - payload));
    trim_in_place(columns_part);
    // 컬럼이 비어있으면 잘못된 쿼리
    if (columns_part[0] == '\0') {
        return 0;
    }

    // FROM 키워드 길이만큼 건너뜀
    from_position += strlen("FROM");
    // FROM 이후 공백 건너뜀
    while (*from_position != '\0' && isspace((unsigned char)*from_position)) {
        from_position++;
    }

    // FROM 이후 아무것도 없으면 테이블명 없는 거니까 리턴
    if (*from_position == '\0') {
        return 0;
    }

    // FROM 이후에서 WHERE 키워드 위치 찾기
    where_position = find_keyword_position(from_position, "WHERE");
    if (where_position != NULL) {
        // WHERE 있으면 FROM ~ WHERE 사이가 테이블명
        strncpy_s(table_part, sizeof(table_part), from_position, (size_t)(where_position - from_position));
        // WHERE 키워드 길이만큼 건너뜀
        where_position += strlen("WHERE");
        // WHERE 이후 나머지가 조건절
        strncpy_s(condition_part, sizeof(condition_part), where_position, _TRUNCATE);
        trim_in_place(condition_part);
    }
    else {
        // WHERE 없으면 FROM 이후 전체가 테이블명
        strncpy_s(table_part, sizeof(table_part), from_position, _TRUNCATE);
        condition_part[0] = '\0';
    }

    // 테이블명 공백 제거 후 비어있으면 리턴
    trim_in_place(table_part);
    if (table_part[0] == '\0') {
        return 0;
    }

    // 테이블명 저장
    strncpy_s(query->table_name, sizeof(query->table_name), table_part, _TRUNCATE);

    // 컬럼 목록을 콤마 기준으로 분리해서 query->items에 저장
    if (!split_items(columns_part, query)) {
        return 0;
    }

    // WHERE 조건절 없으면 여기서 끝
    if (condition_part[0] == '\0') {
        return 1;
    }

    // 조건절에서 =, >, < 연산자 위치 찾기
    operator_position = find_condition_operator(condition_part, &query->where_operator);
    // 연산자 없으면 잘못된 조건절
    if (operator_position == NULL) {
        return 0;
    }

    // 연산자 위치를 \0으로 덮어서 왼쪽(컬럼명)과 오른쪽(값)을 분리
    *operator_position = '\0';
    // 연산자 왼쪽 = WHERE 컬럼명
    strncpy_s(query->where_column, sizeof(query->where_column), condition_part, _TRUNCATE);
    trim_in_place(query->where_column);
    // 연산자 오른쪽 공백 제거
    trim_in_place(operator_position + 1);

    // 컬럼명이나 값이 비어있으면 잘못된 조건절
    if (query->where_column[0] == '\0' || operator_position[1] == '\0') {
        return 0;
    }

    // 연산자 오른쪽 값을 타입 판별해서 where_value에 저장
    if (!parse_condition_value(operator_position + 1, &query->where_value, error_message, sizeof(error_message))) {
        return 0;
    }

    // WHERE 조건 있음 표시
    query->has_where = 1;
    return 1;
}

// input은 값이 바뀌면 안되니까 const, 그리고 query는 수정해야하니까 *변수로 선언
int parse_query(const char* input, Query* query) {
    // 워킹은 input 복사본으로 사용할 녀석
    char working[MAX_INPUT_LENGTH];
    char* semicolon = NULL;
    char* cursor = NULL;
    char* payload = NULL;
    size_t command_length = 0;

    // 쿼리 초기화 Null로 만들고, query크기 만큼 사이즈 만듦.
    // 사이즈오브는 몇 바이트를 0으로 채울지 알아야 해서
    memset(query, 0, sizeof(*query));
    // query 인스턴스의 raw_input에 메모리 넣어주고 값 넣는데, 메모리 넘어가면 중간에 자르기
    strncpy_s(query->raw_input, sizeof(query->raw_input), input, _TRUNCATE);
    
    // string working에 input의 글자값 넣음
    strncpy_s(working, sizeof(working), input, _TRUNCATE);

    // 이거 앞에서부터 공백 제거해주는 함수. working.
    trim_in_place(working);

    // 마지막 ;의 위치를 찾음
    semicolon = strrchr(working, ';');
    // 못찾으면 리턴
    if (semicolon == NULL) {
        return 0;
    }

    // 세미콜론 날리기
    *semicolon = '\0';
    // 공백 제거
    trim_in_place(working);

    // trim 이후 세미콜론 지웠는데 첫 값이 null이면 빈쿼리니까 리턴
    if (working[0] == '\0') {
        return 0;
    }

    // 워킹 주소값 가져오고 
    cursor = working;
    // 워킹의 주소값 위치를 하나씩 넘기면서 해당 구문의 마지막 주소위치를 파악하는 용도
    // isspace는 띄어쓰기까지임
    while (*cursor != '\0' && !isspace((unsigned char)*cursor)) {
        cursor++;
    }
    // 쿼리 길이용도. 시작과 끝을 빼서 길이 구함.
    command_length = (size_t)(cursor - working);
    
    // 0일 경우 혹은 현재 커맨드 길이가 query command랑 동일할 경우, 값이 이상하단 소리니까 리턴
    if (command_length == 0 || command_length >= sizeof(query->command)) {
        return 0;
    }

    // command에 working값을 command 메모리만큼 넣어줌
    memcpy(query->command, working, command_length);

    // 마지막 문자열 뒤에 null 삽입
    query->command[command_length] = '\0';
    // 쿼리 타입 구분
    query->type = parse_query_type(query->command);

    // null 이면서 스페이스 발견까지 주소값 찾기 (마지막 주소까지 찾기)
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        cursor++;
    }

    // 그 값을 페이로드에 삽입
    payload = cursor;
    // 쿼리 파악 안되면 리턴
    if (query->type == QUERY_UNKNOWN || payload[0] == '\0') {
        return 0;
    }

    // select문이면 해당문으로 이동
    if (query->type == QUERY_SELECT) {
        return parse_select_payload(payload, query);
    }

    // 인서트문이면 아래 함수 시행
    return split_items(payload, query);
}

// query에서 INSERT 값들을 꺼내서 insert_command에 타입 판별 후 저장
int parse_insert_command(const Query* query, InsertCommand* insert_command, char* error_message, size_t error_size) {
    int index = 0;

    // INSERT 타입이 아니면 리턴
    if (query->type != QUERY_INSERT) {
        snprintf(error_message, error_size, "현재 INSERT 명령만 저장할 수 있습니다.");
        return 0;
    }

    // 스키마 컬럼 개수(5개)랑 입력값 개수가 다르면 리턴
    // ex) INSERT 1, 'hong123', 'Hong' <- 3개면 실패
    if (query->item_count != SCHEMA_COLUMN_COUNT) {
        snprintf(error_message, error_size, "INSERT는 값 %d개가 필요합니다.", SCHEMA_COLUMN_COUNT);
        return 0;
    }

    // insert_command 초기화
    memset(insert_command, 0, sizeof(*insert_command));

    // query->items 순서대로 꺼내서 타입 판별 후 insert_command->values에 저장
    // ex) 1 -> VALUE_INTEGER, 'hong123' -> VALUE_STRING
    for (index = 0; index < SCHEMA_COLUMN_COUNT; ++index) {
        if (!parse_insert_value(query->items[index], &insert_command->values[index], error_message, error_size)) {
            return 0;
        }
    }

    return 1;
}

int is_exit_command(const char* input) {
    char normalized[MAX_INPUT_LENGTH];

    strncpy_s(normalized, sizeof(normalized), input, _TRUNCATE);
    trim_in_place(normalized);
    to_upper_copy(normalized, sizeof(normalized), normalized);

    return strcmp(normalized, "EXIT") == 0 || strcmp(normalized, "QUIT") == 0;
}
