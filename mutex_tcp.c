#include "mutex_tcp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

// Global variable to store the socket file descriptor
static int sockfd = -1;

void handle_client(int client_fd) {
    // Here you can handle client connections
    // For simplicity, just close the connection
    close(client_fd);
}

void run_server() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd;

    while (1) {
        client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            break;
        }
        handle_client(client_fd);  // Handle each client connection
    }
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
    server_addr.sin_addr.s_addr = inet_addr(MUTEX_TCP_IP); // Bind to localhost
    server_addr.sin_port = htons(port);
    

    // Try to bind the socket to the specified port
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        char error_prefix[50];
        sprintf(error_prefix, "bind %s:%d", MUTEX_TCP_IP, port);    
        perror(error_prefix);
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