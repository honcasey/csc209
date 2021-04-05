#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#ifndef PORT
  #define PORT 30000
#endif
#define BUF_SIZE 128

int main(void) {
    char buf[BUF_SIZE + 1];
    int x = read(STDIN_FILENO, buf, BUF_SIZE);
    if (x == -1) {
        perror("read");
        exit(1);
    }
    buf[x] = '\0';

    // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) < 1) {
        perror("client: inet_pton");
        close(sock_fd);
        exit(1);
    }

    // Connect to the server.
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(sock_fd);
        exit(1);
    }

    // Get the user to provide a name.
    // char buf[2 * BUF_SIZE + 2]; // 2x to allow for usernames
    // printf("Please enter a username: ");
    // fflush(stdout);
    // int num_read = read(STDIN_FILENO, buf, BUF_SIZE);
    // if (num_read == 0) {
    //     close(sock_fd);
    //     exit(0);
    // }
    // buf[num_read] = '\0';
    // if (write(sock_fd, buf, num_read) != num_read) {
    //     perror("client: write");
    //     close(sock_fd);
    //     exit(1);
    // }
    int w = write(sock_fd, buf, x);
    if (w != x) {
        perror("client: write");
        close(sock_fd);
        exit(1);
    }

    // Read input from the user, send it to the server, and then accept the
    // echo that returns. Exit when stdin is closed.
    int max_fd = sock_fd;
    if (STDIN_FILENO > sock_fd) {
        max_fd = STDIN_FILENO;
    }

    fd_set all;
    FD_ZERO(&all);
    FD_SET(sock_fd, &all);
    FD_SET(STDIN_FILENO, &all);

    while (1) {
        fd_set listen_fds = all;
        int num_ready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (num_ready == -1) {
            perror("server: select");
            exit(1);
        }

        int num_read;
        if (FD_ISSET(STDIN_FILENO, &listen_fds)) {
            int num_read = read(STDIN_FILENO, buf, BUF_SIZE);
            if (num_read == 0) {
                break;
            }
            buf[num_read] = '\0';
            printf("[Server] %s", buf);

            if (write(sock_fd, buf, num_read) != num_read) {
                perror("client: write");
                close(sock_fd);
                exit(1);
            }
        }

        if (FD_ISSET(sock_fd, &listen_fds)) {
            num_read = read(sock_fd, buf, BUF_SIZE);
            buf[num_read] = '\0';
            printf("[Server] %s", buf);
        }
        /*
         * We should really send "\r\n" too, so the server can identify partial
         * reads, but you are not required to handle partial reads in this lab.
         */
    }

    close(sock_fd);
    return 0;
}
