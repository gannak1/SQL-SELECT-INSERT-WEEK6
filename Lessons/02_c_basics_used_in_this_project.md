# Lesson 02. 이 프로젝트에서 쓰인 C 기초

## 이 문서의 목적

이 프로젝트를 읽으려면 알아야 하는 C 기초 요소를 실제 코드 위치와 함께 정리합니다.

## 실제 코드에서 등장하는 C 요소

- 문자열
  `src/common.c`, `src/tokenizer.c`, `src/schema.c`, `src/csv_storage.c`
- 구조체
  `include/token.h`, `include/ast.h`, `include/schema.h`, `include/csv_storage.h`, `include/error.h`
- enum
  `TokenType`, `DataType`, `OperatorType`, `StatementType`
- 동적 배열
  `TokenArray`, `Program`, `InsertStatement.values`, `CsvTable.rows`
- 파일 I/O
  `read_entire_file`, `csv_append_row`, `csv_load_table`
- 포인터와 메모리 해제
  `free_token_array`, `free_program`, `free_schema`, `free_csv_table`

## 문자열이 중요한 곳

가장 문자열 작업이 많은 파일은 아래입니다.

- `src/common.c`
  `sql_strdup`, `sql_strndup`, `sql_lowercase_copy`, `sql_uppercase_copy`, `sql_trim_copy`
- `src/tokenizer.c`
  identifier, string literal, operator lexeme을 잘라 복사합니다.
- `src/schema.c`
  `table=...`, `columns=...` 줄을 해석합니다.
- `src/csv_storage.c`
  CSV 한 줄을 셀 문자열 배열로 나눕니다.

이 프로젝트에서 문자열은 "값"일 뿐 아니라 "구조를 만드는 재료"입니다.

## 구조체가 중요한 곳

이 프로젝트는 구조체를 단계별 결과 저장소로 사용합니다.

- `Token`
  문자 단위 입력을 토큰 단위로 바꾼 결과
- `LiteralValue`
  `INT` 또는 `TEXT` literal
- `Condition`
  WHERE의 `column operator value`
- `Statement`
  INSERT 또는 SELECT 한 문장
- `Program`
  여러 statement 묶음
- `TableSchema`
  schema 파일 해석 결과
- `CsvTable`
  CSV 파일을 메모리에 올린 결과

즉, 구조체가 곧 파이프라인의 중간 산출물입니다.

## 포인터가 중요한 곳

- `char *`
  거의 모든 문자열이 포인터로 다뤄집니다.
- `Token *items`
  토큰 동적 배열
- `Statement *statements`
  statement 동적 배열
- `LiteralValue *values`
  INSERT 값 동적 배열
- `char **selected_columns`
  SELECT projection 컬럼 목록
- `CsvTable`
  2차원 문자열 구조

포인터가 많아지는 이유는 "길이를 컴파일 시점에 알 수 없는 데이터"가 많기 때문입니다.

## 헤더와 소스 분리

이 저장소는 `.h`와 `.c`를 분리합니다.

- 헤더
  자료형과 함수 약속을 보여줍니다.
- 소스
  실제 동작을 담습니다.

예를 들어 `include/parser.h`는 `parse_program`을 선언하고, 구현은 `src/parser.c`에 있습니다.

## 동적 메모리가 중요한 이유

SQL 파일 길이, 토큰 수, 컬럼 수, row 수는 미리 고정돼 있지 않습니다. 그래서 `malloc`, `calloc`, `realloc`을 적극적으로 사용합니다.

대표 예시:

- `push_token`
- `append_statement`
- `append_literal`
- `append_column`
- `append_csv_row`

## 초보자 실수 포인트

- 문자열 상수와 heap 문자열을 구분하지 못하기
- `realloc` 실패 시 기존 포인터를 잃어버리기
- 구조체 안쪽 문자열을 먼저 free하지 않고 바깥 배열만 free하기
- `strtol` 뒤 `end_ptr` 검사를 빼먹기

## 디버깅 포인트

- 세그멘테이션 폴트
  대개 NULL 포인터 또는 해제된 메모리 접근입니다.
- 이상한 문자열 출력
  널 종료 누락이나 잘못된 길이 복사를 의심합니다.
- row 수는 맞는데 값이 틀림
  projection 인덱스와 CSV split 결과를 함께 봅니다.

## 더 생각해볼 질문

- 왜 C에서는 메모리 소유권을 더 신경 써야 할까?
- 왜 `char *`와 `char **`가 섞이는 순간부터 코드 난도가 확 올라갈까?
- 같은 기능을 C++ `std::string`, `std::vector`로 쓰면 어떤 부분이 쉬워질까?

## 생각 확장 방향

이 프로젝트를 다 읽고 나면 "C는 왜 작은 helper 함수와 free 함수가 중요한지"를 몸으로 이해하기 좋습니다.
