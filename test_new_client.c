#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include "base.h"

// Declaration of the function we want to test
void new_client(s_env* env);

int mock_accept_return = -1;
int accept_called = 0;
int recv_called = 0;
int send_called = 0;

int __wrap_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    (void)sockfd; (void)addr; (void)addrlen;
    accept_called++;
    return mock_accept_return;
}

ssize_t __wrap_recv(int sockfd, void *buf, size_t len, int flags) {
    (void)sockfd; (void)buf; (void)len; (void)flags;
    recv_called++;
    return 0; // return 0 bytes read for simplicity
}

ssize_t __wrap_send(int sockfd, const void *buf, size_t len, int flags) {
    (void)sockfd; (void)buf; (void)len; (void)flags;
    send_called++;
    return len; // pretend everything was sent
}

void reset_mocks() {
    mock_accept_return = -1;
    accept_called = 0;
    recv_called = 0;
    send_called = 0;
}

void test_accept_error() {
    printf("Running test_accept_error...\n");
    reset_mocks();
    mock_accept_return = -1; // Trigger error path

    s_env env;
    memset(&env, 0, sizeof(env));
    env.sockfd = 3;
    env.fdmax = 3;
    FD_ZERO(&env.master);

    new_client(&env);

    // accept should be called once
    assert(accept_called == 1);

    // Since accept failed, these should NOT be called
    assert(recv_called == 0);
    assert(send_called == 0);

    // FD set should remain unmodified
    assert(env.fdmax == 3);

    printf("test_accept_error passed!\n");
}

void test_accept_success() {
    printf("Running test_accept_success...\n");
    reset_mocks();
    mock_accept_return = 4; // Simulate successful new connection

    s_env env;
    memset(&env, 0, sizeof(env));
    env.sockfd = 3;
    env.fdmax = 3;
    FD_ZERO(&env.master);

    new_client(&env);

    // accept should be called once
    assert(accept_called == 1);

    // New FD should be added to master set
    assert(FD_ISSET(4, &env.master));
    assert(env.fdmax == 4);

    // Should call recv once and send 3 times (welcome, host, create messages)
    assert(recv_called == 1);
    assert(send_called == 3);

    printf("test_accept_success passed!\n");
}

int main() {
    printf("Starting test suite...\n");
    test_accept_error();
    test_accept_success();
    printf("All tests passed successfully!\n");
    return 0;
}
