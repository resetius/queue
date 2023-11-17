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
    QueueFile qf("server.q", 1024*1024, false);
    QueueReader qr(qf);
    while (true) {
        // printf("On pop\n");
        qr.pop(buf, 1024);
        write(1, buf, 1024);
        // printf("Pop ok\n");
    }

    return 0;
}

