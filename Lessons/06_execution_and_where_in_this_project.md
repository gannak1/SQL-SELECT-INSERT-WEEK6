# Lesson 06. 이 프로젝트의 실행기와 WHERE

## 이 문서의 목적

`src/executor.c`가 parser 결과를 실제 데이터 처리로 바꾸는 방법, 그리고 WHERE가 어디서 어떻게 평가되는지 설명합니다.

## 실제 코드 위치

- `include/executor.h`
  `execute_program`
- `src/executor.c`
  실행기 본체
- `src/printer.c`
  출력 포맷
- `src/schema.c`
  schema 검증
- `src/csv_storage.c`
  CSV 저장/읽기

## execute_program의 역할

`execute_program`은 `Program` 안의 `Statement`들을 순서대로 실행합니다.

- INSERT면 `execute_insert_statement`
- SELECT면 `execute_select_statement`

이 함수는 "첫 에러에서 바로 중단"하는 단순 정책을 사용합니다.

## INSERT 실행 흐름

`execute_insert_statement`는 아래 순서로 동작합니다.

1. `load_schema`
2. 값 개수와 컬럼 개수 비교
3. 각 값 타입과 컬럼 타입 비교
4. `csv_append_row`
5. `print_insert_success`

즉 INSERT는 생각보다 단순합니다. 핵심은 schema를 먼저 읽어 타입과 개수를 검증하는 것입니다.

## SELECT 실행 흐름

`execute_select_statement`는 조금 더 많은 일을 합니다.

1. `load_schema`
2. `build_projection`
3. `validate_where_clause`
4. `csv_load_table`
5. `print_result_header`
6. 각 row 순회
7. `row_matches_where`
8. projection 적용 후 `print_result_row`
9. `print_result_footer`

이 순서가 WHERE와 projection이 어디서 생기는지 보여줍니다.

## build_projection

`build_projection`은 SELECT 결과에 어떤 컬럼을 출력할지 숫자 인덱스 배열로 바꿉니다.

- `SELECT *`
  0, 1, 2, ... 전체 컬럼 인덱스 생성
- `SELECT id, name`
  schema에서 `id`, `name` 위치를 찾아 인덱스로 변환

여기서 없는 컬럼을 선택하면 바로:

```text
ERROR: column not found: ...
```

## validate_where_clause

WHERE가 있으면 executor는 실제 실행 전에 아래를 먼저 검사합니다.

1. WHERE 컬럼이 schema에 존재하는가
2. literal 타입이 컬럼 타입과 맞는가
3. TEXT 컬럼에서 허용되지 않는 비교를 쓰지 않았는가

예:

- `age = 'Alice'`
  `ERROR: type mismatch in WHERE for column age`
- `name > 'Bob'`
  `ERROR: unsupported TEXT comparison operator: >`

이 검사를 먼저 하는 이유는 row를 읽기 전에 의미 오류를 빠르게 잡기 위해서입니다.

## row_matches_where

실제 조건 평가는 `row_matches_where`가 합니다.

- INT 컬럼
  CSV 셀 문자열을 `strtol`로 long으로 바꾼 뒤 `=`, `!=`, `<`, `>`, `<=`, `>=` 비교
- TEXT 컬럼
  `strcmp` 기반 `=` 또는 `!=` 비교

즉 CSV는 문자열로 읽히지만, WHERE 평가 시점에 다시 타입을 해석합니다.

## projection이 실제로 적용되는 순간

row가 WHERE를 통과하면 executor는 `projection_indices`를 사용해 필요한 셀만 뽑아 `print_result_row`에 넘깁니다.

그래서 다음 둘은 분리된 단계입니다.

- row를 남길지 결정
- 남긴 row에서 어떤 컬럼만 보여줄지 결정

이 분리가 WHERE와 projection의 차이를 이해하는 핵심입니다.

## printer와 executor의 역할 분리

executor는 "무엇을 출력할지"를 계산합니다.
printer는 "어떤 형식으로 출력할지"를 담당합니다.

현재 printer 함수:

- `print_insert_success`
- `print_result_header`
- `print_result_row`
- `print_result_footer`

이 분리 덕분에 출력 포맷 회귀 테스트가 쉬워집니다.

## 초보자 실수 포인트

- WHERE를 parser가 평가한다고 생각하기
- projection이 CSV를 읽기 전에 적용된다고 생각하기
- TEXT 비교도 숫자처럼 `<`, `>`가 된다고 생각하기

현재 구현은 분명합니다.

- parser는 구조만 만듭니다.
- executor가 schema를 보고 WHERE를 평가합니다.
- projection은 WHERE 뒤에 적용됩니다.

## 디버깅 포인트

- 컬럼이 없다는 에러
  `build_projection` 또는 `validate_where_clause`
- WHERE 결과가 이상함
  `row_matches_where`
- row 수는 맞는데 컬럼 순서가 이상함
  `projection_indices`
- 출력 형식만 틀림
  `src/printer.c`

## 더 생각해볼 질문

- WHERE에 `AND`, `OR`가 들어오면 `Condition`과 `row_matches_where`는 어떻게 바뀔까?
- `ORDER BY`를 넣으려면 row를 모두 메모리에 올린 뒤 어느 단계가 추가돼야 할까?
- 지금은 `find_column_index`가 선형 탐색인데, 큰 schema에서는 무엇으로 바꾸고 싶을까?

## 생각 확장 방향

현재 executor는 의도적으로 "선형 스캔 + 간단한 조건"에 머뭅니다. 덕분에 SQL 처리기의 의미 단계가 잘 보이고, 이후 최적화나 다중 조건 처리를 어느 지점에 붙일지 명확해집니다.
