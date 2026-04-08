# Lesson 10. CSAPP Chapter 3와의 연결

## 이 문서의 목적

CSAPP Chapter 3에서 배우는 함수 호출, stack frame, 배열 접근, 포인터, 분기, 루프가 이 저장소 코드에서 실제로 어디에 나타나는지 연결합니다.

## 함수 호출과 stack frame

가장 먼저 떠올릴 함수 체인은 `main`입니다.

```text
main
  -> read_entire_file
  -> tokenize_sql
  -> parse_program
  -> execute_program
```

이 호출이 일어날 때마다 call stack에 새 frame이 쌓입니다.

예를 들어 `execute_select_statement`가 `row_matches_where`를 호출하면:

- 호출자 지역 변수
- 피호출자 매개변수
- return address

같은 정보가 stack frame 단위로 관리됩니다.

## local variable과 stack storage

아래 같은 변수들은 대부분 stack에 놓입니다.

- `main`의 `TokenArray tokens`, `Program program`, `SqlError error`
- `tokenize_sql`의 `index`, `line`, `column`
- `parse_program`의 `Parser parser`
- `execute_select_statement`의 `projection_indices`, `where_index`

반면 heap은 `malloc`, `calloc`, `realloc`으로 직접 만든 동적 데이터가 차지합니다.

즉 이 프로젝트는 stack과 heap의 차이를 보기 좋은 예제입니다.

## 배열 접근

배열 접근은 거의 모든 모듈에 나옵니다.

- `tokens->items[index]`
- `program->statements[index]`
- `schema->columns[index]`
- `table.rows[row_index].values[column_index]`

CSAPP 관점에서는 이 모든 것이 "기준 주소 + offset 계산"입니다.

특히 `projection_indices[projection_index]`로 실제 CSV 셀을 고르는 부분은 배열 기반 간접 참조가 어떻게 이어지는지 보여줍니다.

## 포인터 arithmetic를 생각해볼 만한 곳

직접 `ptr + 1` 같은 코드가 많지는 않지만, 개념적으로는 계속 사용됩니다.

- `sql_strndup(sql_text + start, index - start)`
- `trimmed + 6`, `trimmed + 8`
- `colon + 1`
- `line + start`

즉 포인터 arithmetic가 문자열 slice 처리에 숨어 있습니다.

## 조건 분기와 루프

CSAPP Chapter 3에서 중요한 분기와 루프도 이 저장소에 풍부합니다.

- tokenizer의 문자 종류 분기
- parser의 INSERT / SELECT 분기
- executor의 statement 타입 분기
- CSV row 순회
- projection 컬럼 순회

이 코드를 읽을 때 "이 if/switch/for가 기계 수준에서는 jump와 비교 명령으로 바뀌겠구나"를 떠올려보면 좋습니다.

## 구조체 메모리 레이아웃

예를 들어 `Token`, `LiteralValue`, `Statement`, `TableSchema`, `CsvTable`은 모두 구조체입니다.

CSAPP 관점에서는 구조체도 결국 메모리 상에 순서대로 놓인 필드들의 묶음입니다.

중요한 점:

- 구조체 안에 포인터 필드가 있으면 실제 문자열 데이터는 다른 메모리 위치에 있습니다.
- 즉 구조체를 값 복사해도 내부 heap 데이터는 깊은 복사가 아닙니다.

이 점이 `append_statement`, `append_literal` 같은 함수에서 특히 중요합니다.

## 문자열과 버퍼 처리의 위험

CSAPP가 강조하는 버퍼 위험은 이 프로젝트에서도 그대로 드러납니다.

- 길이 계산 실수
- 널 종료 누락
- 잘못된 `realloc`
- 해제 후 접근
- `strtol` 입력 검증 누락

그래서 `sql_strndup`, `sql_trim_copy`, `sql_strip_trailing_newline` 같은 작은 helper가 안전성에 큰 역할을 합니다.

## printf 디버깅과 gdb

현재 저장소는 Docker/devcontainer 안에 `gdb`와 `valgrind`가 준비돼 있습니다.

실전 팁:

- tokenizer 문제
  token type, lexeme, line/column을 printf로 찍어봅니다.
- parser 문제
  현재 token과 `parser.current`를 확인합니다.
- executor 문제
  `where_index`, `projection_indices`, 현재 row 값을 찍어봅니다.

더 깊게 보고 싶다면:

```bash
make CFLAGS='-std=c11 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=200809L -g'
gdb ./sql_processor
```

## 초보자 실수 포인트

- heap에 있는 데이터도 함수가 끝나면 자동으로 사라진다고 생각하기
- 구조체 값 복사가 deep copy라고 생각하기
- 배열 인덱스 접근과 포인터 개념을 별개로 생각하기

## 더 생각해볼 질문

- `row_matches_where`에서 일어나는 비교를 어셈블리 수준으로 상상하면 어떤 분기들이 생길까?
- `TokenArray`와 `Program`의 `realloc`은 메모리 이동 관점에서 어떤 비용을 가질까?
- 구조체 필드 순서를 바꾸면 정렬(alignment)과 패딩은 어떻게 달라질까?

## 생각 확장 방향

이 저장소는 CSAPP Chapter 3의 개념을 "작고 읽을 수 있는 C 코드"에 연결하기 좋은 예제입니다. 특히 stack vs heap, 배열 접근, 분기, 문자열 버퍼 위험을 실제 함수 흐름 안에서 볼 수 있습니다.
