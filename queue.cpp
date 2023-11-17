#include <algorithm>
#include <string.h>
#include <assert.h>
#include <atomic>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

struct QueueHeader {
    std::atomic_int size;
    char mem[0];
};

class QueueFile {
public:
    QueueFile(const char* file, int cap, bool attach)
        : fd(open_file(file, cap, attach))
        , cap(cap)
        , data((QueueHeader*)mmap(nullptr, cap + sizeof(QueueHeader), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0))
    {
        if (!attach) {
            data->size = 0;
        }
    }

    ~QueueFile()
    {
        munmap(data, cap + sizeof(QueueHeader));
        close(fd);
    }

    QueueHeader* header() {
        return data;
    }

    int capacity() {
        return cap;
    }

private:
    int open_file(const char* file, int cap, bool attach) {
        int fd = open(file, O_RDWR | O_CREAT, 0666);
        if (!attach) {
            lseek(fd, cap + sizeof(QueueHeader), SEEK_SET);
            write(fd, "", 1);
            lseek(fd, 0, SEEK_SET);
        }
        return fd;
    }

    int fd;
    int cap;
    QueueHeader* data;
};

class QueueStream {
protected:
    QueueStream(QueueFile& qf)
        : capacity(qf.capacity())
        , size(qf.header()->size)
        , mem(qf.header()->mem)
        , pos(0)
    { }

    int capacity;
    std::atomic_int& size;
    char* mem;
    int pos;
};

class QueueReader: public QueueStream {
public:
    QueueReader(QueueFile& qf): QueueStream(qf)
    { }

    void pop(char* buf, int len) {
        while (size < len) {
            ;
        }

        int first = std::min(len, capacity-pos);
        memcpy(buf, mem+pos, first);
        memcpy(buf+first, mem, std::max(0, len-first));
        pos = (pos+len)%capacity;

        size -= len;
    }
};

class QueueWriter: public QueueStream {
public:
    QueueWriter(QueueFile& qf): QueueStream(qf)
    { }

    void push(const char* buf, int len) {
        while (capacity - size < len) {
            ;
        }

        int first = std::min(len, capacity-pos);
        memcpy(mem+pos, buf, first);
        memcpy(mem, buf+first, std::max(0, len-first)); 
        pos = (pos+len)%capacity;
        
        size += len;
    }
};

int main() {
    char buf[1024];
    QueueFile qf("q", 1024, false);
    QueueReader qr(qf);
    QueueWriter qw(qf);
    qw.push("abc", 4);
    qr.pop(buf, 4);

    printf("%s\n", buf);
    assert(memcmp(buf, "abc", 4) == 0);
}

