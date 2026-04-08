# 지원/미지원 문법 요약

## 목적
이 문서는 1차 구현에서 지원하는 MiniSQL 문법과 지원하지 않는 문법을 한눈에 확인하기 위한 요약 문서이다.
구현 범위를 빠르게 확인하고, 테스트 케이스와 발표 설명의 기준점으로 사용한다.

## 1차 구현에서 지원하는 것

### 인터랙티브 CLI
- `MiniSQL> ` 프롬프트를 표시한다.
- 사용자는 MiniSQL 문장을 여러 줄에 걸쳐 입력할 수 있고, 세미콜론이 나오면 하나의 문장으로 실행한다.
- 문장 누적 중에는 `...> ` 프롬프트를 표시한다.
- 빈 줄은 조용히 무시한다.
- `exit`, `quit` 입력 시 종료한다.
- EOF(`Ctrl + D`) 입력 시 조용히 종료한다.

### 지원 명령
- `INSERT`
- `SELECT`

### 지원 테이블
- `users` 테이블 하나만 지원한다.

### 지원 SELECT 형태
```sql
SELECT * FROM users;
SELECT * FROM users WHERE id = 1;
SELECT * FROM users WHERE name = 'Kim';
SELECT * FROM users WHERE age = 25;
```

지원 범위:
- `SELECT *`만 지원
- `FROM users`만 지원
- `WHERE`는 선택 사항
- `WHERE`는 1개 조건만 지원
- 1차 구현의 조건 비교는 `=`만 지원
- `WHERE`는 `users` 테이블의 단일 컬럼 `= 값` 조건을 지원

### 지원 INSERT 형태
```sql
INSERT INTO users VALUES ('kim01', 'Kim', 25, '010-1234-5678', 'kim@example.com');
```

지원 범위:
- `INSERT INTO users VALUES (...)`
- 값은 정확히 5개 필요
- 입력 순서는 `username, name, age, phone, email`
- 실제 저장 시 `id`는 자동 증가로 생성된다

### 키워드/기호 규칙
- 키워드는 대소문자를 구분하지 않는다.
- 세미콜론 `;`은 필수이다.
- 공백은 유연하게 허용한다.

### 값 규칙
- 숫자는 따옴표 없이 입력한다.
- 문자열은 작은따옴표 `'` 또는 큰따옴표 `"`로 감쌀 수 있다.
- 문자열 시작과 끝은 같은 종류의 따옴표여야 한다.
- 빈 문자열을 허용한다.
- 공백, 쉼표, 하이픈, 이메일 기호 등은 따옴표 안에 있으면 문자열 값으로 취급한다.

### 저장 규칙
- 데이터는 `data/users.csv`에 저장한다.
- 문자열 컬럼은 CSV 저장 시 항상 큰따옴표로 감싼다.
- 숫자 컬럼은 CSV 저장 시 따옴표 없이 저장한다.
- `WHERE id = 값` 조회는 실행 중 메모리에 유지되는 B-tree 인덱스를 사용한다.
- `WHERE username`, `name`, `age`, `phone`, `email` 조건은 CSV 전체 스캔으로 처리한다.

### 메모리 규칙
- 긴 입력 버퍼는 동적 메모리로 확장한다.
- CSV 전체 조회 시 row 배열은 동적으로 관리한다.
- split된 CSV 컬럼 문자열도 동적으로 할당 후 해제한다.

## 1차 구현에서 지원하지 않는 것

### 명령
- `CREATE TABLE`
- `UPDATE`
- `DELETE`

### SELECT 확장 문법
- `SELECT id, name FROM users;`
- `SELECT username FROM users;`
- `SELECT * FROM orders;`

### WHERE 확장 문법
- 복합 조건 `AND`, `OR`
- 비교 연산자 `>`, `<`, `>=`, `<=`
- `LIKE`
- 괄호 조건식
- 여러 조건 연결

예:
```sql
SELECT * FROM users WHERE age > 20;
SELECT * FROM users WHERE id = 1 AND age = 25;
SELECT * FROM users WHERE name LIKE 'Kim%';
```

### 입력 확장
- 문자열 내부 줄바꿈
- 문자열 내부 같은 종류의 따옴표

예:
```sql
INSERT INTO users VALUES ('O'Brien', 'Kim', 25, '010-1234-5678', 'kim@example.com');
```

### 테이블 확장
- `users` 외 다른 테이블
- 동적 스키마
- 외부 스키마 파일 기반 테이블 정의

## 오류로 처리되는 대표 사례

### 문법 오류
```sql
SELECT * FROM users
INSERT users VALUES (1, 'kim01', 'Kim', 25, '010-1234-5678', 'kim@example.com');
SELECT FROM users;
```

### 지원 범위 오류
```sql
SELECT name FROM users;
SELECT * FROM orders;
SELECT * FROM users WHERE age > 20;
```

### 값/타입 오류
```sql
INSERT INTO users VALUES ('kim01', 'Kim', 'twenty', '010-1234-5678', 'kim@example.com');
INSERT INTO users VALUES ('kim01', 'Kim');
```

## 요약
1차 구현 범위는 다음 한 문장으로 정리할 수 있다.

`users` 테이블 하나를 대상으로 `INSERT`와 `SELECT *` 및 단순 `WHERE`를 처리하는 인터랙티브 MiniSQL REPL을 구현한다.
