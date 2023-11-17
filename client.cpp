#include <algorithm>
#include <string.h>
#include <assert.h>
#include <atomic>

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/mman.h>

#include "queue.h"

int main() {
    char buf[1024];
    QueueFile qf("server.q", 1024*1024, true);
    QueueWriter qw(qf);

    while (1) {
        read(0, buf, sizeof(buf));
        qw.push(buf, sizeof(buf));
    //while (fgets(buf, sizeof(buf), stdin)) {
    //    printf("Push %s\n", buf);
    //    qw.push(buf, strlen(buf));
    //    printf("Push ok\n");
    //}
    }
}

