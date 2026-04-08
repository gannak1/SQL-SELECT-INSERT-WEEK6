# 처리 흐름 설계

## 전체 흐름
SQL 처리기는 다음 순서로 동작한다.

1. 프로그램 시작 후 `MiniSQL> ` 프롬프트를 출력한다.
2. 사용자 입력을 한 줄씩 받는다.
3. 세미콜론이 나올 때까지 입력을 누적한다.
4. 입력이 `exit` 또는 `quit`인지 확인한다.
5. MiniSQL 문장을 파싱한다.
6. 파싱 결과에 따라 실행한다.
7. 결과를 출력한 뒤 다음 입력을 기다린다.

입력 해석 규칙은 [MiniSQL 입력 규칙](08-minisql-input-rules.md) 문서를 따른다.

## 단계별 설명
### 1. 입력
사용자는 아래처럼 프로그램을 실행한다.

```text
$ ./sql_processor
MiniSQL> INSERT INTO users VALUES (
...> "hong13", "Hong", 26,
...> "010-1313-1414", "hong@example.com"
...> );
MiniSQL> SELECT * FROM users WHERE id = 1;
MiniSQL> exit
```

프로그램은 실행 후 프롬프트를 보여주고, 사용자가 입력한 MiniSQL 문장을 세미콜론이 나올 때까지 누적해서 읽는다.

### 2. 파싱
문자열 상태의 MiniSQL 문장을 분석해서 내부 구조체로 변환한다.

예:
- 명령 종류
- 테이블명
- 값 목록
- WHERE 조건

### 3. 실행
파싱된 결과를 보고 실제 동작을 결정한다.

- `INSERT`이면 데이터 추가
- `SELECT`이면 데이터 읽기 및 조건 검사

### 4. 저장 또는 출력
- INSERT 결과는 파일에 반영된다.
- SELECT 결과는 콘솔에 출력된다.

## 모듈 분리 방향
### main
- 프로그램 시작점
- `repl` 실행 함수 호출

### repl
- 프롬프트 출력
- 반복 입력 루프 처리
- 여러 줄 입력 누적
- 종료 명령 처리
- parser와 executor 연결

### parser
- MiniSQL 문장을 분석
- 명령 구조체 생성

### executor
- `INSERT`, `SELECT` 분기 처리

### storage
- 파일 읽기/쓰기
- 테이블 파일 열기

### printer
- SELECT 결과 출력

## 최소 구현 흐름 예시
### INSERT
1. `INSERT INTO users VALUES ('kim01', 'Kim', 25, '010-1234-5678', 'kim@example.com');` 읽기
2. `users` 테이블, 값 목록 `[kim01, Kim, 25, 010-1234-5678, kim@example.com]` 추출
3. 저장소에 새 `id`를 붙여 한 행 추가
4. 프롬프트로 돌아가 다음 입력을 기다림

### SELECT
1. `SELECT * FROM users WHERE id = 1;` 읽기
2. 테이블명과 조건 추출
3. 테이블 데이터를 순회
4. 조건에 맞는 행만 출력
5. 프롬프트로 돌아가 다음 입력을 기다림
