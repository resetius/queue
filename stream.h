#pragma once
#include <system_error>
#include <vector>
#include <string.h>
#include "queue.h"

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

#ifdef __linux__
// naive version of vmsplice
class VmsplicedOutputStream: public OutputStream {
public:
    VmsplicedOutputStream(int fd)
        : fd(fd)
        , index(0)
        , bufs(64)
    {
        fcntl(fd, F_SETPIPE_SZ, bufs.size() / 2 * 4096);
        iovs.reserve(bufs.size());
    }

    void write(const void* data, int len) override {
        const char* p = (const char*)data;
        while (len != 0) {
            fill(p, len);
            auto r = ::vmsplice(fd, &iovs[0], iovs.size(), 0);
            if (r == -1) {
                if (errno == EAGAIN) {
                    continue;
                } else {
                    throw std::system_error(errno, std::generic_category(), "vmsplice");
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
    char* acquire() {
        auto* r = bufs[index].buf;
        index = (index+1)%bufs.size();
        return r;
    }

    void fill(const char* p, int len) {
        iovs.clear();
        for (int i = 0; i < bufs.size() / 2 && len > 0; i++) {
            int chunk_size = std::min(len, 4096);
            char* buf = acquire();
            memcpy(buf, p, chunk_size);
            iovs.emplace_back(iovec {
                .iov_base = (void*)buf,
                .iov_len = (size_t)chunk_size
            });
            p += chunk_size;
            len -= chunk_size;
        }
    }

    struct Buf {
        char buf[4096];
    };
    int index;
    std::vector<Buf> bufs;
    std::vector<iovec> iovs;
    int fd;
};
#endif

