# MiniSQL 문법 기초

## 왜 MiniSQL이라고 부르는가
이번 과제에서 입력받는 문장은 표준 SQL 전체가 아니다.
`INSERT`, `SELECT`, 단순 `WHERE`만 포함하는 제한된 문법만 지원하므로 문서에서는 이를 `MiniSQL`이라고 부른다.

즉, 이 프로젝트는 범용 DBMS가 아니라 과제용 MiniSQL 처리기이다.

예를 들어 다음과 같은 문장이 이번 과제의 MiniSQL 문장이다.

```sql
INSERT INTO users VALUES ('kim01', 'Kim', 25, '010-1234-5678', 'kim@example.com');
SELECT * FROM users WHERE id = 1;
```

## MiniSQL 문장을 이루는 요소
MiniSQL 문장은 보통 다음 요소들로 이루어진다.

### 키워드
명령의 의미를 알려주는 단어이다.

예:
- `INSERT`
- `INTO`
- `VALUES`
- `SELECT`
- `FROM`
- `WHERE`

### 식별자
테이블 이름이나 컬럼 이름이다.

예:
- `users`
- `id`
- `name`

### 값
실제로 저장하거나 비교할 데이터이다.

예:
- `1`
- `'kim01'`
- `'Kim'`
- `25`
- `'010-1234-5678'`
- `'kim@example.com'`

### 연산자와 구분자
문법 구조를 나누는 기호이다.

예:
- `(`
- `)`
- `,`
- `=`
- `;`

## 이번 과제에서 지원하는 MiniSQL 형태
### INSERT
```sql
INSERT INTO users VALUES ('kim01', 'Kim', 25, '010-1234-5678', 'kim@example.com');
```

구조:
- 명령 종류: `INSERT`
- 테이블명: `users`
- 값 목록: `'kim01'`, `'Kim'`, `25`, `'010-1234-5678'`, `'kim@example.com'`

### SELECT
```sql
SELECT * FROM users;
```

구조:
- 명령 종류: `SELECT`
- 조회 대상: 전체 컬럼 `*`
- 테이블명: `users`

### SELECT with WHERE
```sql
SELECT * FROM users WHERE id = 1;
```

구조:
- 명령 종류: `SELECT`
- 테이블명: `users`
- 조건 컬럼: `id`
- 조건 값: `1`

## 파싱이란 무엇인가
파싱은 MiniSQL 문자열을 프로그램이 다루기 쉬운 구조로 바꾸는 과정이다.

예를 들어 아래 문장을

```sql
SELECT * FROM users WHERE id = 1;
```

이런 식의 내부 정보로 바꾸는 것이다.

- command type: `SELECT`
- table: `users`
- where column: `id`
- where value: `1`

즉, 문자열을 그대로 처리하는 것이 아니라 의미 단위로 나눠서 저장하는 과정이 파싱이다.

## 이 문서에서 중요한 구분
- `SQL`: 일반적인 데이터 질의 언어 전체를 가리키는 큰 개념
- `MiniSQL`: 이번 과제에서 실제로 지원하는 제한된 입력 문법

이 프로젝트 문서에서는 혼동을 줄이기 위해 구현 대상 입력은 `MiniSQL`이라고 부른다.

세부 입력 규칙은 [MiniSQL 입력 규칙](08-minisql-input-rules.md) 문서를 기준으로 한다.
