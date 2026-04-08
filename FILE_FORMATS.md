# FILE_FORMATS
## schema / CSV / 샘플 데이터 형식 정의

이 문서는 파일 기반 DB의 구체적인 포맷을 정의합니다.

---

## 1. schema 파일 위치

각 테이블의 schema는 아래 위치에 존재한다고 가정합니다.

```text
schema/<table>.schema
```

예:
- `schema/users.schema`
- `schema/books.schema`

---

## 2. schema 파일 형식

형식은 아래 두 줄을 기본으로 합니다.

```text
table=users
columns=id:INT,name:TEXT,age:INT
```

### 규칙
- `table=` 뒤에는 테이블 이름
- `columns=` 뒤에는 `name:TYPE` 쌍 목록
- 컬럼은 쉼표로 구분
- 지원 타입은 `INT`, `TEXT`

---

## 3. 제공되는 예시 schema

### users
```text
table=users
columns=id:INT,name:TEXT,age:INT
```

### books
```text
table=books
columns=id:INT,title:TEXT,author:TEXT,price:INT
```

---

## 4. data 파일 위치

각 테이블의 실제 데이터는 아래에 저장합니다.

```text
data/<table>.csv
```

예:
- `data/users.csv`
- `data/books.csv`

---

## 5. CSV 형식

이 프로젝트는 **교육용 최소 CSV subset**을 사용합니다.

### 기본 규칙
- 첫 줄은 header row
- 이후 줄은 데이터 row
- 컬럼 순서는 schema와 동일
- 구분자는 쉼표 `,`
- TEXT 값 안에는 쉼표를 허용하지 않음
- 복잡한 CSV escaping은 지원하지 않음

### 예시
```csv
id,name,age
1,Alice,30
2,Bob,24
```

```csv
id,title,author,price
10,CSAPP,Bryant,42000
11,Clean Code,Martin,33000
```

---

## 6. 왜 CSV를 이렇게 단순화하는가

“CSV를 쓴다”는 말은 간단해 보이지만, 실제 CSV는 꽤 복잡합니다.

예를 들어 다음을 고려해야 할 수 있습니다.

- 쉼표가 들어간 셀
- 큰따옴표 escaping
- 줄바꿈이 들어간 셀
- 빈 셀 처리
- 인코딩 문제

하지만 이번 데모의 핵심은 CSV 라이브러리를 만드는 것이 아니라 **SQL 처리 흐름을 이해하는 것**입니다.  
그래서 일부를 의도적으로 단순화합니다.

---

## 7. INSERT 시 파일 처리 규칙

### 파일이 이미 있으면
- header를 유지하고 마지막에 row append

### 파일이 없으면
- 먼저 header row 생성
- 그 다음 첫 row append

---

## 8. SELECT 시 파일 처리 규칙

### 파일이 있으면
- header를 읽고 스킵하거나 검증
- 이후 row들을 읽어서 사용

### 파일이 없으면
- schema는 존재하지만 data file이 없는 빈 테이블로 취급 가능
- 빈 결과를 출력해도 됨

권장 출력:
```text
RESULT: users
id,name,age
ROWS: 0
```

---

## 9. SQL 문자열과 CSV 텍스트의 관계

SQL 입력에서는 TEXT를 작은따옴표로 받습니다.

예:
```sql
INSERT INTO users VALUES (1, 'Alice', 30);
```

하지만 CSV에는 작은따옴표를 제거한 실제 문자열만 저장합니다.

예:
```csv
id,name,age
1,Alice,30
```

즉 처리 흐름은 다음과 같습니다.

```text
'Alice'  --(parser)-->  TEXT literal "Alice"  --(storage write)-->  Alice
```

---

## 10. 출력 형식 파일은 따로 만들지 않아도 됨

프로그램의 SELECT 결과는 stdout에 출력합니다.  
별도의 result file을 남길 필요는 없습니다.

다만 테스트를 위해 `expected_output/` 폴더에 텍스트 예시를 둘 수 있습니다.

---

## 11. Lessons에서 반드시 짚을 포인트

- schema와 data file은 왜 분리되어 있는가?
- schema가 있는데 CREATE TABLE은 왜 없는가?
- CSV를 택하면 무엇이 쉬워지고 무엇이 어려워지는가?
- 나중에 binary format이나 page 구조로 바꾸려면 무엇이 달라지는가?
