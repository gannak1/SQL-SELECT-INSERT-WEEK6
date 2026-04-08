#include "repl.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "executor.h"
#include "parser.h"
#include "utils.h"

static void print_parse_error(ParseStatus status);
static void print_exec_error(ExecStatus status);
static int has_complete_statement(const char *buffer);
static int append_input_line(char **buffer, size_t *capacity, const char *line);
static int read_input_line(char **line_out);

/* run_repl()은 MiniSQL REPL의 메인 루프다.
   프롬프트를 보여 주고, 입력을 읽고, 세미콜론이 나올 때까지 문장을 모은 뒤,
   완성된 MiniSQL 문장을 parser/executor로 넘긴다. */
int run_repl(void) {
    /* line: 사용자로부터 방금 새로 입력받은 한 줄*/
    char *line = NULL;
    /* buffer: 여러 줄 입력을 하나의 Mini-SQL 문장으로 모아두는 동적 공간.
    세미콜론이 나올 때까지 이어 붙인 전체 문장이다. */
    char *buffer = NULL;
    /* buffer_capacity는 buffer가 현재 몇 바이트까지 담을 수 있는지 기록한다. */
    size_t buffer_capacity = 0;
    /* collecting이 1이면 아직 문장이 덜 끝났다는 뜻이라서
       다음 프롬프트를 MiniSQL> 대신 ...> 로 보여준다. */
    int collecting = 0;
    /* read_status는 read_input_line() 결과를 담는다.
       1은 정상 입력, 0은 EOF, -1은 메모리 오류다. */
    int read_status;

    /* 처음에는 너무 큰 메모리를 잡지 않고,
       "문장 몇 개는 충분히 담을 수 있는" 시작 크기만 확보한다.
       나중에 입력이 길어지면 realloc()으로 점점 늘린다. */
    buffer = (char *) malloc(INITIAL_BUFFER_CAPACITY);
    if (buffer == NULL) {
        /* buffer를 아예 만들 수 없으면
           이후 REPL 전체가 동작할 수 없으므로 바로 종료한다. */
        puts("Error: out of memory");
        return 1;
    }
    /* buffer는 C 문자열로 사용할 예정이므로
       첫 글자를 '\0'로 두어 "지금은 빈 문자열" 상태로 만든다. */
    buffer[0] = '\0';
    /* 현재 buffer의 총 크기를 별도 변수에 기억해 두어야
       append_input_line()에서 공간 부족 여부를 판단할 수 있다. */
    buffer_capacity = INITIAL_BUFFER_CAPACITY;

    while (1) {
        /* collecting이 1이면 아직 문장이 덜 끝난 상태이므로 ...> 를,
           0이면 새 문장을 시작하는 상태이므로 MiniSQL> 를 고른다.
           삼항 연산자 (조건 ? 참일 때 값 : 거짓일 때 값) 를 사용한 표현이다. */
        fputs(collecting ? CONTINUATION_PROMPT : PROMPT_TEXT, stdout);
        /* 프롬프트가 버퍼에만 머무르지 않고 터미널에 바로 보이게 한다. */
        fflush(stdout);

        /* 사용자 입력 한 줄을 heap 메모리에 읽어 온다.
           read_input_line() 안에서 malloc/realloc이 일어나므로
           성공하면 line은 나중에 반드시 free해야 한다. */
        read_status = read_input_line(&line);
        if (read_status == 0) {
            /* Ctrl + D 같은 EOF가 들어오면 조용히 종료한다. */
            fputc('\n', stdout);
            /* run_repl()이 직접 만든 buffer는 여기서 해제한다. */
            free(buffer);
            return 0;
        }
        if (read_status < 0) {
            puts("Error: out of memory");
            free(buffer);
            return 1;
        }

        /* 입력 줄 끝의 개행과 앞뒤 공백을 정리한다. */
        /* fgets가 아니라 직접 문자 단위로 읽었기 때문에
           마지막 '\n'과 앞뒤 공백을 여기서 정리한다. */
        trim_newline(line);
        trim_spaces(line);

        /* 빈 줄은 명령으로 보지 않고 다시 입력받는다. */
        if (is_blank_string(line)) {
            free(line);
            /* 빈 줄 메모리를 정리한 뒤
               다음 루프에서 실수로 재사용하지 않게 NULL로 만든다. */
            line = NULL;
            continue;
        }

        /* 종료 명령은 파서까지 보내지 않고 REPL 단계에서 바로 처리한다. */
        if (is_exit_command(line)) {
            free(line);
            free(buffer);
            return 0;
        }

        /* 여러 줄 입력을 하나의 버퍼에 이어 붙인다. */
        /* 이번 줄을 기존 누적 문장 뒤에 붙인다.
           세미콜론이 아직 없으면 다음 줄도 계속 이어 붙이게 된다. */
        if (append_input_line(&buffer, &buffer_capacity, line) != 0) {
            puts("Error: out of memory");
            free(line);
            free(buffer);
            return 1;
        }

        free(line);
        /* line 내용은 이미 buffer로 복사했으므로
           포인터를 비워 두는 편이 흐름을 따라가기 쉽다. */
        line = NULL;

        /* 세미콜론이 아직 없으면 문장이 덜 끝난 것으로 보고 다음 줄을 기다린다. */
        if (!has_complete_statement(buffer)) {
            collecting = 1;
            continue;
        }

        /* 문장이 완성되면 파싱과 실행 단계로 넘긴다. */
        process_input_line(buffer);

        /* 한 문장을 처리했으니 누적 버퍼를 비운다. */
        /* 새 버퍼를 다시 malloc하지 않고
           첫 글자만 '\0'로 바꿔 빈 문자열 상태로 재사용한다. */
        buffer[0] = '\0';
        collecting = 0;
    }
}

/* process_input_line()은 REPL이 완성된 MiniSQL 한 문장을 넘겨주면
   그 문장을 parser로 해석하고 executor로 실행까지 이어 주는 연결 함수다.
   즉, "문자열 입력"을 "실제 동작"으로 바꾸는 중간 관문 역할을 한다. */
int process_input_line(const char *line) {
    /* 이 함수 안에서 "진짜 파서 역할"을 하는 것은 parse_command() 호출이다.
       아래의 Command command 변수는 파서 그 자체가 아니라,
       parse_command()가 해석 결과를 채워 넣을 빈 구조체 상자다. */
    Command command;
    /* parse_status는 문법 해석 결과,
       exec_status는 실제 실행 결과를 담는다. */
    ParseStatus parse_status;
    ExecStatus exec_status;

    /* command 안 포인터들이 쓰레기값을 갖지 않도록 먼저 초기화한다. */
    init_command(&command);

    /* 여기서 parse_command()가 실제 parser로 동작하면서
       line 문자열을 읽고 command 구조체 안에
       "INSERT인지 SELECT인지, 테이블명은 무엇인지, 값은 무엇인지"를 채운다. */
    parse_status = parse_command(line, &command);
    if (parse_status != PARSE_OK) {
        print_parse_error(parse_status);
        /* parse 중간에 일부 값이 malloc됐을 수 있으므로
           실패해도 free_command()를 호출해 안전하게 정리한다. */
        free_command(&command);
        return -1;
    }

    /* parse가 끝난 command 구조체를 executor에 넘기면
       이제부터는 "문자열 해석"이 아니라 "실제 저장/조회 실행" 단계가 된다. */
    exec_status = execute_command(&command);
    if (exec_status != EXEC_OK && exec_status != EXEC_NO_ROWS_FOUND) {
        print_exec_error(exec_status);
        /* parse가 성공했다면 command 안에는 동적 메모리가 들어 있을 수 있으므로
           실행 실패여도 반드시 free_command()를 호출한다. */
        free_command(&command);
        return -1;
    }

    /* SELECT는 문법상 맞더라도 결과가 0건일 수 있으므로
       오류가 아니라 별도 상태로 처리한다. */
    if (exec_status == EXEC_NO_ROWS_FOUND) {
        puts("No rows found");
    }

    free_command(&command);
    return 0;
}

/* is_exit_command()는 사용자가 입력한 한 줄이
   실제 SQL 문장이 아니라 REPL 종료 명령(exit/quit)인지 판별한다. */
int is_exit_command(const char *line) {
    /* exit123 같은 문자열은 종료 명령이 아니므로
       뒤 문자가 문자열 끝인지까지 함께 확인한다. */
    return (starts_with_ignore_case(line, "exit") && line[4] == '\0')
        || (starts_with_ignore_case(line, "quit") && line[4] == '\0');
}

/* print_parse_error()는 parser가 돌려준 내부 상태 코드를
   사람이 읽을 수 있는 오류 메시지로 바꿔 출력한다. */
static void print_parse_error(ParseStatus status) {
    /* parser 내부 상태 코드를 사용자용 문장으로 바꾼다. */
    switch (status) {
        case PARSE_MISSING_SEMICOLON:
            puts("Error: missing semicolon");
            break;
        case PARSE_UNSUPPORTED_COMMAND:
            puts("Error: unsupported command");
            break;
        case PARSE_INVALID_INSERT:
            puts("Error: invalid INSERT syntax");
            break;
        case PARSE_INVALID_SELECT:
            puts("Error: invalid SELECT syntax");
            break;
        case PARSE_INVALID_WHERE:
            puts("Error: invalid WHERE syntax");
            break;
        case PARSE_UNTERMINATED_STRING:
            puts("Error: unterminated string literal");
            break;
        case PARSE_UNSUPPORTED_QUOTED_FORMAT:
            puts("Error: unsupported quoted string format");
            break;
        case PARSE_OUT_OF_MEMORY:
            puts("Error: out of memory");
            break;
        case PARSE_OK:
        default:
            break;
    }
}

/* print_exec_error()는 executor가 돌려준 실행 오류 코드를
   사용자용 에러 문장으로 바꿔 출력한다. */
static void print_exec_error(ExecStatus status) {
    /* executor 내부 상태 코드도 같은 방식으로 출력한다. */
    switch (status) {
        case EXEC_UNSUPPORTED_TABLE:
            puts("Error: unsupported table");
            break;
        case EXEC_UNSUPPORTED_SELECT_COLUMNS:
            puts("Error: unsupported SELECT columns");
            break;
        case EXEC_UNSUPPORTED_WHERE_CONDITION:
            puts("Error: unsupported WHERE condition");
            break;
        case EXEC_INSERT_VALUE_COUNT_MISMATCH:
            puts("Error: INSERT expects 5 values");
            break;
        case EXEC_INVALID_ID:
            puts("Error: invalid numeric value for id");
            break;
        case EXEC_INVALID_AGE:
            puts("Error: invalid numeric value for age");
            break;
        case EXEC_DATA_FILE_NOT_FOUND:
            puts("Error: data file not found");
            break;
        case EXEC_READ_FAILED:
            puts("Error: failed to read data file");
            break;
        case EXEC_WRITE_FAILED:
            puts("Error: failed to write data file");
            break;
        case EXEC_MEMORY_ERROR:
            puts("Error: out of memory");
            break;
        case EXEC_OK:
        case EXEC_NO_ROWS_FOUND:
        default:
            break;
    }
}

/* has_complete_statement()는 현재까지 모은 buffer 안에
   "문자열 바깥의 세미콜론"이 있는지 검사한다.
   즉, REPL이 지금 문장을 실행해도 되는지 판단하는 함수다. */
static int has_complete_statement(const char *buffer) {
    /* 따옴표 바깥에서 세미콜론이 나왔을 때만
       "문장이 끝났다"라고 판단한다. */
    /* in_single / in_double은 현재 cursor가
       작은따옴표 문자열 안인지, 큰따옴표 문자열 안인지를 기억한다. */
    int in_single = 0;
    int in_double = 0;
    /* cursor는 buffer를 처음부터 끝까지 한 글자씩 훑는 포인터다. */
    const char *cursor;

    if (buffer == NULL) {
        return 0;
    }

    cursor = buffer;
    while (*cursor != '\0') {
        /* 작은따옴표 안으로 들어가면
           다음 작은따옴표가 나올 때까지는 문자열 내부라고 본다. */
        if (*cursor == '\'' && !in_double) {
            in_single = !in_single;
        /* 큰따옴표도 같은 원리로 추적한다. */
        } else if (*cursor == '"' && !in_single) {
            in_double = !in_double;
        /* 따옴표 바깥의 세미콜론만 문장 종료로 인정한다. */
        } else if (*cursor == ';' && !in_single && !in_double) {
            /* 세미콜론을 찾았으므로,
               이제 뒤에 공백 말고 다른 문자가 남아 있는지 확인한다. */
            cursor++;
            /* 세미콜론 뒤에는 공백만 남아 있어야 한다. */
            while (*cursor != '\0') {
                if (!isspace((unsigned char) *cursor)) {
                    return 1;
                }
                cursor++;
            }
            return 1;
        }
        cursor++;
    }

    return 0;
}

/* append_input_line()은 방금 입력한 한 줄을
   기존 누적 문장 buffer 뒤에 이어 붙이는 함수다.
   여러 줄 입력을 한 개의 MiniSQL 문장으로 합치기 위해 사용한다. */
static int append_input_line(char **buffer, size_t *capacity, const char *line) {
    /* buffer_len: 지금까지 누적된 문장 길이
       line_len: 방금 새로 받은 한 줄 길이
       needed: 이번 줄까지 붙였을 때 총 필요한 바이트 수 */
    size_t buffer_len;
    size_t line_len;
    size_t needed;
    char *new_buffer;

    if (buffer == NULL || *buffer == NULL || capacity == NULL || line == NULL) {
        return -1;
    }

    buffer_len = strlen(*buffer);
    line_len = strlen(line);
    /* +1은 마지막 문자열 종료 문자 '\0' 자리다. */
    needed = buffer_len + line_len + 1;
    if (buffer_len > 0) {
        /* 줄과 줄 사이에 공백 한 칸을 넣기 때문에
           그 공백 자리도 같이 계산한다. */
        needed++;
    }

    if (needed > *capacity) {
        while (needed > *capacity) {
            /* 버퍼는 한 번에 2배씩 키워서 realloc 횟수를 줄인다. */
            *capacity *= 2;
        }

        /* realloc은 기존 내용을 보존한 채 더 큰 공간을 요청한다.
           성공하면 새 주소를 돌려줄 수 있으므로
           반드시 임시 포인터 new_buffer로 먼저 받아야 안전하다. */
        new_buffer = (char *) realloc(*buffer, *capacity);
        if (new_buffer == NULL) {
            return -1;
        }
        /* realloc 성공 후에만 원래 buffer 포인터를 새 주소로 갱신한다. */
        *buffer = new_buffer;
    }

    /* 이미 앞줄이 있으면 공백 하나를 넣어서 자연스럽게 이어 붙인다. */
    if (buffer_len > 0) {
        (*buffer)[buffer_len++] = ' ';
        /* 공백을 직접 넣었으므로 문자열 끝 표시도 다시 맞춰 둔다. */
        (*buffer)[buffer_len] = '\0';
    }

    /* line 전체를 buffer 뒤에 그대로 이어 붙인다. */
    /* line_len + 1을 복사하는 이유는
       문자들뿐 아니라 마지막 '\0'까지 같이 옮겨서
       buffer가 여전히 올바른 C 문자열이 되게 하기 위해서다. */
    memcpy(*buffer + buffer_len, line, line_len + 1);
    return 0;
}

/* read_input_line()은 키보드(stdin)에서 사용자가 친 한 줄을 읽어
   heap 메모리에 새 문자열로 만들어 line_out에 돌려준다.
   입력 길이를 미리 알 수 없으므로 malloc/realloc으로 버퍼를 키워 가며 읽는다. */
static int read_input_line(char **line_out) {
    /* ch는 현재 읽은 문자 1개를 담는다.
       EOF(-1)도 함께 표현해야 해서 int를 사용한다. */
    int ch;
    /* buffer는 이번 한 줄 전체를 담는 heap 메모리다. */
    char *buffer;
    /* capacity는 buffer 전체 크기,
       length는 현재까지 실제로 채운 글자 수다. */
    size_t capacity = INITIAL_BUFFER_CAPACITY;
    size_t length = 0;
    char *new_buffer;

    if (line_out == NULL) {
        return -1;
    }

    *line_out = NULL;
    /* 한 줄 입력을 담을 시작 버퍼를 만든다.
       처음에는 작은 크기로 시작하고,
       긴 줄이 들어오면 아래 while 안에서 점점 늘린다. */
    buffer = (char *) malloc(capacity);
    if (buffer == NULL) {
        return -1;
    }

    while ((ch = fgetc(stdin)) != EOF) {
        if (length + 2 > capacity) {
            /* 새 문자 1개와 마지막 '\0' 1개 자리를 함께 확보한다. */
            /* 공간이 부족하면 현재 크기의 2배로 늘린다.
               이렇게 하면 입력이 길어져도 너무 자주 realloc하지 않아도 된다. */
            capacity *= 2;
            new_buffer = (char *) realloc(buffer, capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return -1;
            }
            /* 이제부터는 더 큰 새 버퍼를 계속 사용한다. */
            buffer = new_buffer;
        }

        /* 입력 문자 1개를 buffer 끝에 저장하고
           저장한 뒤에는 length를 1 늘린다. */
        buffer[length++] = (char) ch;
        if (ch == '\n') {
            /* 개행을 만나면 한 줄 입력이 끝난 것이다. */
            break;
        }
    }

    if (ch == EOF && length == 0) {
        /* 아무 글자도 읽지 못한 EOF는
           "정상 종료 신호"로 해석한다. */
        free(buffer);
        return 0;
    }

    /* C 문자열 규칙에 맞게 마지막에 '\0'를 붙인다. */
    buffer[length] = '\0';
    *line_out = buffer;
    return 1;
}
