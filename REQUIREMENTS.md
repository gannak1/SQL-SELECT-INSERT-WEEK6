# REQUIREMENTS
## 교육용 최소 SQL 처리기 데모 프로젝트 요구사항

이 문서는 **무엇을 구현해야 하는지**를 가장 직접적으로 정의합니다.

---

## 1. 프로젝트 목표

이 프로젝트의 목적은 “진짜 DBMS 만들기”가 아닙니다.  
목적은 다음과 같습니다.

- SQL 처리기의 최소 동작 흐름을 손으로 구현해보기
- C에서 문자열, 포인터, 구조체, 파일 I/O, 에러 처리, 모듈 분리를 경험하기
- tokenizer / parser / executor / storage의 분리를 이해하기
- 구현 결과 자체가 학습자료가 되도록 만들기

---

## 2. 환경

- 운영체제: **Ubuntu**
- 컴파일러: **gcc**
- 빌드 도구: **make**
- 언어: **C**
- 외부 라이브러리: **사용 금지**
- parser generator: **사용 금지**
- 데이터베이스 라이브러리: **사용 금지**

권장 컴파일 옵션 예시:

```bash
gcc -std=c11 -Wall -Wextra -pedantic
```

---

## 3. CLI 요구사항

프로그램은 **SQL 파일 경로를 커맨드라인 인자로 받는 CLI 프로그램**이어야 합니다.

기본 사용 예시는 다음과 같습니다.

```bash
./sql_processor sample_sql/09_full_demo.sql
```

### 기본 디렉터리 규칙
- schema 디렉터리: `./schema`
- data 디렉터리: `./data`

즉 프로그램은 현재 작업 디렉터리 기준으로 `schema/`와 `data/`를 사용해도 됩니다.

---

## 4. 지원해야 하는 SQL 범위

### 4-1. INSERT
반드시 아래 형태를 지원합니다.

```sql
INSERT INTO users VALUES (1, 'Alice', 30);
```

### 4-2. SELECT 전체 컬럼
```sql
SELECT * FROM users;
```

### 4-3. SELECT 특정 컬럼
```sql
SELECT id, name FROM users;
```

### 4-4. SELECT + WHERE 단일 조건
```sql
SELECT id, name FROM users WHERE age >= 20;
SELECT * FROM users WHERE name = 'Alice';
```

---

## 5. 지원하지 않는 것

다음은 구현하지 않습니다.

- `CREATE TABLE`
- `UPDATE`
- `DELETE`
- `JOIN`
- `ORDER BY`
- `GROUP BY`
- `HAVING`
- `AND`
- `OR`
- multiple WHERE conditions
- subquery
- alias
- aggregation

---

## 6. 데이터 타입

최소한 아래 두 타입을 지원해야 합니다.

- `INT`
- `TEXT`

### INT
- 정수 리터럴 사용
- 예: `1`, `42`, `-7`

### TEXT
- SQL에서는 작은따옴표를 사용
- 예: `'Alice'`, `'Clean Code'`

### TEXT의 제한
이 데모 프로젝트에서는 단순화를 위해 TEXT 값에 아래 제한을 둡니다.

- 쉼표 `,` 포함 금지
- 줄바꿈 포함 금지
- 작은따옴표 `'` 포함 금지
- 복잡한 escaping 미지원

이 제한은 교육용 최소 구현을 위한 의도적 단순화입니다.

---

## 7. WHERE 요구사항

### 형태
WHERE는 반드시 **단일 조건**만 지원합니다.

```sql
WHERE column operator value
```

### 지원 연산자
- `=`
- `!=`
- `<`
- `>`
- `<=`
- `>=`

### 타입 규칙
- `INT` 컬럼: 모든 연산자 허용
- `TEXT` 컬럼: `=` 와 `!=` 만 허용
- `TEXT`에 `<`, `>`, `<=`, `>=`를 사용하면 에러 처리

---

## 8. schema 관련 규칙

- schema와 table은 이미 존재한다고 가정합니다.
- schema 파일은 `schema/<table>.schema` 형식으로 존재합니다.
- schema 파일이 없다면 에러입니다.

### schema 파일 예시
```text
table=users
columns=id:INT,name:TEXT,age:INT
```

---

## 9. CSV 저장 규칙

각 테이블은 `data/<table>.csv`에 저장합니다.

### CSV 파일 기본 규칙
- 첫 줄은 header row
- 이후 줄은 실제 row 데이터
- 컬럼 순서는 schema의 순서를 따름

예:
```csv
id,name,age
1,Alice,30
2,Bob,24
```

### 데이터 파일이 없을 때
- `INSERT` 시: 파일을 생성하고 header row를 쓴 뒤 데이터를 append
- `SELECT` 시: schema만 존재하면 빈 테이블로 취급해도 됨  
  출력은 header와 `ROWS: 0`으로 처리 가능

---

## 10. 출력 형식

출력 형식은 단순하고 테스트하기 쉬워야 합니다.

### INSERT 성공
```text
OK: 1 row inserted into users
```

### SELECT 성공 예시
```text
RESULT: users
id,name
1,Alice
2,Bob
ROWS: 2
```

### 비어 있는 결과
```text
RESULT: users
id,name
ROWS: 0
```

출력 형식은 이 문서와 최대한 맞추세요.  
테스트와 샘플 출력도 같은 형식을 가정합니다.

---

## 11. 에러 처리 형식

에러는 가능한 한 명확하게 출력해야 합니다.

권장 형식:

- 문법 오류  
  `ERROR: syntax error near "<token>"`
- 테이블 없음  
  `ERROR: table not found: users`
- 컬럼 없음  
  `ERROR: column not found: nickname`
- 타입 불일치  
  `ERROR: type mismatch for column age`
- WHERE 타입 불일치  
  `ERROR: type mismatch in WHERE for column age`
- TEXT 비교 미지원  
  `ERROR: unsupported TEXT comparison operator: >`
- 값 개수 불일치  
  `ERROR: value count does not match schema for table users`

### 오류 처리 정책
- 첫 번째 오류에서 중단해도 됩니다.
- 종료 코드는 non-zero여야 합니다.

---

## 12. SQL 파일 처리 규칙

- 한 SQL 파일 안에 여러 문장을 둘 수 있습니다.
- 문장 구분자는 세미콜론 `;`
- 공백과 줄바꿈은 자유롭게 허용
- blank line 허용
- SQL 주석은 필수 지원 아님

---

## 13. 구현 구조 요구사항

최소한 다음 개념 경계는 코드에서 드러나야 합니다.

- file reading
- tokenization
- parsing
- schema loading
- execution
- csv reading/writing
- output printing
- error handling

즉, `main.c`에 모든 로직을 몰아넣으면 안 됩니다.

---

## 14. 테스트 요구사항

최소한 다음은 포함해야 합니다.

### 단위 테스트
- tokenizer 기본 동작
- parser 기본 동작
- WHERE 조건 평가
- CSV row append/read

### 통합 테스트
- sample SQL 실행
- 기대 출력 비교
- 에러 케이스 검증

---

## 15. 주석과 문서화 요구사항

이 프로젝트는 학습자료로도 사용될 예정입니다.  
따라서 모든 코드에는 **매우 풍부한 주석**이 필요합니다.

필수:
- 파일 상단 목적 설명
- 함수 상단 설명
- 복잡한 블록 주석
- 메모리 / 포인터 / 문자열 처리 설명

또한 구현 후 `Lessons/` 폴더에 실제 코드 기반 문서를 작성해야 합니다.

---

## 16. 수용 기준(acceptance criteria)

아래를 만족하면 최소 요구사항을 충족한 것입니다.

1. `make`로 빌드된다.
2. `./sql_processor sample_sql/09_full_demo.sql`로 데모가 실행된다.
3. `INSERT`, `SELECT`, `WHERE`가 동작한다.
4. CSV 파일이 실제로 생성/추가/조회된다.
5. 오류 메시지가 명확하다.
6. 테스트가 있다.
7. Lessons가 실제 구현 기준으로 작성되어 있다.
