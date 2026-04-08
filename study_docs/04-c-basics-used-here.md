# 이 프로젝트에서 쓰이는 핵심 C 문법

## 1. 함수
예:

```c
int main(void) {
    return run_repl();
}
```

뜻:
- `int`: 정수를 반환
- `main`: 함수 이름
- `(void)`: 인자를 받지 않음

## 2. 배열
예:

```c
char line[MAX_INPUT_LEN];
```

뜻:
- 문자 여러 개를 저장하는 배열
- 문자열을 담을 때 자주 사용

## 3. 문자열
C에서 문자열은 사실 `char` 배열이다.

예:

```c
char table[32];
```

이건 최대 31글자 정도의 문자열을 저장하기 위한 공간이다.

## 4. 구조체(struct)
관련 파일:
- [types.h](../include/types.h)

예:

```c
typedef struct {
    char table[32];
    char *values[MAX_VALUE_COUNT];
    int value_count;
} InsertCommand;
```

뜻:
- 서로 관련 있는 데이터를 하나로 묶은 것
- `INSERT` 문장을 표현하기 위한 자료형

## 5. enum
예:

```c
typedef enum {
    CMD_INSERT,
    CMD_SELECT,
    CMD_EXIT,
    CMD_INVALID
} CommandType;
```

뜻:
- 정해진 선택지들 중 하나를 표현
- 명령 종류를 구분할 때 사용

## 6. 포인터
예:

```c
int execute_command(const Command *command);
```

여기서 `*command`는 `Command` 구조체를 가리키는 포인터다.

초보자 관점에서는 이렇게 이해하면 충분하다.
- 큰 구조체를 복사하지 않고 참조만 하려고 포인터를 쓴다.

## 7. `if`, `switch`, `while`
이 프로젝트에서 많이 나오는 제어문이다.

### `if`
조건에 따라 분기

### `switch`
enum 값에 따라 분기

### `while`
반복 처리

REPL의 입력 루프는 `while`로 만든다.

## 8. 파일 입출력
관련 파일:
- [storage.c](../src/storage.c)

주요 함수:
- `fopen`
- `fgets`
- `fputs`
- `fputc`
- `fclose`

이 함수들로 CSV 파일을 읽고 쓴다.

## 9. 문자열 함수
주요 함수:
- `strlen`
- `strcmp`
- `strncpy`
- `memcpy`
- `memmove`

이 프로젝트는 SQL 파싱 때문에 문자열 함수가 많이 나온다.

## 10. 헤더 파일
관련 폴더:
- [include](../include/constants.h)

`.h` 파일은
- 함수 선언
- 구조체 정의
- 상수 정의

를 모아두는 파일이다.

`.c` 파일이 실제 구현이라면, `.h` 파일은 약속서에 가깝다.
