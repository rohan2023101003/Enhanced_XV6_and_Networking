#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>

#define MAX_BUFFER 1024
#define CHUNK_SIZE 10
#define MAX_CHUNKS 1000
#define TIMEOUT 100000 // 0.1 seconds in microseconds
#include "IP.h" // IP.h should define the IP address you're using for communication
#define MAX_RETRANSMISSIONS 5 // Maximum number of retransmissions
#define WINDOW_SIZE 4 // Size of the sliding window

struct Packet {
    int seq_num;
    int total_chunks;
    char data[CHUNK_SIZE];
};

// Function to send ACKs
void send_ack(int sockfd, struct sockaddr_in *server_addr, int seq_num, socklen_t addr_len) {
    int ack[2] = {seq_num, 1}; // ACK structure: {sequence number, acknowledgment}
    sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)server_addr, addr_len);
    printf("ACK sent for chunk %d\n", seq_num);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER];
    socklen_t addr_len = sizeof(server_addr);

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    // Server information
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, IP, &server_addr.sin_addr);

    // Set socket to non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    while (1) {
        printf("Enter a message to send (or 'quit' to exit): ");
        fgets(buffer, MAX_BUFFER, stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // Remove newline

        if (strcmp(buffer, "quit") == 0) {
            break;
        }

        int total_chunks = (strlen(buffer) + CHUNK_SIZE - 1) / CHUNK_SIZE;
        int base = 0; // The base of the current window
        int next_seq_num = 0; // Next sequence number to send
        int ack[2];
        int retransmissions = 0; // Count retransmissions

        // Send packets within the window
        while (base < total_chunks) {
            // Send packets until the window is full
            while (next_seq_num < base + WINDOW_SIZE && next_seq_num < total_chunks) {
                struct Packet send_packet;
                send_packet.seq_num = next_seq_num + 1; // Sequence numbers start at 1
                send_packet.total_chunks = total_chunks;
                strncpy(send_packet.data, buffer + (next_seq_num * CHUNK_SIZE), CHUNK_SIZE);

                sendto(sockfd, &send_packet, sizeof(struct Packet), 0, (struct sockaddr *)&server_addr, addr_len);
                printf("Sent chunk %d of %d\n", send_packet.seq_num, send_packet.total_chunks);
                next_seq_num++;
            }

            // Wait for ACKs (with timeout)
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = TIMEOUT;

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);

            int ready = select(sockfd + 1, &readfds, NULL, NULL, &tv);
            if (ready > 0) {
                // Process ACKs
                while (recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&server_addr, &addr_len) > 0) {
                    int ack_seq_num = ack[0];
                    if (ack_seq_num >= base + 1) {
                        printf("ACK received for chunk %d\n", ack_seq_num);
                        base = ack_seq_num; // Move base forward
                        retransmissions = 0; // Reset retransmissions on successful ACK
                    }
                }
            } else {
                // Timeout, retransmit the whole window starting from the base
                if (retransmissions < MAX_RETRANSMISSIONS) {
                    // printf("Timeout, resending window starting from chunk %d\n", base + 1);
                    next_seq_num = base; // Reset next sequence number to the base
                    retransmissions++;
                } else {
                    // printf("Max retransmissions reached. Exiting...\n");
                    break;
                }
            }
        }

        // Receive response from server
        char full_message[MAX_BUFFER] = {0};
        int chunks_received = 0;
        int total_chunks_expected = 0;

        while (chunks_received < MAX_CHUNKS) {
            struct Packet received_packet;
            int received = recvfrom(sockfd, &received_packet, sizeof(struct Packet), 0, (struct sockaddr *)&server_addr, &addr_len);

            if (received > 0) {
                printf("Received chunk %d of %d\n", received_packet.seq_num, received_packet.total_chunks);

                // Send ACK with sequence number
                send_ack(sockfd, &server_addr, received_packet.seq_num, addr_len);

                // Add received data to the full message
                strncat(full_message, received_packet.data, sizeof(received_packet.data));
                chunks_received++;
                total_chunks_expected = received_packet.total_chunks;

                if (chunks_received == total_chunks_expected) {
                    break;
                }
            }
        }

        if (chunks_received == total_chunks_expected) {
            printf("Full message received from server: %s\n", full_message);
        } else {
            printf("Failed to receive complete message from server\n");
        }
    }

    close(sockfd);
    return 0;
}
