# 하드코딩 테이블 정의

## 목적
1차 구현에서는 과제 범위를 단순화하기 위해 회원가입 시 생성되는 사용자 테이블 `users`가 이미 존재한다고 가정한다.
즉, 범용 테이블 엔진을 만드는 대신 `users` 테이블 하나만 안정적으로 처리하는 것을 목표로 한다.

## 고정 테이블
- 테이블명: `users`

## 컬럼 정의
1차 구현에서는 아래 컬럼을 고정으로 사용한다.

| 컬럼명 | 의미 | 예시 값 |
| --- | --- | --- |
| `id` | 사용자 번호 | `1` |
| `username` | 로그인용 아이디 | `'kim01'` |
| `name` | 사용자 이름 | `'Kim'` |
| `age` | 사용자 나이 | `25` |
| `phone` | 전화번호 | `'010-1234-5678'` |
| `email` | 이메일 주소 | `'kim@example.com'` |

## 기본 규칙
- `id`는 정수형 값으로 사용하며, 사용자가 직접 입력하지 않고 INSERT 시 자동 증가로 생성한다.
- `age`는 정수형 값으로 사용한다.
- `username`, `name`, `phone`, `email`은 문자열로 사용한다.
- 입력 MiniSQL의 값 순서는 항상 `username, name, age, phone, email` 순서라고 가정한다.
- 실제 CSV 저장 순서는 `id, username, name, age, phone, email` 순서라고 가정한다.
- `SELECT`의 `WHERE`는 `users` 테이블의 단일 컬럼 `= 값` 비교를 지원한다.

## 지원 SQL 예시
### INSERT
```sql
INSERT INTO users VALUES ('kim01', 'Kim', 25, '010-1234-5678', 'kim@example.com');
```

### SELECT ALL
```sql
SELECT * FROM users;
```

### SELECT WHERE
```sql
SELECT * FROM users WHERE id = 1;
```

## CLI 실행 예시
```text
$ ./sql_processor
MiniSQL> SELECT * FROM users;
MiniSQL> SELECT * FROM users WHERE id = 1;
MiniSQL> INSERT INTO users VALUES (
...> "hong13", "Hong", 26,
...> "010-1313-1414", "hong@example.com"
...> );
MiniSQL> exit
```

## 설계 의도
이 문서에서 테이블 구조를 고정해두면 다음 장점이 있다.

- 스키마 파일을 따로 설계하지 않아도 된다.
- SQL 파서와 실행기 구현에 집중할 수 있다.
- 발표 시 "왜 하드코딩했는가"를 설명하기 쉽다.

## 이후 확장 방향
1차 구현이 안정화되면 다음 순서로 확장할 수 있다.

1. `users` 외 다른 테이블 추가
2. 코드 하드코딩 제거
3. 별도 스키마 파일 또는 파일 내부 헤더 도입
