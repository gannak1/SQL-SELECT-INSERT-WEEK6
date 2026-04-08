# SQL_GRAMMAR
## 지원 SQL 문법 정의

이 문서는 tokenizer와 parser의 기준을 제공합니다.

---

## 1. 문장 단위

한 SQL 파일 안에는 여러 문장이 들어갈 수 있습니다.

각 문장은 반드시 세미콜론 `;` 으로 끝납니다.

예:
```sql
INSERT INTO users VALUES (1, 'Alice', 30);
SELECT * FROM users;
SELECT id, name FROM users WHERE age >= 20;
```

---

## 2. 키워드

최소한 아래 키워드를 지원해야 합니다.

- `INSERT`
- `INTO`
- `VALUES`
- `SELECT`
- `FROM`
- `WHERE`

권장:
- SQL 키워드는 대소문자 구분 없이 처리  
  예: `select`, `Select`, `SELECT` 모두 허용

식별자(table/column)는 이 데모에서는 소문자 기준으로 사용해도 됩니다.

---

## 3. 식별자(identifier)

식별자는 아래 제약을 따른다고 가정해도 됩니다.

- 시작 문자: 영문자 또는 underscore
- 이후 문자: 영문자, 숫자, underscore

예:
- `users`
- `books`
- `user_name`
- `price`

---

## 4. 리터럴

### 4-1. 정수 리터럴
예:
```sql
1
42
-7
```

### 4-2. 문자열 리터럴
작은따옴표를 사용합니다.

예:
```sql
'Alice'
'Clean Code'
'Boostcamp SQL'
```

### 문자열 리터럴 제한
- 작은따옴표 내부에 작은따옴표 없음
- 쉼표 없음
- 줄바꿈 없음
- escaping 미지원

---

## 5. 연산자

WHERE에서 아래 연산자를 지원합니다.

- `=`
- `!=`
- `<`
- `>`
- `<=`
- `>=`

---

## 6. EBNF 스타일 문법

### 프로그램
```text
program ::= statement*
```

### statement
```text
statement ::= insert_statement | select_statement
```

### insert
```text
insert_statement ::= INSERT INTO identifier VALUES "(" value_list ")" ";"
value_list ::= value ("," value)*
value ::= int_literal | string_literal
```

### select
```text
select_statement ::= SELECT select_list FROM identifier where_clause? ";"
select_list ::= "*" | identifier ("," identifier)*
where_clause ::= WHERE identifier operator value
```

---

## 7. 유효한 예시

### INSERT
```sql
INSERT INTO users VALUES (1, 'Alice', 30);
INSERT INTO books VALUES (10, 'CSAPP', 'Bryant', 42000);
```

### SELECT
```sql
SELECT * FROM users;
SELECT id, name FROM users;
SELECT id, name FROM users WHERE age >= 20;
SELECT * FROM users WHERE name = 'Alice';
```

---

## 8. 무효한 예시

### INSERT에 column list 사용
```sql
INSERT INTO users (id, name, age) VALUES (1, 'Alice', 30);
```
이 데모에서는 지원하지 않음.

### AND 사용
```sql
SELECT * FROM users WHERE age >= 20 AND name = 'Alice';
```
지원하지 않음.

### ORDER BY 사용
```sql
SELECT * FROM users ORDER BY age;
```
지원하지 않음.

### 세미콜론 누락
```sql
SELECT * FROM users
```
문법 오류로 처리.

---

## 9. tokenizer 관점에서 필요한 토큰 종류

필수 토큰 후보:

- keyword
- identifier
- integer literal
- string literal
- comma `,`
- left parenthesis `(`
- right parenthesis `)`
- semicolon `;`
- asterisk `*`
- operator

이 프로젝트는 문법이 작기 때문에 tokenizer와 parser 모두 hand-written으로 충분합니다.

---

## 10. parser가 해야 할 핵심 질문

parser는 결국 아래를 판단해야 합니다.

### INSERT라면
- 테이블 이름은 무엇인가?
- 값은 몇 개인가?
- 각 값의 타입은 무엇인가?

### SELECT라면
- 전체 컬럼 선택인가?
- 특정 컬럼 선택인가?
- 대상 테이블은 무엇인가?
- WHERE가 있는가?
- 있다면 대상 컬럼 / 연산자 / 값은 무엇인가?

---

## 11. WHERE 의미 규칙

예:
```sql
SELECT id, name FROM users WHERE age >= 20;
```

이 문장은 다음 의미를 가집니다.

- `users` 테이블을 스캔한다.
- 각 row에서 `age` 컬럼 값을 꺼낸다.
- `>= 20` 조건을 검사한다.
- 참인 row만 남긴다.
- 그 row에서 `id`, `name`만 출력한다.

즉 WHERE는 parser만의 문제가 아니라, **executor와 storage까지 이어지는 기능**입니다.

---

## 12. Lessons에서 꼭 설명해야 할 문법 포인트

- 왜 토큰화를 먼저 하는가?
- 왜 작은따옴표 문자열을 따로 다뤄야 하는가?
- 왜 WHERE는 “조건식”의 아주 작은 형태인가?
- 왜 지금은 하나의 조건만 두었는가?
- 나중에 AND/OR를 넣으려면 parser 구조가 어떻게 달라져야 하는가?
