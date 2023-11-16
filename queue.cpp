#include <algorithm>
#include <string.h>
#include <assert.h>
#include <atomic>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

struct QueueHeader {
    std::atomic_int size;
    std::atomic_int free_size;
    char mem[0];
};

class QueueFile {
public:
    QueueFile(const char* file, int capacity, bool attach)
        : fd(open_file(file, capacity, attach))
        , capacity(capacity)
        , data((QueueHeader*)mmap(nullptr, capacity + sizeof(QueueHeader), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0))
    {
        if (!attach) {
            data->size = 0;
            data->free_size = capacity;
        }
    }

    ~QueueFile()
    {
        munmap(data, capacity + sizeof(QueueHeader));
        close(fd);
    }

    QueueHeader* header() {
        return data;
    }

private:
    int open_file(const char* file, int capacity, bool attach) {
        int fd = open(file, O_RDWR | O_CREAT, 0666);
        if (!attach) {
            lseek(fd, capacity + sizeof(QueueHeader), SEEK_SET);
            write(fd, "", 1);
            lseek(fd, 0, SEEK_SET);
        }
        return fd;
    }

    int fd;
    int capacity;
    QueueHeader* data;
};

class Queue {
public:
    Queue(const char* file, int capacity, bool attach)
        : qfile(QueueFile(file, capacity, attach))
        , capacity(capacity)
        , size(qfile.header()->size)
        , free_size(qfile.header()->free_size)
        , mem(qfile.header()->mem)
        , w(0)
        , r(0)
    { }

    ~Queue()
    {
    }

    void push(const char* buf, int len) {
        while (free_size < len) {
            ;
        }

        int first = std::min(len, capacity-w);
        memcpy(mem+w, buf, first);
        memcpy(mem, buf+first, std::max(0, len-first)); 
        w = (w+len)%capacity;
        
        free_size -= len;
        size += len;
    }

    void pop(char* buf, int len) {
        while (size < len) {
            ;
        }

        int first = std::min(len, capacity-r);
        memcpy(buf, mem+r, first);
        memcpy(buf+first, mem, std::max(0, len-first));
        r = (r+len)%capacity;

        free_size += len;
        size -= len;
    }

private:
    QueueFile qfile;
    int capacity;
    std::atomic_int& size;
    std::atomic_int& free_size;
    char* mem;
    int w;
    int r;
};

int main() {
    char buf[1024];
    Queue q("q", 1024, false);
    q.push("abc", 4);
    q.pop(buf, 4);

    printf("%s\n", buf);
    assert(memcmp(buf, "abc", 4) == 0);
}

