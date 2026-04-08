# Lesson 08. 메모리, 포인터, 구조체, 문자열

## 이 문서의 목적

이 저장소에서 가장 C다운 부분인 메모리 소유권, 포인터, 문자열 복사, free 흐름을 실제 함수 기준으로 설명합니다.

## 이 저장소에서 메모리 소유권이 중요한 이유

이 프로젝트는 아래처럼 "길이를 미리 모르는 것"이 많습니다.

- SQL 파일 길이
- 토큰 수
- SELECT 컬럼 수
- INSERT 값 수
- schema 컬럼 수
- CSV row 수

그래서 거의 모든 중간 결과가 동적 메모리입니다.

## 주요 소유권 맵

- `read_entire_file`
  SQL 전체 문자열을 할당하고 `main`이 `free(sql_text)`로 해제
- `tokenize_sql`
  `TokenArray.items`와 각 `Token.lexeme`을 할당하고 `free_token_array`가 해제
- `parse_program`
  `Program.statements`, 문자열들, literal 배열을 할당하고 `free_program`이 해제
- `load_schema`
  `TableSchema.columns`, 각 컬럼 이름을 할당하고 `free_schema`가 해제
- `csv_load_table`
  `CsvTable.header`, `rows`, 각 셀 문자열을 할당하고 `free_csv_table`이 해제

이 소유권 맵을 기억하면 구조가 훨씬 명확해집니다.

## 문자열 helper가 왜 따로 있는가

`src/common.c`에는 아래 helper가 있습니다.

- `sql_strdup`
- `sql_strndup`
- `sql_lowercase_copy`
- `sql_uppercase_copy`
- `sql_trim_copy`

이 함수를 둔 이유:

- C 표준 라이브러리만으로는 의도가 덜 드러납니다.
- 문자열 복사 규칙을 한 곳에 모을 수 있습니다.
- tokenizer, parser, schema가 같은 스타일로 동작합니다.

## realloc이 많이 등장하는 이유

토큰, statement, 컬럼, row는 개수를 미리 모릅니다. 그래서 아래 함수들이 `realloc`을 사용합니다.

- `push_token`
- `append_statement`
- `append_literal`
- `append_selected_column`
- `append_column`
- `append_csv_row`

이 함수들을 읽을 때는 항상 두 가지를 보세요.

- capacity 또는 new_count를 어떻게 계산하는가
- 실패했을 때 누가 메모리를 정리하는가

## free 함수가 따로 있는 이유

이 프로젝트는 중첩 구조가 많아서 free도 중첩됩니다.

- `free_token_array`
- `free_literal_value`
- `free_statement`
- `free_program`
- `free_schema`
- `free_csv_table`

예를 들어 `Program`은 statement 배열만 free하면 끝이 아닙니다. 각 statement 안의 문자열, values 배열, WHERE literal까지 같이 정리해야 합니다.

## 포인터가 헷갈리는 실제 지점

- `char *`
  문자열 하나
- `char **`
  문자열 배열
- `Token *`
  구조체 배열
- `CsvRow *`
  row 배열

특히 `CsvTable`은 "행 배열 안에 셀 문자열 배열"이 있으므로 2단계 해제가 필요합니다.

## 문자열과 숫자의 왕복

이 프로젝트는 CSV를 문자열로 저장하므로 INT도 파일 안에서는 텍스트입니다.

흐름:

```text
SQL int literal -> LiteralValue.int_value
             -> csv_append_row에서 "%ld"로 파일 저장
             -> csv_load_table에서 문자열로 읽기
             -> row_matches_where에서 strtol로 다시 long 변환
```

이 왕복을 이해하면 "왜 타입 검사가 executor에 있는지"도 함께 이해됩니다.

## 자주 생기는 버그 유형

- 널 종료 문자 누락
- 부분 실패 후 메모리 누수
- `realloc` 실패 처리 누락
- 문자열 길이 계산 실수
- 해제 후 다시 접근

## 디버깅 포인트

- `make CFLAGS='-std=c11 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=200809L -g'`
  gdb에 더 유리한 바이너리를 만들 수 있습니다.
- Docker 환경에는 `gdb`, `valgrind`가 들어 있으므로 컨테이너 안에서 바로 점검할 수 있습니다.
- 세그폴트가 나면 "누가 이 포인터를 만들었고 누가 해제해야 하는가"를 먼저 추적하세요.

## 초보자 실수 포인트

- string literal `"Alice"`와 heap 복사본 `"Alice"`를 같은 것으로 생각하기
- free 함수가 "배열만" 정리한다고 착각하기
- parser 실패 경로처럼 중간 생성 상태를 과소평가하기

## 더 생각해볼 질문

- Rust처럼 소유권 모델이 있는 언어라면 이 프로젝트는 어떤 부분이 더 쉬워질까?
- `CsvTable`을 linked list로 바꾸면 free 구조는 더 쉬워질까, 더 어려워질까?
- 지금의 free 함수들을 일반화하면 어떤 장단점이 생길까?

## 생각 확장 방향

이 Lesson은 C가 왜 "동작만 하면 끝"이 아닌지를 보여줍니다. 이 프로젝트를 통해 소유권과 해제 규칙을 습관처럼 보는 연습을 하기 좋습니다.
