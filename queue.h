#pragma once

#include <algorithm>
#include <cstdio>
#include <string.h>
#include <atomic>

#include <fcntl.h>
#include <sys/fcntl.h>
#include <system_error>
#include <unistd.h>
#include <pthread.h>

#include <sys/mman.h>

struct QueueHeader {
    int capacity;
    std::atomic_int size;
    pthread_cond_t cond;
    pthread_mutex_t mut;
    char mem[0];
};

struct QueueBaseLockFree {
    int capacity;
    std::atomic_int size;
    char mem[0];

    void init(int cap) {
        capacity = cap;
        size = 0;
    }

    void wait_read(int len) {
        while (size < len) {
            ;
        }
    }

    void wait_write(int len) {
        while (capacity - size < len) {
            ;
        }
    }

    void inc_size(int len) {
        size += len;
    }
};

struct QueueBase {
    int capacity;
    std::atomic_int size;
    pthread_cond_t cond;
    pthread_mutex_t mut;
    char mem[0];

    void init(int cap) {
        capacity = cap;
        size = 0;
        pthread_mutexattr_t mattr;
        pthread_mutexattr_init(&mattr);
        pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
#ifdef __linux__
        pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
#endif
        pthread_mutex_init(&mut, &mattr);
        pthread_mutexattr_destroy(&mattr);

        pthread_condattr_t cattr;
        pthread_condattr_init(&cattr);
        pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&cond, &cattr);
        pthread_condattr_destroy(&cattr);        
    }

    void wait_read(int len) {
        pthread_mutex_lock(&mut);
        while (size < len) {
            pthread_cond_wait(&cond, &mut);
        }
        pthread_mutex_unlock(&mut);
    }

    void wait_write(int len) {
        pthread_mutex_lock(&mut);
        while (capacity - size < len) {
            pthread_cond_wait(&cond, &mut);
        }
        pthread_mutex_unlock(&mut);
    }

    void inc_size(int len) {
        pthread_mutex_lock(&mut);
        size += len;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mut);
    }
};

template<typename Base>
class QueueFile {
public:
    static QueueFile create(const char* file, int capacity) {
        int fd = ::open(file, O_RDWR | O_CREAT, 0666);
        if (fd == -1) {
            throw std::system_error(errno, std::generic_category(), "open");
        }
        int size = capacity + sizeof(Base);
        if (ftruncate(fd, size) == -1) {
            close(fd);
            throw std::system_error(errno, std::generic_category(), "ftruncate");
        }
        Base* header = static_cast<Base*>(mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));
        if (header == MAP_FAILED) {
            throw std::system_error(errno, std::generic_category(), "mmap");
        }
        header->init(capacity);
        return QueueFile(header, fd);
    }

    static QueueFile open(int fd) {
        int size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
        Base* header = static_cast<Base*>(mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));
        if (header == MAP_FAILED) {
            throw std::system_error(errno, std::generic_category(), "mmap");
        }
        return QueueFile(header, fd);
    }

    static QueueFile open(const char* file) {
        int fd = ::open(file, O_RDWR);
        if (fd == -1) {
            throw std::system_error(errno, std::generic_category(), "open");
        }
        return open(fd);
    }

    ~QueueFile()
    {
        munmap(head, sizeof(Base) + head->capacity);
        close(fd);
    }

    Base* header() {
        return head;
    }

private:
    QueueFile(Base* data, int fd)
        : head(data)
        , fd(fd) 
    { }

    Base* head;
    int fd;
};

template<typename Base>
class QueueStream {
public:
    int capacity() {
        return cap;
    }

protected:
    QueueStream(QueueFile<Base>& qf)
        : header(qf.header())
        , cap(header->capacity)
        , mem(header->mem)
        , pos(0)
    { }

    Base* header;
    int cap;
    char* mem;
    int pos;
};

template<typename Base>
class QueueReader: public QueueStream<Base> {
public:
    QueueReader(QueueFile<Base>& qf): QueueStream<Base>(qf)
    { }

    void pop(char* buf, int len) {
        this->header->wait_read(len);

        int first = std::min(len, this->cap-this->pos);
        memcpy(buf, this->mem+this->pos, first);
        memcpy(buf+first, this->mem, std::max(0, len-first));
        this->pos = (this->pos+len)%this->cap;

        this->header->inc_size(-len);
    }

    int pop_any(char* buf, int len) {
        this->header->wait_read(1);
        len = std::min(this->header->size.load(), len);
        pop(buf, len);
        return len;
    }
};

template<typename Base>
class QueueWriter: public QueueStream<Base> {
public:
    QueueWriter(QueueFile<Base>& qf): QueueStream<Base>(qf)
    { }

    void push(const char* buf, int len) {
        this->header->wait_write(len);

        int first = std::min(len, this->cap-this->pos);
        memcpy(this->mem+this->pos, buf, first);
        memcpy(this->mem, buf+first, std::max(0, len-first)); 
        this->pos = (this->pos+len)%this->cap;

        this->header->inc_size(len);
    }
};
