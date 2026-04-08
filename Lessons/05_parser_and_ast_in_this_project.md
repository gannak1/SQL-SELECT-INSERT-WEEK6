# Lesson 05. 이 프로젝트의 parser와 AST

## 이 문서의 목적

`src/parser.c`가 `TokenArray`를 어떻게 `Program`과 `Statement` 구조체로 바꾸는지, 그리고 왜 이런 구조가 executor에 유리한지 설명합니다.

## 실제 코드 위치

- `include/ast.h`
  `LiteralValue`, `Condition`, `InsertStatement`, `SelectStatement`, `Statement`, `Program`
- `include/parser.h`
  `parse_program` 선언
- `src/parser.c`
  parser 구현
- `src/ast.c`
  AST 비슷한 구조체 해제 함수

## 이 프로젝트에서 AST가 의미하는 것

이 프로젝트는 복잡한 트리 AST를 만들지는 않지만, 최소한 아래 정보는 구조체로 분리해 둡니다.

- INSERT
  테이블 이름, 값 목록
- SELECT
  전체 컬럼인지 여부, 선택 컬럼 목록, 테이블 이름, WHERE 여부
- WHERE
  컬럼 이름, 연산자, literal 값

즉 "그냥 토큰 배열"이 아니라 "실행하기 쉬운 구조"를 만드는 것이 parser의 역할입니다.

## 핵심 구조체

- `LiteralValue`
  `DATA_TYPE_INT` 또는 `DATA_TYPE_TEXT` 값을 담습니다.
- `Condition`
  WHERE의 `column operator value`를 표현합니다.
- `InsertStatement`
  `table_name`, `values`, `value_count`
- `SelectStatement`
  `select_all`, `selected_columns`, `table_name`, `has_where`, `where`
- `Statement`
  `STMT_INSERT` 또는 `STMT_SELECT`
- `Program`
  여러 SQL 문장을 담는 동적 배열

## parse_program 흐름

`parse_program`은 EOF 전까지 반복합니다.

1. 현재 토큰이 `INSERT`인지 `SELECT`인지 확인
2. 맞는 전용 parser 호출
3. 결과 `Statement`를 `Program` 배열에 추가
4. 다음 문장으로 이동

이 덕분에 SQL 파일 하나에 여러 문장을 넣을 수 있습니다.

## INSERT 파싱

`parse_insert_statement`는 아래 순서를 강제합니다.

1. `INSERT`
2. `INTO`
3. 테이블 이름
4. `VALUES`
5. `(`
6. 값 목록
7. `)`
8. `;`

값 목록은 `parse_literal`을 반복 호출해 `InsertStatement.values`에 쌓습니다.

## SELECT 파싱

`parse_select_statement`는 아래 순서를 강제합니다.

1. `SELECT`
2. `*` 또는 컬럼 목록
3. `FROM`
4. 테이블 이름
5. optional `WHERE`
6. `;`

WHERE가 있으면 아래 세 요소를 읽습니다.

- 컬럼 이름
- 연산자
- literal 값

연산자는 `parse_operator`가 문자열에서 `OperatorType` enum으로 변환합니다.

## parser helper 함수가 중요한 이유

`src/parser.c`에는 아래 helper가 많습니다.

- `peek_token`
- `advance_token`
- `check_token`
- `match_token`
- `check_keyword`
- `match_keyword`
- `expect_token`
- `expect_keyword`

이 함수들이 있는 이유는 parser 코드를 "의미 있는 문장"처럼 읽히게 하기 위해서입니다.

예를 들어 `expect_keyword(parser, "FROM")`은 단순 비교보다 훨씬 읽기 쉽습니다.

## 왜 executor가 토큰 대신 Statement를 읽는가

만약 executor가 토큰 배열을 직접 읽는다면:

- INSERT와 SELECT 분기가 더 복잡해집니다.
- WHERE 해석 로직이 실행기 안으로 섞입니다.
- 에러 메시지가 더 난해해집니다.
- Lessons에서 설명하기 어려워집니다.

현재 구조에서는 parser가 의미를 먼저 정리해두고, executor는 그 결과만 소비합니다.

## 메모리 관점에서 parser가 하는 일

parser는 문자열과 배열을 새로 할당합니다.

- 테이블 이름 복사
- SELECT 컬럼 이름 복사
- string literal 복사
- `Program.statements` 동적 확장
- `InsertStatement.values` 동적 확장

그래서 `src/ast.c`의 `free_program`, `free_statement`, `free_literal_value`가 꼭 필요합니다.

## 초보자 실수 포인트

- parser가 타입 검사까지 한다고 생각하기
- WHERE가 있으면 executor가 더 이상 schema를 안 봐도 된다고 생각하기
- `Program`을 트리라고만 생각해 과하게 어렵게 느끼기

실제로는:

- parser는 문법과 구조화까지 담당합니다.
- 타입 검사와 컬럼 존재 검사는 executor가 합니다.
- 현재 `Program`은 "작은 AST 비슷한 구조체 모음"으로 이해하면 충분합니다.

## 디버깅 포인트

- 세미콜론 누락
  `expected ';'` 에러를 확인합니다.
- 잘못된 토큰 순서
  `expect_token`, `expect_keyword`가 어디서 실패했는지 봅니다.
- WHERE 파싱 이상
  `parse_operator`, `parse_literal` 순서를 확인합니다.

`tests/test_parser.c`는 이 계층을 바로 검증하는 가장 좋은 출발점입니다.

## 더 생각해볼 질문

- `AND`, `OR`를 넣으려면 `Condition`이 어떻게 바뀌어야 할까?
- SELECT 목록에 함수 호출이나 alias를 넣으려면 어떤 자료구조가 더 필요할까?
- 지금의 `Program`을 진짜 expression tree로 바꾸는 시점은 언제일까?

## 생각 확장 방향

현재 parser는 작은 범위에 맞춰 직선적으로 설계돼 있습니다. 그래서 초보자가 읽기 좋고, 동시에 나중에 expression tree를 도입할 때 어떤 부분을 확장해야 할지도 비교적 분명하게 보입니다.
