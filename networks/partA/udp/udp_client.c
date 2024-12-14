// tictactoe_client.c

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "IP.h"
#define PORT 8080
#define BUFFER_SIZE 1024
// #define IP "10.1.130.66"

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    socklen_t addr_len = sizeof(serv_addr);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Send a message to the server to indicate that the client is ready
    sendto(sock, "READY", strlen("READY"), 0, (struct sockaddr *)&serv_addr, addr_len);

    while (1) {
        // Clear buffer before reading
        memset(buffer, 0, BUFFER_SIZE);

        // Read message from server
        int valread = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serv_addr, &addr_len);
        if (valread <= 0) {
            perror("recvfrom failed");
            break;  // Exit the loop on error
        }
        if (strcmp(buffer, "Server is shutting down") == 0) {
            printf("Received shutdown message from server. Terminating...\n");
            break;
        }

        buffer[valread] = '\0'; // Null terminate the string

        // Display server message
        printf("%s", buffer);
        
        // If the server asks for input (like move or replay), send it
        if (strstr(buffer, "Your move") || strstr(buffer, "Do you want to play again") || strstr(buffer, "Do you want to wait")) {
            fgets(buffer, BUFFER_SIZE, stdin);
            sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr, addr_len);
        }

    }

    close(sock);
    return 0;
}
