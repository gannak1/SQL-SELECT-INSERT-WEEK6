# B-tree를 우리 MiniSQL 프로젝트로 이해하기

이 문서는 `B-tree`가 왜 필요한지, 그리고 이번 프로젝트에서 정확히 어디에 쓰이는지를 초보자 기준으로 설명한다.

## 1. B-tree가 없을 때는 어떻게 찾나?
현재 CSV 파일은 [users.csv](../data/users.csv)에 저장되어 있다.

만약 B-tree가 없다면:

```sql
SELECT * FROM users WHERE id = 14;
```

를 처리할 때 프로그램은 보통:

1. CSV 파일 처음부터 읽기
2. 한 줄씩 `id` 값 확인하기
3. 원하는 `id`가 나올 때까지 계속 비교하기

를 해야 한다.

이건 데이터가 적을 때는 괜찮지만, 행이 많아질수록 느려진다.

## 2. B-tree가 있으면 뭐가 달라지나?
B-tree는 `정렬된 검색용 자료구조`라고 생각하면 된다.

이번 프로젝트에서는:

- key: `id`
- value: CSV 파일 안에서 그 행이 시작하는 위치(`ftell` 오프셋)

를 저장한다.

그러면 `WHERE id = 14`가 들어왔을 때:

1. B-tree에서 `14`를 먼저 찾고
2. 해당 파일 위치를 얻고
3. CSV 파일에서 그 한 줄만 읽는다

는 식으로 동작한다.

## 3. 이 프로젝트에서 B-tree는 어디에 있나?
관련 코드는 아래에 있다.

- [btree.c](../src/btree.c)
- [btree.h](../include/btree.h)
- [storage.c](../src/storage.c)

역할은 이렇게 나뉜다.

- `btree.c`
  - B-tree 자체 구현
  - 검색과 삽입 담당
- `storage.c`
  - CSV 파일을 스캔해서 B-tree를 구성
  - `id`로 한 줄만 읽는 함수 제공

## 4. 실제로 어떤 함수가 연결되나?
### 프로그램 시작 또는 첫 조회 시
[storage.c](../src/storage.c)의 아래 흐름이 동작한다.

- `ensure_user_index_loaded()`
- `rebuild_user_index()`
- `btree_insert()`

즉:

1. CSV 파일을 한 번 읽고
2. 각 행의 `id`
3. 그 행이 파일 안에서 시작하는 위치

를 B-tree에 넣는다.

### `SELECT * FROM users WHERE id = 14;`
이때는:

- [executor.c](../src/executor.c)의 `execute_select()`
- [storage.c](../src/storage.c)의 `read_user_row_by_id()`
- [btree.c](../src/btree.c)의 `btree_search()`

가 이어진다.

흐름은:

1. `execute_select()`가 조건이 `id = 값`인지 확인
2. `read_user_row_by_id(14, &row)` 호출
3. `btree_search()`로 `14`의 파일 위치 찾기
4. `fseek()`로 그 위치로 이동
5. 그 한 줄만 읽기
6. `print_user_row()`로 출력

## 5. INSERT할 때는 왜 B-tree도 같이 업데이트해야 하나?
새 행을 CSV 끝에 추가한 뒤에는, 자동 생성된 `id`와 파일 위치도 B-tree에 바로 넣어야 한다.

그래야 다음 `SELECT * FROM users WHERE id = 새값;` 에서 전체 파일을 다시 훑지 않고도 찾을 수 있다.

## 6. 왜 디스크에 B-tree를 저장하지 않고 메모리에만 두나?
지금 버전은 학습용 + 최소 구현을 우선한 구조이다.

그래서:

- 실제 데이터는 CSV 파일에 영구 저장
- B-tree는 프로그램 실행 중 메모리에만 유지

하는 절충형 방식을 쓴다.

## 7. 그럼 `users.idx`는 뭐야?
[users.idx](../data/users.idx)는
"이 프로젝트가 B-tree 인덱스를 사용한다"는 것을 보여주는 안내용 파일이다.

즉 현재 버전은:

- 실제 검색 자료구조는 메모리 안 B-tree
- `users.idx`는 설명용 파일

이라고 이해하면 된다.

## 8. 이 프로젝트를 기준으로 B-tree를 한 문장으로 말하면
이 프로젝트의 B-tree는 `users.csv` 안의 각 행을 `id`로 빠르게 찾기 위해, `id -> 파일 위치`를 메모리 안에 정리해 둔 검색용 인덱스 구조이다.
