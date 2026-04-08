# TEST_PLAN
## 구현 검증 계획

이 문서는 Codex가 어떤 테스트를 만들어야 하는지 안내합니다.

---

## 1. 목표

테스트의 목적은 단순히 “돌아간다”를 확인하는 것이 아니라,  
**이 저장소가 정의한 스펙을 실제로 만족하는지** 검증하는 것입니다.

---

## 2. 필수 Makefile target

최소한 아래를 제공하세요.

- `make`
- `make clean`
- `make test`

권장:
- `make run-demo`

---

## 3. 단위 테스트 대상

### tokenizer
검증할 것:
- keyword 인식
- identifier 인식
- integer literal 인식
- string literal 인식
- operator 인식
- comma / parenthesis / semicolon 인식

예:
```sql
SELECT id, name FROM users WHERE age >= 20;
```

### parser
검증할 것:
- INSERT statement 구조화
- SELECT * 구조화
- SELECT column list 구조화
- WHERE 단일 조건 구조화
- 잘못된 문법 거부

### executor / WHERE
검증할 것:
- INT 비교
- TEXT의 `=` / `!=`
- 잘못된 TEXT 비교 연산자 처리

### csv storage
검증할 것:
- header 생성
- row append
- row read
- 빈 파일 / 없는 파일 처리

---

## 4. 통합 테스트 대상

아래 sample SQL 파일을 사용해 통합 테스트를 구성하세요.

- `sample_sql/01_users_bootstrap.sql`
- `sample_sql/02_users_projection.sql`
- `sample_sql/03_users_where_int.sql`
- `sample_sql/04_users_where_text.sql`
- `sample_sql/05_books_demo.sql`
- `sample_sql/06_error_unknown_column.sql`
- `sample_sql/07_error_type_mismatch.sql`
- `sample_sql/08_error_unsupported_text_compare.sql`
- `sample_sql/09_full_demo.sql`

### 통합 테스트 기본 흐름
1. `data/` 디렉터리 초기화
2. bootstrap SQL 실행
3. 후속 SELECT 실행
4. 기대 출력과 비교
5. 에러 케이스 실행
6. non-zero 종료 코드와 에러 메시지 확인

---

## 5. 권장 테스트 전략

### 전략 A. 작은 함수부터 검증
tokenizer, parser, operator evaluation을 먼저 테스트

### 전략 B. sample SQL로 end-to-end 검증
실제 CLI 실행으로 검증

### 전략 C. 에러 케이스를 꼭 포함
초보 프로젝트는 정상 경로만 테스트하고 끝나기 쉬움  
하지만 이 과제는 오류 처리도 중요함

---

## 6. 최소 필수 케이스

### 정상 케이스
- INSERT 1건
- INSERT 여러 건
- SELECT *
- SELECT 일부 컬럼
- WHERE INT
- WHERE TEXT equality
- 빈 결과

### 에러 케이스
- 없는 테이블
- 없는 컬럼
- 값 개수 불일치
- 타입 불일치
- TEXT에 `>` 비교
- 세미콜론 누락 또는 문법 오류

---

## 7. 출력 검증 방식

가능하면 아래 방식으로 검증하세요.

- 프로그램 stdout를 파일로 저장
- `expected_output/*.txt`와 비교
- 에러 케이스는 stderr 또는 combined output 확인

---

## 8. 권장 추가 검증

가능하다면 아래도 좋습니다.

- `valgrind`로 메모리 누수 확인
- 여러 문장 파일 실행 확인
- 같은 테이블에 여러 번 INSERT 후 SELECT
- data 파일이 없는 상태에서 SELECT

---

## 9. 최종 시연 전에 꼭 해야 할 것

- `make clean && make`
- `make test`
- `./sql_processor sample_sql/09_full_demo.sql`

이 세 가지가 한 번에 잘 돌아가야 데모가 안정적입니다.
