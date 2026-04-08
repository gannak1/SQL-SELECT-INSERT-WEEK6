/*
 * 파일 목적:
 *   파일 전체 내용을 한 번에 읽어 메모리에 올리는 함수를 구현한다.
 *
 * 전체 흐름에서 위치:
 *   SQL 파일 입력과 schema 파일 로딩의 첫 단계에서 사용된다.
 *
 * 주요 내용:
 *   - fopen / fseek / ftell / fread 기반 전체 읽기
 *
 * 초보자 포인트:
 *   파일을 조금씩 읽는 방식도 가능하지만,
 *   이번 프로젝트처럼 파일이 작다고 가정할 때는 전체 읽기가 더 단순하다.
 *
 * 메모리 / 문자열 주의점:
 *   fread 후 마지막에 직접 '\0'을 붙여 문자열처럼 다룰 수 있게 만든다.
 */

#include "file_reader.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/*
 * 목적:
 *   지정한 파일의 내용을 통째로 읽어 널 종료 문자열로 반환한다.
 *
 * 입력:
 *   path - 읽을 파일 경로.
 *   out_contents - 성공 시 결과 문자열을 받을 포인터.
 *   error - 실패 이유를 기록할 구조체.
 *
 * 반환:
 *   성공 시 1, 실패 시 0.
 *
 * 동작 요약:
 *   파일 크기를 구한 뒤 그 크기만큼 메모리를 할당해 fread로 읽는다.
 *
 * 초보자 주의:
 *   fread는 읽은 바이트 수를 반환하므로,
 *   파일 크기와 다르면 부분 읽기 실패를 의심해야 한다.
 */
int read_entire_file(const char *path, char **out_contents, SqlError *error) {
    FILE *file;
    long file_size;
    size_t read_count;
    char *buffer;

    if (out_contents == NULL) {
        sql_error_set(error, "ERROR: internal file reader error");
        return 0;
    }

    *out_contents = NULL;

    file = fopen(path, "rb");
    if (file == NULL) {
        sql_error_set(error, "ERROR: cannot open file: %s", path);
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        sql_error_set(error, "ERROR: failed to seek file: %s", path);
        return 0;
    }

    file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        sql_error_set(error, "ERROR: failed to tell file size: %s", path);
        return 0;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        sql_error_set(error, "ERROR: failed to rewind file: %s", path);
        return 0;
    }

    buffer = (char *)malloc((size_t)file_size + 1);
    if (buffer == NULL) {
        fclose(file);
        sql_error_set(error, "ERROR: out of memory while reading file: %s", path);
        return 0;
    }

    read_count = fread(buffer, 1, (size_t)file_size, file);
    if (read_count != (size_t)file_size) {
        free(buffer);
        fclose(file);
        sql_error_set(error, "ERROR: failed to read file: %s", path);
        return 0;
    }

    buffer[file_size] = '\0';

    if (fclose(file) != 0) {
        free(buffer);
        sql_error_set(error, "ERROR: failed to close file: %s", path);
        return 0;
    }

    *out_contents = buffer;
    return 1;
}
