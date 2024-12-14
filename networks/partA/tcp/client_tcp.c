// tictactoe_client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "IP.h"
#define PORT 8080
#define BUFFER_SIZE 1024
// #define IP "10.42.0.1"
int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4  address from text to binary form
    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    while (1) {
        // Read message from server
        valread = read(sock, buffer, BUFFER_SIZE);
        if (valread <= 0) {  // Check for disconnection
            printf("Server closed the connection.\n");
            close(sock);
            exit(0);
            break;  // Exit the loop on server disconnect
        }

        buffer[valread] = '\0'; // Null terminate the string

        // Display server message
        printf("%s", buffer);
        // If the server asks for input (like move or replay), send it
        if (strstr(buffer, "Your move") || strstr(buffer, "Do you want to play again") || strstr(buffer, "Do you want to wait")) {
            fgets(buffer, BUFFER_SIZE, stdin);
            send(sock, buffer, strlen(buffer), 0);
        }

        // If the game ends, break the loop
        if (strstr(buffer, "Goodbye")) {
            printf("Ending game.\n");
            break;
        }
    }

    close(sock);
    return 0;
}
