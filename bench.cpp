#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <array>

#include "stream.h"

uint64_t now() {
    uint64_t r;
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    r = ts.tv_sec * 1000000000L;
    r += ts.tv_nsec;
    return r;
}

template<size_t size>
struct Message {
    uint64_t t;
    std::array<char, size> data;

    void init() {
        t = now();
    }
};

template<size_t size>
void client(OutputStream& output) {
    Message<size> m;

    for (int i = 0; i < 10000000; i++) {
        m.init();
        output.write(&m, sizeof(m));
    }
    exit(0);
}

template<size_t size>
void server(InputStream& input) {
    Message<size> m;
    uint64_t bytes = 0;
    uint64_t messages = 0;
    uint64_t avglat = 0;
    auto t0 = now();
    for (int i = 0; i < 10000000; i++) {
        input.read(&m, sizeof(m));
        auto t2 = now();
        bytes += size;
        messages ++;
        avglat += t2-m.t;
    }
    auto t1 = now();
    double seconds = (t1-t0+1)/1000000000.0;
    printf("%0.2f, %0.2f, %0.2f\n", bytes/1024/1024/seconds, messages/seconds, (double)avglat / messages);
}

template<size_t size, typename Base>
void spawn() {
    auto qf = QueueFile<Base>::create("server.q", 1024*1024);

    pid_t child;
    if ((child = fork()) == 0) {
        // child
        QueueWriter<Base> qw(qf);
        MappedOutputStream<Base> output(qw);
        client<size>(output);
    } else {
        // parent
        QueueReader<Base> qr(qf);
        MappedInputStream<Base> input(qr);
        printf("%s %lu: ", typeid(Base).name(), size);
        server<size>(input);
    }
}

template<size_t size>
void spawn_pipe() {
    int p[2];
    if (pipe(p) == -1) {
        throw std::system_error(errno, std::generic_category(), "pipe");
    }
    pid_t child;
    if ((child = fork()) == 0) {
        // child
        PipedOutputStream output(p[1]);
        client<size>(output);
    } else {
        // parent
        PipedInputStream input(p[0]);
        printf("Pipe %lu: ", size);
        server<size>(input);
        kill(child, 9);
    }
}

template<size_t size>
void spawn_vmsplice() {
    int p[2];
    if (pipe(p) == -1) {
        throw std::system_error(errno, std::generic_category(), "pipe");
    }
    pid_t child;
    if ((child = fork()) == 0) {
        // child
        VmsplicedOutputStream output(p[1]);
        client<size>(output);
    } else {
        // parent
        PipedInputStream input(p[0]);
        printf("Vmsplice %lu: ", size);
        server<size>(input);
        kill(child, 9);
    }
}


int main() {
    spawn<1024, QueueBaseLockFree>();
    spawn<2048, QueueBaseLockFree>();
    spawn<4096, QueueBaseLockFree>();
    spawn<8192, QueueBaseLockFree>();
    spawn<16384, QueueBaseLockFree>();
    spawn<32768, QueueBaseLockFree>();

    spawn<1024, QueueBase>();
    spawn<2048, QueueBase>();
    spawn<4096, QueueBase>();
    spawn<8192, QueueBase>();
    spawn<16384, QueueBase>();
    spawn<32768, QueueBase>();

    spawn_pipe<1024>();
    spawn_pipe<2048>();
    spawn_pipe<4096>();
    spawn_pipe<8192>();
    spawn_pipe<16384>();
    spawn_pipe<32768>();

    spawn_vmsplice<1024>();
    spawn_vmsplice<2048>();
    spawn_vmsplice<4096>();
    spawn_vmsplice<8192>();
    spawn_vmsplice<16384>();
    spawn_vmsplice<32768>();

    return 0;
}
