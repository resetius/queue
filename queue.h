#pragma once

#include <algorithm>
#include <string.h>
#include <assert.h>
#include <atomic>

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/mman.h>

struct QueueHeader {
    std::atomic_int size;
    pthread_cond_t cond;
    pthread_mutex_t mut;
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
            pthread_mutexattr_t mattr;
            pthread_mutexattr_init(&mattr);
            pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
            // pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
            pthread_mutex_init(&data->mut, &mattr);
            pthread_mutexattr_destroy(&mattr);

            pthread_condattr_t cattr;
            pthread_condattr_init(&cattr);
            pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
            pthread_cond_init(&data->cond, &cattr);
            pthread_condattr_destroy(&cattr);;
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
        , header(qf.header())
        , pos(0)
    { }

    int capacity;
    std::atomic_int& size;
    char* mem;
    QueueHeader* header;
    int pos;
};

class QueueReader: public QueueStream {
public:
    QueueReader(QueueFile& qf): QueueStream(qf)
    { }

    void pop(char* buf, int len) {
        {
            pthread_mutex_lock(&header->mut);
            while (size < len) {
                pthread_cond_wait(&header->cond, &header->mut);
            }
            pthread_mutex_unlock(&header->mut);
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
        {
            pthread_mutex_lock(&header->mut);
            while (capacity - size < len) {
                pthread_cond_wait(&header->cond, &header->mut);
            }
            pthread_mutex_unlock(&header->mut);
        }

        int first = std::min(len, capacity-pos);
        memcpy(mem+pos, buf, first);
        memcpy(mem, buf+first, std::max(0, len-first)); 
        pos = (pos+len)%capacity;
        
        size += len;
    }
};

