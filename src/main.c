/*
 * 파일 목적:
 *   CLI 진입점으로서 파일 모드, -e 문자열 실행 모드, REPL 모드를 분기하고
 *   공통 SQL 실행 파이프라인을 호출한다.
 *
 * 전체 흐름에서 위치:
 *   사용자는 이 파일을 통해 세 가지 입력 모드 중 하나를 선택하고,
 *   main은 입력을 SQL 문자열로 정리한 뒤 공통 실행 모듈에 넘긴다.
 *
 * 주요 내용:
 *   - CLI 모드 선택
 *   - REPL 입력 누적
 *   - usage / help 출력
 *   - 공통 SQL 실행 경로 호출
 *
 * 초보자 포인트:
 *   입력 방식이 여러 개여도 tokenizer, parser, executor를 다시 구현하지 않고
 *   같은 실행 경로를 재사용하는 것이 중요한 설계 포인트다.
 *
 * 메모리 / 문자열 주의점:
 *   REPL은 사용자가 입력한 줄을 동적 버퍼에 누적하므로,
 *   반복문이 끝날 때 그 버퍼를 free해야 한다.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "error.h"
#include "sql_runner.h"

static void print_usage(const char *program_name);
static int repl_line_matches_command(const char *line, const char *command_a, const char *command_b);
static int repl_line_is_blank(const char *line);
static int append_repl_input(char **buffer, size_t *length, size_t *capacity, const char *line, SqlError *error);
static int repl_buffer_has_complete_statement(const char *buffer);
static void print_repl_help(void);
static int run_repl(const char *schema_dir, const char *data_dir);

/*
 * 목적:
 *   사용 가능한 세 가지 실행 모드를 사용법과 함께 출력한다.
 *
 * 입력:
 *   program_name - 현재 실행 파일 이름.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   파일 모드, -e 모드, REPL 모드 예시를 stderr에 출력한다.
 *
 * 초보자 주의:
 *   usage 메시지는 사용자가 어떤 입력 형식을 기대해야 하는지 알려주는 중요한 문서 역할도 한다.
 */
static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s <sql_file>\n", program_name);
    fprintf(stderr, "  %s -e \"<sql_text>\"\n", program_name);
    fprintf(stderr, "  %s --repl\n", program_name);
}

/*
 * 목적:
 *   REPL에서 입력 줄이 특정 명령어와 같은지 검사한다.
 *
 * 입력:
 *   line - 사용자가 입력한 한 줄.
 *   command_a, command_b - 허용할 두 명령 문자열.
 *
 * 반환:
 *   일치하면 1, 아니면 0.
 *
 * 동작 요약:
 *   trim한 뒤 완전히 같은 문자열인지 비교한다.
 *
 * 초보자 주의:
 *   공백이 섞여 들어와도 명령을 인식할 수 있도록 trim 후 비교한다.
 */
static int repl_line_matches_command(const char *line, const char *command_a, const char *command_b) {
    char *trimmed;
    int matches;

    trimmed = sql_trim_copy(line);
    if (trimmed == NULL) {
        return 0;
    }

    matches = strcmp(trimmed, command_a) == 0 || strcmp(trimmed, command_b) == 0;
    free(trimmed);
    return matches;
}

/*
 * 목적:
 *   REPL에서 사용자가 빈 줄만 입력했는지 검사한다.
 *
 * 입력:
 *   line - 사용자가 입력한 한 줄.
 *
 * 반환:
 *   빈 줄이면 1, 아니면 0.
 *
 * 동작 요약:
 *   공백을 건너뛴 뒤 바로 문자열 끝이 나오면 빈 줄로 본다.
 *
 * 초보자 주의:
 *   빈 줄을 무시하면 REPL에서 괜히 문법 오류가 나지 않아 사용감이 좋아진다.
 */
static int repl_line_is_blank(const char *line) {
    const unsigned char *cursor;

    if (line == NULL) {
        return 1;
    }

    cursor = (const unsigned char *)line;
    while (*cursor != '\0') {
        if (!isspace(*cursor)) {
            return 0;
        }
        ++cursor;
    }

    return 1;
}

/*
 * 목적:
 *   REPL 입력 한 줄을 누적 버퍼 뒤에 이어 붙인다.
 *
 * 입력:
 *   buffer - 누적 SQL 버퍼 포인터.
 *   length - 현재 버퍼 길이.
 *   capacity - 현재 버퍼 용량.
 *   line - 새로 읽은 입력 줄.
 *   error - 메모리 실패 시 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   필요 시 realloc으로 버퍼를 늘리고 새 줄을 뒤에 복사한다.
 *
 * 초보자 주의:
 *   REPL은 여러 줄을 모아 하나의 SQL 문자열로 실행할 수 있으므로 동적 버퍼가 필요하다.
 */
static int append_repl_input(char **buffer, size_t *length, size_t *capacity, const char *line, SqlError *error) {
    char *resized_buffer;
    size_t line_length;
    size_t required_length;
    size_t new_capacity;

    line_length = strlen(line);
    required_length = *length + line_length + 1;

    if (required_length > *capacity) {
        new_capacity = *capacity == 0 ? 128 : *capacity;
        while (new_capacity < required_length) {
            new_capacity *= 2;
        }

        resized_buffer = (char *)realloc(*buffer, new_capacity);
        if (resized_buffer == NULL) {
            sql_error_set(error, "ERROR: out of memory while buffering REPL input");
            return 0;
        }

        *buffer = resized_buffer;
        *capacity = new_capacity;
    }

    memcpy(*buffer + *length, line, line_length + 1);
    *length += line_length;
    return 1;
}

/*
 * 목적:
 *   현재 REPL 버퍼가 "실행해도 되는 완성된 SQL 입력"인지 검사한다.
 *
 * 입력:
 *   buffer - 지금까지 누적한 REPL SQL 문자열.
 *
 * 반환:
 *   마지막 의미 있는 문자가 문자열 바깥의 세미콜론이면 1, 아니면 0.
 *
 * 동작 요약:
 *   작은따옴표 안팎을 추적하며, 문자열 바깥의 마지막 비공백 문자를 확인한다.
 *
 * 초보자 주의:
 *   단순히 문자열 전체에 ';'가 있는지만 보면,
 *   중간 완성 문장 뒤에 아직 덜 입력된 문장이 붙어 있는 경우를 잘못 처리할 수 있다.
 */
static int repl_buffer_has_complete_statement(const char *buffer) {
    int in_string;
    char last_non_space_outside_string;
    size_t index;

    if (buffer == NULL) {
        return 0;
    }

    in_string = 0;
    last_non_space_outside_string = '\0';

    for (index = 0; buffer[index] != '\0'; ++index) {
        if (buffer[index] == '\'') {
            in_string = !in_string;
            continue;
        }

        if (!in_string && !isspace((unsigned char)buffer[index])) {
            last_non_space_outside_string = buffer[index];
        }
    }

    return !in_string && last_non_space_outside_string == ';';
}

/*
 * 목적:
 *   REPL 안에서 사용할 짧은 도움말을 출력한다.
 *
 * 입력:
 *   없음.
 *
 * 반환:
 *   없음.
 *
 * 동작 요약:
 *   입력 가능한 SQL과 종료 명령을 한 번에 보여준다.
 *
 * 초보자 주의:
 *   REPL은 문서를 안 보고 바로 시도하는 경우가 많아서 help가 특히 유용하다.
 */
static void print_repl_help(void) {
    printf("REPL commands:\n");
    printf("  Enter SQL ending with ';' to execute it.\n");
    printf("  Type 'exit', 'quit', '.exit', or '.quit' to leave the REPL.\n");
    printf("  Example: SELECT * FROM users WHERE age >= 29;\n");
}

/*
 * 목적:
 *   반복 입력을 받아 세미콜론 단위로 SQL을 실행하는 REPL 모드를 수행한다.
 *
 * 입력:
 *   schema_dir - schema 디렉터리 경로.
 *   data_dir - data 디렉터리 경로.
 *
 * 반환:
 *   정상 종료 시 0, 입력/메모리 오류 시 1.
 *
 * 동작 요약:
 *   프롬프트를 보여주고 입력을 누적하다가 완성된 SQL이 되면 run_sql_text를 호출한다.
 *
 * 초보자 주의:
 *   프롬프트는 stderr로 보내서, 실제 SELECT 결과 stdout과 섞이지 않게 했다.
 */
static int run_repl(const char *schema_dir, const char *data_dir) {
    char line_buffer[SQL_LINE_BUFFER_SIZE];
    char *sql_buffer;
    size_t sql_length;
    size_t sql_capacity;
    SqlError error;

    sql_buffer = NULL;
    sql_length = 0;
    sql_capacity = 0;
    sql_error_clear(&error);

    while (1) {
        fprintf(stderr, "%s", sql_length == 0 ? "sql> " : "...> ");
        fflush(stderr);

        if (fgets(line_buffer, sizeof(line_buffer), stdin) == NULL) {
            if (ferror(stdin)) {
                sql_error_set(&error, "ERROR: failed to read from stdin");
                sql_error_print(&error);
                free(sql_buffer);
                return 1;
            }

            if (sql_length != 0) {
                sql_error_set(&error, "ERROR: incomplete SQL statement in REPL input");
                sql_error_print(&error);
                free(sql_buffer);
                return 1;
            }

            fputc('\n', stderr);
            break;
        }

        if (sql_length == 0) {
            if (repl_line_is_blank(line_buffer)) {
                continue;
            }

            if (repl_line_matches_command(line_buffer, "exit", ".exit") ||
                repl_line_matches_command(line_buffer, "quit", ".quit")) {
                break;
            }

            if (repl_line_matches_command(line_buffer, "help", ".help")) {
                print_repl_help();
                continue;
            }
        }

        sql_error_clear(&error);
        if (!append_repl_input(&sql_buffer, &sql_length, &sql_capacity, line_buffer, &error)) {
            sql_error_print(&error);
            free(sql_buffer);
            return 1;
        }

        if (!repl_buffer_has_complete_statement(sql_buffer)) {
            continue;
        }

        sql_error_clear(&error);
        if (!run_sql_text(sql_buffer, schema_dir, data_dir, &error)) {
            sql_error_print(&error);
        }

        if (sql_buffer != NULL) {
            sql_buffer[0] = '\0';
        }
        sql_length = 0;
    }

    free(sql_buffer);
    return 0;
}

/*
 * 목적:
 *   SQL 처리기 프로그램의 시작점 역할을 한다.
 *
 * 입력:
 *   argc, argv - 커맨드라인 인자 개수와 값.
 *
 * 반환:
 *   성공 시 0, 실패 시 1.
 *
 * 동작 요약:
 *   CLI 인자를 보고 파일 모드, -e 모드, REPL 모드 중 하나를 선택한다.
 *
 * 초보자 주의:
 *   실행 모드는 달라도 결국 run_sql_text 또는 run_sql_file로 같은 파이프라인을 사용한다.
 */
int main(int argc, char **argv) {
    SqlError error;
    sql_error_clear(&error);

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "--repl") == 0) {
        return run_repl("schema", "data");
    }

    if (argc == 3 && strcmp(argv[1], "-e") == 0) {
        if (!run_sql_text(argv[2], "schema", "data", &error)) {
            sql_error_print(&error);
            return 1;
        }
        return 0;
    }

    if (argc == 2) {
        if (!run_sql_file(argv[1], "schema", "data", &error)) {
            sql_error_print(&error);
            return 1;
        }
        return 0;
    }

    print_usage(argv[0]);
    if (argc > 1) {
        sql_error_set(&error, "ERROR: invalid CLI arguments");
        sql_error_print(&error);
    }
    return 1;
}
