#include <string.h>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/mman.h>

#include "queue.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
extern "C" {
#include <cmocka.h>
}

std::string get_file_name(const std::string& prefix, const std::string& suffix) {
    return prefix + "_" + suffix;
}

template<typename Base>
void test_push_pop(void** state) {
    auto fn = get_file_name("test_push_pop", typeid(Base).name());
    char buf[1024];
    auto qf = QueueFile<Base>::create(fn.c_str(), 1024);
    QueueReader<Base> qr(qf);
    QueueWriter<Base> qw(qf);
    qw.push("abc", 4);
    qr.pop(buf, 4);
    assert_string_equal(buf, "abc");
}

template<typename Base>
void test_push_pop_attach(void** state) {
    auto fn = get_file_name("test_push_pop_attach", typeid(Base).name());
    char buf[1024];
    {
        QueueFile<Base>::create(fn.c_str(), 1024);
    }
    auto qf = QueueFile<Base>::open(fn.c_str());
    QueueReader<Base> qr(qf);
    QueueWriter<Base> qw(qf);
    qw.push("abc", 4);
    qr.pop(buf, 4);
    assert_string_equal(buf, "abc");
}

template<typename Base>
void test_push_pop_fork(void** state) {
    auto fn = get_file_name("test_push_pop_fork", typeid(Base).name());
    char buf[1024];
    auto qf = QueueFile<Base>::create(fn.c_str(), 1024);
    pid_t pid = fork();
    if (pid == 0) {
        QueueWriter<Base> qw(qf);
        qw.push("abc", 4);
        exit(0);
    } else {
        QueueReader<Base> qr(qf);
        qr.pop(buf, 4);
        assert_string_equal(buf, "abc");
    }
}

template<typename Base>
void test_push_pop_fork_attach(void** state) {
    auto fn = get_file_name("test_push_pop_fork_attach", typeid(Base).name());
    {
        QueueFile<Base>::create(fn.c_str(), 1024);
    }
    int fd = ::open(fn.c_str(), O_RDWR);
    pid_t pid = fork();
    auto qf = QueueFile<Base>::open(fd);
    if (pid == 0) {
        QueueWriter<Base> qw(qf);
        qw.push("abc", 4);
        exit(0);
    } else {
        char buf[1024];
        QueueReader<Base> qr(qf);
        qr.pop(buf, 4);
        assert_string_equal(buf, "abc");
    }
}

template<typename Base>
void test_push_pop_fork_read_fixed(void** state) {
    auto fn = get_file_name("test_push_pop_fork_read_fixed", typeid(Base).name());
    char buf[1024];
    {
        QueueFile<Base>::create(fn.c_str(), 1024);
    }
    int fd = ::open(fn.c_str(), O_RDWR);
    pid_t pid = fork();
    auto qf = QueueFile<Base>::open(fd);
    if (pid == 0) {
        QueueWriter<Base> qw(qf);
        for (int i = 0; i < 10000; i++) {
            snprintf(buf, sizeof(buf), "%04d", i);
            qw.push(buf, sizeof(buf));
        }
        exit(0);
    } else {
        QueueReader<Base> qr(qf);
        char rbuf[1024];
        for (int i = 0; i < 10000; i++) {
            snprintf(buf, sizeof(buf), "%04d", i);
            qr.pop(rbuf, sizeof(rbuf));
            assert_true(memcmp(buf, rbuf, sizeof(buf)) == 0);
        }
    }
}

uint32_t rand_(uint32_t* seed) {
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;
    return *seed;
}

template<typename Base>
void test_push_pop_fork_read_random(void** state) {
    auto fn = get_file_name("test_push_pop_fork_read_random", typeid(Base).name());
    char buf[1024];
    {
        QueueFile<Base>::create(fn.c_str(), 1024);
    }
    int fd = ::open(fn.c_str(), O_RDWR);
    std::vector<int> wsizes;
    uint32_t seed = 1;
    uint64_t total_size = 0;
    for (int i = 0; i < 10000; i++) {
        auto size = rand_(&seed) % 1000+24;
        wsizes.push_back(size);
        total_size += size;
    }
    pid_t pid = fork();
    auto qf = QueueFile<Base>::open(fd);
    if (pid == 0) {
        uint32_t seed = 1;
        QueueWriter<Base> qw(qf);
        char k = 0;
        for (auto size : wsizes) {
            for (int j = 0; j < size; j++) {
                buf[j] = k++;
            }
            qw.push(buf, size);
        }
        exit(0);
    } else {
        uint32_t seed = 31337;
        QueueReader<Base> qr(qf);
        char rbuf[1024];
        char k = 0;
        while (total_size != 0) {
            auto size = qr.pop_any(rbuf, sizeof(rbuf));
            for (int j = 0; j < size; j++) {
                assert_true(rbuf[j] == k++);
            }
            total_size -= size;
        }
    }
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_push_pop<QueueBaseLockFree>),
        cmocka_unit_test(test_push_pop<QueueBase>),
        cmocka_unit_test(test_push_pop_fork<QueueBaseLockFree>),
        cmocka_unit_test(test_push_pop_fork<QueueBase>),
        cmocka_unit_test(test_push_pop_attach<QueueBaseLockFree>),
        cmocka_unit_test(test_push_pop_attach<QueueBase>),
        cmocka_unit_test(test_push_pop_fork_attach<QueueBaseLockFree>),
        cmocka_unit_test(test_push_pop_fork_attach<QueueBase>),
        cmocka_unit_test(test_push_pop_fork_read_fixed<QueueBaseLockFree>),
        cmocka_unit_test(test_push_pop_fork_read_fixed<QueueBase>),
        //cmocka_unit_test(test_push_pop_fork_read_random<QueueBaseLockFree>),
        cmocka_unit_test(test_push_pop_fork_read_random<QueueBase>),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
