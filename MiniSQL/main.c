#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "printer.h"
#include "storage.h"
#include "types.h"

// 세미콜론이 나올 때까지 여러 줄 입력을 이어 붙여 한 문장으로 만든다.
static int read_query_input(char* input, size_t input_size) {
    char line[MAX_INPUT_LENGTH];
    size_t current_length = 0;

    input[0] = '\0';

    while (1) {
        printf(current_length == 0 ? "\nminiSQL> " : "      -> ");

        // 조건걸면서 input 같이 받기
        if (fgets(line, sizeof(line), stdin) == NULL) {
            return 0;
        }

        line[strcspn(line, "\r\n")] = '\0';
        

        // 현재 길이가 0이면서, is_exit_command 요청을 보낼 경우
        if (current_length == 0 && is_exit_command(line)) {
            strncpy_s(input, input_size, line, _TRUNCATE);
            return 1;
        }

        // 입력값 확인
        if (current_length > 0) {
            // input_size(512)보다 현재 입력된 값이 더 많으면
            // 정상적인 쿼리문 아니니까 0 리턴
            // 추가적으로 마지막에 \0 넣어줘야 해서 511 + 1
            if (current_length + 1 >= input_size) {
                return 0;
            }
            input[current_length++] = ' ';
            input[current_length] = '\0';
        }

        // 여태 입력한 길이(current_length) + 현재 줄(strlen(line)) 이 input_size보다 클 경우
        if (current_length + strlen(line) >= input_size) {
            return 0;
        }

        // 여기에서 값 넣어줌
        // input == \0인데, 여기부터 값을 넣음.
        // 목적 변수 / 버퍼 크기 (512) / 붙일 문자열
        strcat_s(input, input_size, line);
        current_length = strlen(input);
        

        if (strchr(line, ';') != NULL) {
            return 1;
        }
    }
}

int main(void) {
    char input[MAX_INPUT_LENGTH];
    Query query;
    InsertCommand insert_command;
    char error_message[256];

    setlocale(LC_ALL, "");

    printf("------------------\n");
    printf("미니SQL 프로젝트\n");
    printf("------------------\n\n");
    printf("SQL같은 타입의 쿼리 작성 후, 세미콜론 (;) 을 붙여주세요\n");
    printf("EXIT을 입력하거나 프로그램을 종료해주세요.\n");

    while (1) {
        if (!read_query_input(input, sizeof(input))) {
            printf("\n입력값 에러.\n");
            return 1;
        }

        if (is_exit_command(input)) {
            printf("미니SQL 종료.\n");
            break;
        }

        if (!parse_query(input, &query)) {
            printf("\n읽기 실패\n");
            printf("이렇게 해보세요:\n");
            printf("  SELECT name FROM table;\n");
            printf("  SELECT * FROM table WHERE login_id = 'hong123';\n");
            printf("  SELECT id FROM table WHERE id > 1;\n");
            printf("  INSERT 1, 'hong123', 'Hong', 'pass1234', '2026-04-08 18:30:00';\n");
            continue;
        }

        print_query(&query);

        if (query.type == QUERY_INSERT) {
            if (!parse_insert_command(&query, &insert_command, error_message, sizeof(error_message))) {
                printf("\nINSERT 해석 실패: %s\n", error_message);
                continue;
            }

            print_insert_command(&insert_command);

            if (!append_insert_to_csv(&insert_command, error_message, sizeof(error_message))) {
                printf("\nCSV 저장 실패: %s\n", error_message);
                continue;
            }

            printf("\nINSERT 저장 완료: result\\table.csv\n");
            continue;
        }

        if (query.type == QUERY_SELECT) {
            if (!execute_select_query(&query, error_message, sizeof(error_message))) {
                printf("\nSELECT 실행 실패: %s\n", error_message);
            }
        }
    }

    return 0;
}