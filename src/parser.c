#include "parser.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "utils.h"

static const char *skip_spaces(const char *s);
static int match_keyword(const char **input, const char *keyword);
static int parse_identifier_token(const char **input, char *dest, size_t dest_size);
static ParseStatus parse_value_token(const char **input, char **out_value);
static ParseStatus parse_values_list(const char **input, InsertCommand *insert_cmd);
static char *duplicate_range(const char *start, size_t len);

/* parse_command()는 parser의 진입점이다.
   사용자가 입력한 한 문장을 보고 INSERT인지 SELECT인지 먼저 구분한 뒤,
   맞는 세부 parser 함수로 보내는 역할을 한다. */
ParseStatus parse_command(const char *input, Command *command) {
    /* parser의 시작점:
       입력 문자열이 INSERT인지 SELECT인지 먼저 판별한다. */
    /* cursor는 input 문자열 안에서
       "지금 어디까지 읽었는지"를 가리키는 포인터다. */
    const char *cursor;

    if (input == NULL || command == NULL) {
        return PARSE_UNSUPPORTED_COMMAND;
    }

    init_command(command);
    cursor = skip_spaces(input);

    /* 입력 첫 단어를 보고 어떤 parser로 보낼지 결정한다. */
    if (starts_with_ignore_case(cursor, "insert")) {
        return parse_insert(cursor, command);
    }

    if (starts_with_ignore_case(cursor, "select")) {
        return parse_select(cursor, command);
    }

    return PARSE_UNSUPPORTED_COMMAND;
}

/* parse_insert()는
   INSERT INTO users VALUES (...) 형태를 구조체로 바꾸는 함수다. */
ParseStatus parse_insert(const char *input, Command *command) {
    /* INSERT INTO users VALUES (...) 형태를 순서대로 확인한다.
       현재 VALUES 안에는 username, name, age, phone, email의
       5개 사용자 입력 값만 들어오고, id는 나중에 자동 생성된다. */
    /* cursor는 INSERT 문장을 왼쪽부터 읽어 가면서
       현재 해석 위치를 계속 앞으로 옮기는 데 사용한다. */
    const char *cursor = input;
    /* status는 아래에서 parse_values_list() 같은 하위 parser를 호출했을 때
       그 결과가 성공(PARSE_OK)인지, 아니면 어떤 파싱 오류인지
       잠깐 저장해 두는 상태 변수다. */
    ParseStatus status;

    /* INSERT 키워드 확인 */
    if (!match_keyword(&cursor, "INSERT")) {
        return PARSE_INVALID_INSERT;
    }

    /* INTO 키워드 확인 */
    if (!match_keyword(&cursor, "INTO")) {
        return PARSE_INVALID_INSERT;
    }

    /* 테이블 이름 저장 */
    if (!parse_identifier_token(&cursor, command->insert_cmd.table, sizeof(command->insert_cmd.table))) {
        return PARSE_INVALID_INSERT;
    }
    /* 성공하면 table 이름이 insert_cmd.table 안으로 복사된다. */

    /* VALUES 키워드 확인 */
    if (!match_keyword(&cursor, "VALUES")) {
        return PARSE_INVALID_INSERT;
    }

    cursor = skip_spaces(cursor);
    /* 값 목록은 반드시 여는 괄호로 시작한다. */
    if (*cursor != '(') {
        return PARSE_INVALID_INSERT;
    }
    cursor++;

    /* 괄호 안의 값들을 차례대로 읽는다. */
    status = parse_values_list(&cursor, &command->insert_cmd);
    if (status != PARSE_OK) {
        return status;
    }

    cursor = skip_spaces(cursor);
    /* 값 목록 뒤에는 닫는 괄호가 와야 한다. */
    if (*cursor != ')') {
        return PARSE_INVALID_INSERT;
    }
    cursor++;

    cursor = skip_spaces(cursor);
    /* 문장 끝 세미콜론 검사 */
    if (*cursor != ';') {
        return PARSE_MISSING_SEMICOLON;
    }
    cursor++;

    /* 현재 구현은 한 번에 한 문장만 허용한다. */
    if (!is_blank_string(cursor)) {
        return PARSE_INVALID_INSERT;
    }

    command->type = CMD_INSERT;
    return PARSE_OK;
}

/* parse_select()는
   SELECT * FROM users [WHERE column = value]; 형태를 해석한다. */
ParseStatus parse_select(const char *input, Command *command) {
    /* 1차 구현에서는 SELECT * FROM users [WHERE ...]; 만 허용한다. */
    /* SELECT도 같은 방식으로 cursor를 조금씩 앞으로 옮기며 해석한다. */
    const char *cursor = input;
    ParseStatus status;

    /* SELECT 키워드 확인 */
    if (!match_keyword(&cursor, "SELECT")) {
        return PARSE_INVALID_SELECT;
    }

    cursor = skip_spaces(cursor);
    /* 현재는 SELECT * 만 허용한다. */
    if (*cursor != '*') {
        return PARSE_INVALID_SELECT;
    }
    cursor++;

    /* FROM 키워드 확인 */
    if (!match_keyword(&cursor, "FROM")) {
        return PARSE_INVALID_SELECT;
    }

    /* 테이블 이름 읽기 */
    if (!parse_identifier_token(&cursor, command->select_cmd.table, sizeof(command->select_cmd.table))) {
        return PARSE_INVALID_SELECT;
    }

    cursor = skip_spaces(cursor);
    /* WHERE가 있으면 조건 parsing을 추가로 수행한다. */
    if (starts_with_ignore_case(cursor, "WHERE")) {
        status = parse_condition(cursor, &command->select_cmd.condition);
        if (status != PARSE_OK) {
            return status;
        }

        /* 조건 뒤 세미콜론 위치까지 커서를 이동한다. */
        while (*cursor != '\0' && *cursor != ';') {
            cursor++;
        }
    } else {
        /* WHERE가 없으면 전체 조회 */
        /* has_condition = 0 이면 executor는 조건 없는 SELECT로 처리한다. */
        command->select_cmd.condition.has_condition = 0;
    }

    cursor = skip_spaces(cursor);
    if (*cursor != ';') {
        return PARSE_MISSING_SEMICOLON;
    }
    cursor++;

    /* SELECT 문장도 세미콜론 뒤에 다른 내용이 있으면 거절한다. */
    if (!is_blank_string(cursor)) {
        return PARSE_INVALID_SELECT;
    }

    command->type = CMD_SELECT;
    return PARSE_OK;
}

/* parse_condition()은 WHERE 절만 따로 읽어서
   column, value, has_condition 정보를 Condition 구조체에 채운다. */
ParseStatus parse_condition(const char *input, Condition *condition) {
    /* WHERE column = value 형태만 해석한다. */
    /* cursor는 WHERE 절 안에서 column, =, value를 차례대로 읽는다. */
    const char *cursor = input;
    ParseStatus status;

    if (condition == NULL) {
        return PARSE_INVALID_WHERE;
    }

    /* WHERE 키워드 확인 */
    if (!match_keyword(&cursor, "WHERE")) {
        return PARSE_INVALID_WHERE;
    }

    /* 왼쪽 피연산자인 칼럼 이름 읽기 */
    if (!parse_identifier_token(&cursor, condition->column, sizeof(condition->column))) {
        return PARSE_INVALID_WHERE;
    }
    /* 성공하면 condition->column 안에 id, name 같은 컬럼명이 저장된다. */

    cursor = skip_spaces(cursor);
    /* 1차 구현은 = 비교만 지원 */
    if (*cursor != '=') {
        return PARSE_INVALID_WHERE;
    }
    cursor++;

    /* 오른쪽 비교값 읽기 */
    status = parse_value_token(&cursor, &condition->value);
    if (status != PARSE_OK) {
        return status == PARSE_INVALID_INSERT ? PARSE_INVALID_WHERE : status;
    }

    cursor = skip_spaces(cursor);
    /* WHERE 뒤에는 더 복잡한 식 없이 바로 세미콜론이 와야 한다. */
    if (*cursor != ';') {
        return PARSE_INVALID_WHERE;
    }

    condition->has_condition = 1;
    /* has_condition을 1로 바꾸는 순간부터
       executor는 이 SELECT를 "조건 있는 조회"로 해석한다. */
    return PARSE_OK;
}

/* init_command()는 Command 구조체를 사용하기 전에
   안전한 초기 상태로 만드는 함수다. */
void init_command(Command *command) {
    if (command == NULL) {
        return;
    }

    /* 이전 값이 남지 않도록 구조체 전체를 0으로 초기화한다. */
    memset(command, 0, sizeof(*command));
    command->type = CMD_INVALID;
}

/* free_command()는 parser가 malloc 해 둔 문자열들을
   Command 사용이 끝난 뒤 한 번에 정리하는 함수다. */
void free_command(Command *command) {
    int i;

    if (command == NULL) {
        return;
    }

    for (i = 0; i < MAX_VALUE_COUNT; i++) {
        /* parse_value_token()이 만든 문자열 메모리를
           command 사용이 끝난 시점에 하나씩 해제한다. */
        free(command->insert_cmd.values[i]);
        command->insert_cmd.values[i] = NULL;
    }

    /* WHERE 비교값도 heap에 있을 수 있으므로 함께 정리한다. */
    free(command->select_cmd.condition.value);
    command->select_cmd.condition.value = NULL;
    command->select_cmd.condition.has_condition = 0;
}

/* skip_spaces()는 parser가 다음 토큰을 읽기 전에
   앞쪽 공백을 건너뛰기 위해 사용하는 helper 함수다. */
static const char *skip_spaces(const char *s) {
    /* parser 안에서는 공백을 많이 건너뛰므로 작은 helper로 분리했다. */
    while (s != NULL && *s != '\0' && isspace((unsigned char) *s)) {
        s++;
    }
    return s;
}

/* match_keyword()는 현재 위치에서
   INSERT, SELECT, FROM 같은 예약어가 정확히 오는지 검사하고,
   성공하면 input 포인터를 그 뒤로 이동시킨다. */
static int match_keyword(const char **input, const char *keyword) {
    /* 키워드는 대소문자를 구분하지 않고 비교한다.
       예: SELECT, select, SeLeCt 모두 허용 */
    size_t len;
    const char *cursor;
    size_t i;

    if (input == NULL || *input == NULL || keyword == NULL) {
        return 0;
    }

    cursor = skip_spaces(*input);
    len = strlen(keyword);

    /* keyword 길이만큼 한 글자씩 비교 */
    for (i = 0; i < len; i++) {
        if (toupper((unsigned char) cursor[i]) != toupper((unsigned char) keyword[i])) {
            return 0;
        }
    }

    /* SELECTOR 같은 더 긴 단어를 SELECT로 오인하지 않게 막는다. */
    if (isalnum((unsigned char) cursor[len]) || cursor[len] == '_') {
        return 0;
    }

    *input = cursor + len;
    return 1;
}

/* parse_identifier_token()은 users, id, username 같은 식별자 한 개를 읽어
   목적 버퍼 dest에 복사하는 함수다. */
static int parse_identifier_token(const char **input, char *dest, size_t dest_size) {
    /* users, id 같은 식별자를 읽어오는 함수다. */
    const char *cursor;
    /* len은 지금까지 식별자에 복사한 글자 수다. */
    size_t len = 0;

    if (input == NULL || *input == NULL || dest == NULL || dest_size == 0) {
        return 0;
    }

    cursor = skip_spaces(*input);
    /* 식별자는 영문자 또는 _ 로 시작해야 한다. */
    if (!isalpha((unsigned char) *cursor) && *cursor != '_') {
        return 0;
    }

    /* 식별자에 들어갈 수 있는 문자를 끝까지 복사한다. */
    while ((isalnum((unsigned char) cursor[len]) || cursor[len] == '_') && len + 1 < dest_size) {
        dest[len] = cursor[len];
        len++;
    }

    /* 버퍼보다 더 긴 식별자는 잘못된 입력으로 본다. */
    if (isalnum((unsigned char) cursor[len]) || cursor[len] == '_') {
        return 0;
    }

    dest[len] = '\0';
    *input = cursor + len;
    return 1;
}

/* parse_value_token()은 VALUES(...)나 WHERE 값 자리에서
   숫자 또는 따옴표 문자열 한 개를 읽어 heap 문자열로 만들어 돌려준다. */
static ParseStatus parse_value_token(const char **input, char **out_value) {
    /* 값 하나를 읽는다.
       - 숫자: 123
       - 문자열: 'Kim' 또는 "Kim" */
    /* cursor는 현재 읽는 위치,
       start는 값이 실제로 시작한 위치를 기억한다. */
    const char *cursor;
    const char *start;
    /* len은 start부터 cursor 전까지 몇 글자를 읽었는지 센다. */
    size_t len = 0;
    /* quote는 문자열이 ' 로 시작했는지 " 로 시작했는지 기억한다. */
    char quote = '\0';
    char *copy;

    if (input == NULL || *input == NULL || out_value == NULL) {
        return PARSE_INVALID_INSERT;
    }

    *out_value = NULL;
    cursor = skip_spaces(*input);
    if (*cursor == '\0') {
        return PARSE_INVALID_INSERT;
    }

    if (*cursor == '\'' || *cursor == '"') {
        quote = *cursor;
        cursor++;
        start = cursor;

        /* 따옴표로 시작했으면 같은 따옴표가 나올 때까지 문자열로 읽는다. */
        while (*cursor != '\0' && *cursor != quote) {
            /* 줄바꿈이 나오면 문자열이 중간에 끊긴 것으로 처리 */
            if (*cursor == '\n' || *cursor == '\r') {
                return PARSE_UNTERMINATED_STRING;
            }
            cursor++;
            len++;
        }

        /* 닫는 따옴표가 없으면 문자열이 끝나지 않은 것이다. */
        if (*cursor != quote) {
            return PARSE_UNTERMINATED_STRING;
        }

        copy = duplicate_range(start, len);
        if (copy == NULL) {
            return PARSE_OUT_OF_MEMORY;
        }
        /* copy는 이제 command 구조체 안에 오래 보관할 실제 문자열 메모리다. */

        cursor++;
        cursor = skip_spaces(cursor);

        /* 문자열 다음에는 쉼표, 닫는 괄호, 세미콜론만 허용한다. */
        if (*cursor != ',' && *cursor != ')' && *cursor != ';' && *cursor != '\0') {
            free(copy);
            return PARSE_UNSUPPORTED_QUOTED_FORMAT;
        }

        *out_value = copy;
        *input = cursor;
        return PARSE_OK;
    }

    start = cursor;
    /* 따옴표 없는 값은 숫자처럼 취급하고
       공백/쉼표/괄호 전까지 읽는다. */
    while (*cursor != '\0' && *cursor != ',' && *cursor != ')' && *cursor != ';' && !isspace((unsigned char) *cursor)) {
        cursor++;
        len++;
    }

    if (len == 0) {
        return PARSE_INVALID_INSERT;
    }

    copy = duplicate_range(start, len);
    if (copy == NULL) {
        return PARSE_OUT_OF_MEMORY;
    }

    *out_value = copy;
    *input = cursor;
    return PARSE_OK;
}

/* parse_values_list()는 INSERT VALUES 괄호 안에서
   쉼표로 구분된 여러 값을 순서대로 읽어 InsertCommand 안에 채운다. */
static ParseStatus parse_values_list(const char **input, InsertCommand *insert_cmd) {
    /* INSERT 안의 값 목록을 , 기준으로 여러 개 읽는다. */
    const char *cursor = *input;
    ParseStatus status;
    /* index는 지금 몇 번째 값을 읽는지 나타낸다.
       username=0, name=1, age=2 ... 식으로 대응된다. */
    int index = 0;

    cursor = skip_spaces(cursor);
    /* VALUES()처럼 비어 있는 경우도 감지한다. */
    if (*cursor == ')') {
        insert_cmd->value_count = 0;
        *input = cursor;
        return PARSE_OK;
    }

    /* 쉼표를 기준으로 값들을 하나씩 읽는다. */
    while (*cursor != '\0' && *cursor != ')') {
        if (index >= USER_INPUT_VALUE_COUNT) {
            return PARSE_INVALID_INSERT;
        }

        status = parse_value_token(&cursor, &insert_cmd->values[index]);
        if (status != PARSE_OK) {
            return status;
        }

        index++;
        /* 값 하나를 성공적으로 읽었으므로 다음 칸으로 이동한다. */
        cursor = skip_spaces(cursor);

        /* 닫는 괄호를 만나면 값 목록이 끝난다. */
        if (*cursor == ')') {
            break;
        }

        /* 닫는 괄호가 아니라면 다음 값 앞의 쉼표가 꼭 필요하다. */
        if (*cursor != ',') {
            return PARSE_INVALID_INSERT;
        }
        cursor++;
        cursor = skip_spaces(cursor);
    }

    cursor = skip_spaces(cursor);
    /* 마지막 값 뒤에 쉼표만 있고 끝난 경우를 막는다. */
    if (*cursor == ',') {
        return PARSE_INVALID_INSERT;
    }

    /* executor가 값 개수를 검사할 수 있도록 실제 개수를 저장한다. */
    insert_cmd->value_count = index;
    *input = cursor;
    return PARSE_OK;
}

/* duplicate_range()는 입력 문자열의 일부 구간만 잘라
   새 heap 문자열로 복사해 두고 싶을 때 사용하는 helper다. */
static char *duplicate_range(const char *start, size_t len) {
    char *copy;

    copy = (char *) malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, start, len);
    copy[len] = '\0';
    return copy;
}
