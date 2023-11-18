#include <iostream>
#include <string_view>

#include "queue.h"

int main() {
    char buf[1024];
    auto qf = QueueFile<QueueBaseLockFree>::create("server.q", 1024*1024);
    QueueReader<QueueBaseLockFree> qr(qf);
    while (true) {
        int len = qr.pop_any(buf, sizeof(buf));
        std::cerr << std::string_view(buf, len);
    }

    return 0;
}

