#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "base.h"

// Mock send function
ssize_t mock_send(int sockfd, const void *buf, size_t len, int flags);

#define send mock_send
#define main app_main

// Include the C file to test its internal functions
#include "main.c"

#undef send
#undef main

struct send_call {
    int sockfd;
    char buf[1024];
    size_t len;
    int flags;
};

struct send_call send_calls[100];
int send_call_count = 0;

ssize_t mock_send(int sockfd, const void *buf, size_t len, int flags) {
    if (send_call_count < 100) {
        send_calls[send_call_count].sockfd = sockfd;
        size_t copy_len = len < 1024 ? len : 1023;
        memcpy(send_calls[send_call_count].buf, buf, copy_len);
        send_calls[send_call_count].buf[copy_len] = '\0';
        send_calls[send_call_count].len = len;
        send_calls[send_call_count].flags = flags;
        send_call_count++;
    }
    return len;
}

void reset_mock() {
    send_call_count = 0;
    memset(send_calls, 0, sizeof(send_calls));
}

void test_send_all_client() {
    s_env env;
    memset(&env, 0, sizeof(env));

    FD_ZERO(&env.master);

    env.sockfd = 3; // Server socket
    env.fdmax = 6;

    // Add some clients
    FD_SET(3, &env.master); // server socket
    FD_SET(4, &env.master); // client 1
    FD_SET(5, &env.master); // client 2
    FD_SET(6, &env.master); // client 3

    const char *test_msg = "Hello World";
    int sender_fd = 5;
    int nbytes = strlen(test_msg);

    reset_mock();
    send_all_client(&env, nbytes, test_msg, sender_fd);

    // Check that send was called twice
    assert(send_call_count == 2);

    // Check that it was sent to fd 4
    assert(send_calls[0].sockfd == 4);
    assert(send_calls[0].len == nbytes);

    // Check that it was sent to fd 6
    assert(send_calls[1].sockfd == 6);
    assert(send_calls[1].len == nbytes);

    printf("test_send_all_client passed!\n");
}

int main() {
    test_send_all_client();
    return 0;
}
