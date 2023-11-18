#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <array>
#include <system_error>

#include "queue.h"

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

struct OutputStream {
    virtual ~OutputStream() = default;
    virtual void write(const void* data, int len) = 0;
};

struct InputStream {
    virtual ~InputStream() = default;
    virtual void read(void* data, int len) = 0;
};

template<typename Base>
class MappedInputStream: public InputStream {
public:
    MappedInputStream(QueueReader<Base>& reader)
        : reader(reader)
    { }

    void read(void* data, int len) override {
        char* p = (char*)data;
        while (len != 0) {
            auto chunk_size = std::min(len, reader.capacity() / 2);
            reader.pop(p, chunk_size);
            p += chunk_size;
            len -= chunk_size;
        }
    }

private:
    QueueReader<Base> reader;
};

template<typename Base>
class MappedOutputStream: public OutputStream {
public:
    MappedOutputStream(QueueWriter<Base>& writer)
        : writer(writer)
    { }

    void write(const void* data, int len) override {
        const char* p = (const char*)data;
        while (len != 0) {
            auto chunk_size = std::min(len, writer.capacity() / 2);
            writer.push(p, chunk_size);
            p += chunk_size;
            len -= chunk_size;
        }
    }

private:
    QueueWriter<Base> writer;
};

class PipedInputStream: public InputStream {
public:
    PipedInputStream(int fd)
        : fd(fd)
    { }

    void read(void* data, int len) override {
        char* p = (char*)data;
        while (len != 0) {
            auto r = ::read(fd, p, len);
            if (r == -1) {
                if (errno == EAGAIN) {
                    continue;
                } else {
                    throw std::system_error(errno, std::generic_category(), "read");
                }
            }
            if (r == 0) {
                throw std::runtime_error("end of stream");
            }
            p += r;
            len -= r;
        }
    }

private:
    int fd;
};

class PipedOutputStream: public OutputStream {
public:
    PipedOutputStream(int fd)
        : fd(fd)
    { }

    void write(const void* data, int len) override {
        const char* p = (const char*)data;
        while (len != 0) {
            auto r = ::write(fd, p, len);
            if (r == -1) {
                if (errno == EAGAIN) {
                    continue;
                } else {
                    throw std::system_error(errno, std::generic_category(), "write");
                }
            }
            if (r == 0) {
                throw std::runtime_error("end of stream");
            }
            p += r;
            len -= r;
        }
    }

private:
    int fd;
};

template<size_t size>
void client(OutputStream& output) {
    Message<size> m;

    for (int i = 0; i < 10000000; i++) {
        m.init();
        output.write(&m, sizeof(m));
    }
}

template<size_t size>
void server(InputStream& input) {
    Message<size> m;
    uint64_t bytes = 0;
    uint64_t messages = 0;
    auto t0 = now();
    for (int i = 0; i < 10000000; i++) {
        input.read(&m, sizeof(m));
        bytes += size;
        messages ++;
    }
    auto t1 = now();
    double seconds = (t1-t0+1)/1000000000.0;
    printf("%f, %f\n", bytes/1024/1024/seconds, messages/seconds);
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
        kill(child, 9);
    }
}

template<size_t size>
void spawn_pipe() {
    int p[2];
    pipe(p);
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

int main() {
    spawn<1024, QueueBaseLockFree>();
    spawn<2048, QueueBaseLockFree>();
    spawn<8192, QueueBaseLockFree>();
    spawn<32784, QueueBaseLockFree>();

    spawn<1024, QueueBase>();
    spawn<2048, QueueBase>();
    spawn<8192, QueueBase>();
    spawn<32784, QueueBase>();

    spawn_pipe<1024>();
    spawn_pipe<2048>();
    spawn_pipe<8192>();
    spawn_pipe<32784>();

    return 0;
}
