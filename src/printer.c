#include "printer.h"

#include <stdio.h>

static void print_csv_string(const char *value);

/* print_user_row()는 values[0]~values[5]에 들어 있는 한 사용자 행을
   콘솔에 CSV 모양으로 다시 출력한다. */
void print_user_row(char *const values[USER_COLUMN_COUNT]) {
    /* SELECT 결과를 콘솔에 다시 CSV 형태로 보여준다. */
    /* values는 CSV 6칸을 순서대로 담고 있고,
       여기서는 그 순서를 유지한 채 다시 한 줄로 출력한다. */
    if (values == NULL) {
        return;
    }

    /* 숫자 칼럼은 그대로,
       문자열 칼럼은 따옴표 포함 CSV 스타일로 출력한다. */
    printf("%s,", values[0]);
    print_csv_string(values[1]);
    putchar(',');
    print_csv_string(values[2]);
    printf(",%s,", values[3]);
    print_csv_string(values[4]);
    putchar(',');
    print_csv_string(values[5]);
    putchar('\n');
}

/* print_select_header()는 SELECT 결과 위에
   컬럼 이름 한 줄을 보여 주기 위한 함수다. */
void print_select_header(void) {
    /* 결과를 읽기 쉽게 하기 위해 컬럼 이름을 먼저 출력한다. */
    puts("id,username,name,age,phone,email");
}

/* print_rows_selected()는 SELECT가 끝난 뒤
   몇 행이 선택되었는지 요약해서 보여 준다. */
void print_rows_selected(int count) {
    printf("%d rows selected\n", count);
}

/* print_message()는 INSERT 성공 같은 일반 안내 메시지를 출력한다. */
void print_message(const char *message) {
    if (message != NULL) {
        puts(message);
    }
}

/* print_error()는 사용자에게 보여 줄 오류 문장을 출력하는 helper다. */
void print_error(const char *message) {
    if (message != NULL) {
        puts(message);
    }
}

/* print_csv_string()은 문자열 하나를 CSV 스타일로 화면에 출력한다.
   즉 큰따옴표로 감싸고, 내부 큰따옴표는 두 번 찍는다. */
static void print_csv_string(const char *value) {
    /* 출력도 CSV처럼 보이게 문자열은 큰따옴표로 감싼다. */
    /* cursor는 value 문자열을 처음부터 끝까지 한 글자씩 훑는 포인터다. */
    const char *cursor = value;

    /* NULL이면 빈 문자열처럼 ""를 출력한다. */
    if (value == NULL) {
        fputs("\"\"", stdout);
        return;
    }

    /* 문자열 시작 큰따옴표 */
    putchar('"');
    while (*cursor != '\0') {
        /* 화면 출력에서도 따옴표는 두 번 써서 CSV 모양을 유지한다. */
        if (*cursor == '"') {
            putchar('"');
        }
        putchar(*cursor);
        cursor++;
    }
    /* 문자열 끝 큰따옴표 */
    putchar('"');
}
