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
    auto qf = QueueFile<QueueBaseLockFree>::create("q", 1024);
    QueueReader<QueueBaseLockFree> qr(qf);
    QueueWriter<QueueBaseLockFree> qw(qf);
    qw.push("abc", 4);
    qr.pop(buf, 4);

    printf("%s\n", buf);
    assert(memcmp(buf, "abc", 4) == 0);
}
