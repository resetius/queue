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
    auto qf = QueueFile<QueueBaseLockFree>::open("server.q");
    QueueWriter<QueueBaseLockFree> qw(qf);

    while (fgets(buf, sizeof(buf), stdin)) {
        qw.push(buf, strlen(buf));
    }
}

