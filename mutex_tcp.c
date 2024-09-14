#include "mutex_tcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

// Global variable to store the socket file descriptor
static int sockfd = -1;

// Function to set the socket to non-blocking mode
void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        exit(EXIT_FAILURE);
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL)");
        exit(EXIT_FAILURE);
    }
}

// Child process function to run the server with non-blocking accept()
void run_server() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd;
    fd_set read_fds;
    struct timeval timeout;

    // Set the listening socket to non-blocking mode
    set_non_blocking(sockfd);

    while (1) {
        // Check if the parent process has died
        if (getppid() == 1) {
            printf("Parent process has terminated. Shutting down server...\n");
            break;  // Exit the server loop if the parent process is gone
        }

        // Initialize the file descriptor set
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        
        // Set timeout (optional), e.g., 5 seconds
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        // Use select to wait for incoming connections (with a timeout)
        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0) {
            perror("select");
            break;
        } else if (activity == 0) {
            // Timeout occurred, no incoming connection
            printf("Waiting for connections...\n");
            continue;
        }

        // If we got here, there's an incoming connection ready to be accepted
        if (FD_ISSET(sockfd, &read_fds)) {
            client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    // Non-blocking mode: no incoming connection yet
                    continue;
                } else {
                    // Other error occurred
                    perror("accept");
                    break;
                }
            }

            printf("Accepted a connection!\n");
            close(client_fd);  // Handle client request and close
        }
    }

    // Cleanup
    close(sockfd);
}

int mutex_start(int port) {
    struct sockaddr_in server_addr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    // Set up the server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Bind to localhost
    server_addr.sin_port = htons(port);

    // Try to bind the socket to the specified port
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        sockfd = -1;
        return -1; // Port is already in use, likely another instance running
    }

    // Listen on the socket (for incoming connections)
    if (listen(sockfd, 1) < 0) {
        perror("listen");
        close(sockfd);
        sockfd = -1;
        return -1;
    }

    // Fork the process to run the server in the background
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        // Child process: Run the server
        run_server();
        exit(0);  // Exit when server is done
    }

    // Parent process: return success
    return 0;
}

void mutex_stop(void) {
    if (sockfd != -1) {
        close(sockfd);  // Close the socket to release the port
        sockfd = -1;    // Reset the file descriptor
    }
}