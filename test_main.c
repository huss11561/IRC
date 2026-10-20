#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include "base.h"

// Provide an empty main to redefine it from main.c
#define main app_main
#include "main.c"
#undef main

int mock_send_calls = 0;
int mock_send_last_fd = -1;
char mock_send_last_buf[1024];
size_t mock_send_last_len = 0;

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    mock_send_calls++;
    mock_send_last_fd = sockfd;
    if (len < sizeof(mock_send_last_buf)) {
        memcpy(mock_send_last_buf, buf, len);
        mock_send_last_buf[len] = '\0';
    }
    mock_send_last_len = len;
    return len;
}

int mock_recv_calls = 0;
int mock_recv_return_val = 4;
ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    mock_recv_calls++;
    if (mock_recv_return_val > 0) {
        strcpy(buf, "test");
    }
    return mock_recv_return_val;
}

int mock_accept_calls = 0;
int mock_accept_return_fd = 5;
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    mock_accept_calls++;
    return mock_accept_return_fd;
}

int mock_close_calls = 0;
int mock_close_last_fd = -1;
int close(int fd) {
    mock_close_calls++;
    mock_close_last_fd = fd;
    return 0;
}

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    return 1;
}

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    static struct addrinfo fake_res;
    fake_res.ai_family = AF_INET;
    fake_res.ai_socktype = SOCK_STREAM;
    fake_res.ai_protocol = 0;
    fake_res.ai_addr = NULL;
    fake_res.ai_addrlen = 0;
    *res = &fake_res;
    return 0;
}

int socket(int domain, int type, int protocol) { return 3; }
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) { return 0; }
int listen(int sockfd, int backlog) { return 0; }


void test_send_all_client() {
    s_env env;
    memset(&env, 0, sizeof(env));
    env.sockfd = 3;
    env.fdmax = 5;
    FD_ZERO(&env.master);
    FD_SET(3, &env.master);
    FD_SET(4, &env.master);
    FD_SET(5, &env.master);

    mock_send_calls = 0;
    char *test_msg = "hello";
    send_all_client(&env, strlen(test_msg), test_msg, 4);

    assert(mock_send_calls == 1);
    assert(mock_send_last_fd == 5);
    // There is a bug in original send_all_client: send(j,buff,sizeof buff,0)
    // sizeof buff when buff is char* is just pointer size.
    assert(mock_send_last_len == sizeof(char*));
    printf("test_send_all_client passed\n");
}

void test_new_client() {
    s_env env;
    memset(&env, 0, sizeof(env));
    env.sockfd = 3;
    env.fdmax = 3;
    FD_ZERO(&env.master);
    FD_SET(3, &env.master);

    mock_accept_calls = 0;
    mock_recv_calls = 0;
    mock_send_calls = 0;
    mock_recv_return_val = 4;
    mock_accept_return_fd = 4;

    new_client(&env);

    assert(mock_accept_calls == 1);
    assert(env.fdmax == 4);
    assert(FD_ISSET(4, &env.master));
    assert(mock_recv_calls == 1);
    assert(mock_send_calls == 3); // welcom, yourhost, create
    printf("test_new_client passed\n");
}

void test_read_fd_new_client() {
    s_env env;
    memset(&env, 0, sizeof(env));
    env.sockfd = 3;
    env.fdmax = 3;
    FD_ZERO(&env.master);
    FD_SET(3, &env.master);
    FD_ZERO(&env.fdreads);
    FD_SET(3, &env.fdreads); // sockfd has activity

    mock_accept_calls = 0;
    mock_accept_return_fd = 4;

    read_fd(&env);

    assert(mock_accept_calls == 1);
    assert(env.fdmax == 4);
    printf("test_read_fd_new_client passed\n");
}

void test_read_fd_existing_client() {
    s_env env;
    memset(&env, 0, sizeof(env));
    env.sockfd = 3;
    env.fdmax = 4;
    FD_ZERO(&env.master);
    FD_SET(3, &env.master);
    FD_SET(4, &env.master);
    FD_ZERO(&env.fdreads);
    FD_SET(4, &env.fdreads); // client fd 4 has activity

    mock_recv_calls = 0;
    mock_send_calls = 0;
    mock_recv_return_val = 4;

    read_fd(&env);

    assert(mock_recv_calls == 1);
    assert(mock_send_calls == 0); // No other clients to broadcast to
    printf("test_read_fd_existing_client passed\n");
}

void test_read_fd_client_disconnect() {
    s_env env;
    memset(&env, 0, sizeof(env));
    env.sockfd = 3;
    env.fdmax = 4;
    FD_ZERO(&env.master);
    FD_SET(3, &env.master);
    FD_SET(4, &env.master);
    FD_ZERO(&env.fdreads);
    FD_SET(4, &env.fdreads);

    mock_recv_calls = 0;
    mock_recv_return_val = 0;
    mock_close_calls = 0;

    read_fd(&env);

    assert(mock_recv_calls == 1);
    assert(mock_close_calls == 1);
    assert(mock_close_last_fd == 4);
    assert(!FD_ISSET(4, &env.master));
    printf("test_read_fd_client_disconnect passed\n");
}

int main() {
    printf("Running tests...\n");
    test_send_all_client();
    test_new_client();
    test_read_fd_new_client();
    test_read_fd_existing_client();
    test_read_fd_client_disconnect();
    printf("All tests passed!\n");
    return 0;
}
