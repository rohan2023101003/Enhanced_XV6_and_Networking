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
#define MAX_RETRANSMISSIONS 5
#define WINDOW_SIZE 4   // Size of the sliding window

struct Packet {
    int seq_num;
    int total_chunks;
    char data[CHUNK_SIZE];
};

// Function to simulate sending ACKs with skipped acknowledgment for every 3rd packet
void send_ack(int sockfd, struct sockaddr_in *client_addr, int seq_num, int bytes_received) {
    static int ack_count = 0;  // Keeps track of the number of chunks received
    ack_count++;

    // // Skip sending ACK for every 3rd packet (simulating lost ACKs)
    if (ack_count % 3 == 0) {
        printf("Simulating lost ACK for chunk %d\n", seq_num);
        return;  // Skip sending the ACK for this packet
    }

    int ack_data[2];
    ack_data[0] = seq_num;         // ACK = seq_num
    ack_data[1] = bytes_received;   // Number of bytes received

    // Send ACK for the chunk
    // sendto(sockfd, ack_data, sizeof(ack_data), 0, (struct sockaddr *)client_addr, sizeof(*client_addr));
    // printf("ACK sent for chunk %d\n", seq_num);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[MAX_BUFFER];

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // Server information
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    // Bind socket to server address
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening...\n");

    // Set socket to non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    char full_message[MAX_BUFFER] = {0};  // Moved outside the loop for proper concatenation

    while (1) {
        struct Packet received_packet;
        int received = recvfrom(sockfd, &received_packet, sizeof(struct Packet), 0, (struct sockaddr *)&client_addr, &addr_len);

        if (received > 0) {
            printf("Received chunk %d of %d\n", received_packet.seq_num, received_packet.total_chunks);

            // Concatenate the data progressively as chunks are received
            strncat(full_message, received_packet.data, CHUNK_SIZE);

            // Simulate delayed acknowledgment for every other segment
            if (received_packet.seq_num % 2 == 0) {
                usleep(100000);  // Simulate delay
            }
            send_ack(sockfd, &client_addr, received_packet.seq_num, strlen(received_packet.data));

            // If this is the last chunk, process the message
            if (received_packet.seq_num == received_packet.total_chunks) {
                printf("Full message received: %s\n", full_message);

                // Ask for user input to send a response
                printf("Enter a message to send back: ");
                fgets(buffer, MAX_BUFFER, stdin);
                buffer[strcspn(buffer, "\n")] = 0;  // Remove newline

                int total_chunks = (strlen(buffer) + CHUNK_SIZE - 1) / CHUNK_SIZE;
                int base = 0;  // Track the base of the window
                int next_seq_num = 0;  // Track the next sequence number to send
                int retransmissions[MAX_CHUNKS] = {0};  // Track retransmissions for each chunk

                while (base < total_chunks) {
                    // Send packets in the window
                    while (next_seq_num < base + WINDOW_SIZE && next_seq_num < total_chunks) {
                        struct Packet send_packet;
                        send_packet.seq_num = next_seq_num + 1;  // 1-based index
                        send_packet.total_chunks = total_chunks;
                        strncpy(send_packet.data, buffer + (next_seq_num * CHUNK_SIZE), CHUNK_SIZE);

                        // Send the packet
                        sendto(sockfd, &send_packet, sizeof(struct Packet), 0, (struct sockaddr *)&client_addr, addr_len);
                        printf("Sent chunk %d\n", send_packet.seq_num);
                        next_seq_num++;
                    }

                    // Wait for ACKs (with timeout)
                    struct timeval tv;
                    tv.tv_sec = 0;
                    tv.tv_usec = TIMEOUT;

                    fd_set readfds;
                    FD_ZERO(&readfds);
                    FD_SET(sockfd, &readfds);

                    int ack[2];
                    int ready = select(sockfd + 1, &readfds, NULL, NULL, &tv);
                    if (ready > 0) {
                        recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&client_addr, &addr_len);
                        if (ack[0] >= base + 1 && ack[0] <= total_chunks) {
                            printf("ACK received for chunk %d\n", ack[0]);
                            base = ack[0];  // Move the base to the next unacknowledged packet
                        }
                    } else {
                        // Retransmit only the chunks that need it
                        // printf("Timeout, retransmitting chunks...\n");
                        for (int i = base; i < base + WINDOW_SIZE && i < total_chunks; i++) {
                            if (retransmissions[i] < MAX_RETRANSMISSIONS) {
                                struct Packet resend_packet;
                                resend_packet.seq_num = i + 1;  // 1-based index
                                resend_packet.total_chunks = total_chunks;
                                strncpy(resend_packet.data, buffer + (i * CHUNK_SIZE), CHUNK_SIZE);

                                // Resend the packet
                                sendto(sockfd, &resend_packet, sizeof(struct Packet), 0, (struct sockaddr *)&client_addr, addr_len);
                                retransmissions[i]++;
                                // printf("Retransmitted chunk %d (%d times)\n", resend_packet.seq_num, retransmissions[i]);

                                // If we've hit the max retransmissions, stop trying for this chunk
                                if (retransmissions[i] >= MAX_RETRANSMISSIONS) {
                                    // printf("Max retransmissions reached for chunk %d, skipping.\n", resend_packet.seq_num);
                                }
                            }
                        }
                    }
                }
                // Reset full_message buffer for the next transmission cycle
                memset(full_message, 0, sizeof(full_message));
            }
        }
    }

    close(sockfd);
    return 0;
}

