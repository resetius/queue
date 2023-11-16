#include <algorithm>
#include <string.h>
#include <assert.h>

class Queue {
public:
    Queue(int capacity)
        : capacity(capacity)
        , size(0)
        , free_size(capacity)
        , mem(new char[capacity])
        , w(0)
        , r(0)
    { }

    ~Queue()
    {
        delete[] mem;
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
    int capacity;
    int size;
    int free_size;
    int r;
    int w;
    char* mem;
};

int main() {
    char buf[1024];
    Queue q(1024);
    q.push("abc", 4);
    q.pop(buf, 4);

    printf("%s\n", buf);
    assert(memcmp(buf, "abc", 4) == 0);
}

