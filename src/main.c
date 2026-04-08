#include "storage.h"
#include "repl.h"

/* main()은 프로그램의 시작점이다.
   실제 MiniSQL 입력/실행은 run_repl()에 맡기고,
   종료 직전에는 storage 쪽 전역 메모리를 정리한다. */
int main(void) {
    /* status는 run_repl()이 끝났을 때 돌려준 종료 코드를 잠깐 보관한다.
       REPL이 정상 종료했는지, 오류로 끝났는지 main()이 그대로 돌려주기 위해 필요하다. */
    int status;

    /* 프로그램 시작 후 실제 작업은 REPL 루프가 담당한다. */
    status = run_repl();
    /* REPL이 끝난 뒤에는 storage가 잡고 있던 전역 B-tree 인덱스를 정리한다. */
    storage_shutdown();
    /* main의 반환값은 프로그램 종료 상태가 되므로,
       run_repl() 결과를 status에 담아 두었다가 그대로 반환한다. */
    return status;
}
