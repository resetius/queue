#include <algorithm>
#include <string.h>
#include <assert.h>
#include <atomic>

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

template<typename Base>
void test_push_pop(void** state) {
    char buf[1024];
    auto qf = QueueFile<Base>::create("test_push_pop", 1024);
    QueueReader<Base> qr(qf);
    QueueWriter<Base> qw(qf);
    qw.push("abc", 4);
    qr.pop(buf, 4);
    assert_string_equal(buf, "abc");
}

template<typename Base>
void test_push_pop_attach(void** state) {
    char buf[1024];
    {
        QueueFile<Base>::create("test_push_pop_attach", 1024);
    }
    auto qf = QueueFile<Base>::open("test_push_pop_attach");
    QueueReader<Base> qr(qf);
    QueueWriter<Base> qw(qf);
    qw.push("abc", 4);
    qr.pop(buf, 4);
    assert_string_equal(buf, "abc");
}

template<typename Base>
void test_push_pop_fork(void** state) {
    char buf[1024];
    auto qf = QueueFile<Base>::create("test_push_pop_fork", 1024);
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
    char buf[1024];
    {
        QueueFile<Base>::create("test_push_pop_fork_attach", 1024);
    }
    int fd = ::open("test_push_pop_fork_attach", O_RDWR);
    pid_t pid = fork();
    auto qf = QueueFile<Base>::open(fd);
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
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
