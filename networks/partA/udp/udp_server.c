#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define yellow_outline "\033[33m"  // Yellow
#define reset_outline "\033[0m"  
char board[3][3];
int player_turn = 0; // 0 for Player 1 (X), 1 for Player 2 (O)
struct sockaddr_in player1_addr, player2_addr; // To store player addresses
socklen_t addrlen1 = sizeof(player1_addr);
socklen_t addrlen2 = sizeof(player2_addr);
void send_message(int socket, struct sockaddr_in *addr, socklen_t addrlen, const char *message);
void receive_message(int socket, struct sockaddr_in *addr, socklen_t *addrlen, char *buffer);

void initialize_board() {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            board[i][j] = ' ';
}

void broadcast_message(const char *message, int player1, int player2) {
    sendto(player1, message, strlen(message), 0, (struct sockaddr *)&player1_addr, addrlen1);
    sendto(player2, message, strlen(message), 0, (struct sockaddr *)&player2_addr, addrlen2);
}

// Function to check if there is a win or draw
int check_winner() {
    // Check rows, columns, and diagonals
    for (int i = 0; i < 3; i++) {
        if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return board[i][0] == 'X' ? 1 : 2;
        if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i])
            return board[0][i] == 'X' ? 1 : 2;
    }
    if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return board[0][0] == 'X' ? 1 : 2;
    if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return board[0][2] == 'X' ? 1 : 2;

    // Check for draw
    int draw = 1;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == ' ') {
                draw = 0;
                break;
            }
        }
    }
    return draw ? 0 : -1; // 0 for draw, -1 for ongoing game
}

const char *yellow = "\033[1;33m"; // Yellow for grid outline
const char *blue = "\033[1;34m";   // Blue for grid lines
const char *red = "\033[1;31m";    // Red for 'X'
const char *green = "\033[1;32m";  // Green for 'O'
const char *reset = "\033[0m";     // Reset to default color
const char* get_color(char value) {
    if (value == 'X') {
        return red;
    } else if (value == 'O') {
        return green;
    } else {
        return reset;  // For empty spaces or other characters
    }
}

// Function to broadcast board state to both clients
void send_board(int player1, int player2) {
    char buffer[BUFFER_SIZE];

    sprintf(buffer,
        "%s-------------------%s\n"        // Top yellow outline
        "  %s%c%s | %s%c%s | %s%c%s \n"    // First row
        "  %s---+---+---%s  \n"            // Horizontal line
        "  %s%c%s | %s%c%s | %s%c%s \n"    // Second row
        "  %s---+---+---%s \n"             // Horizontal line
        "  %s%c%s | %s%c%s | %s%c%s\n"     // Third row
        "%s-------------------%s\n",       // Bottom yellow outline
        yellow, reset,                     // Applying yellow outline for the top
        get_color(board[0][0]), board[0][0], reset, get_color(board[0][1]), board[0][1], reset, get_color(board[0][2]), board[0][2], reset,
        blue, reset,                       // Horizontal line color
        get_color(board[1][0]), board[1][0], reset, get_color(board[1][1]), board[1][1], reset, get_color(board[1][2]), board[1][2], reset,
        blue, reset,                       // Horizontal line color
        get_color(board[2][0]), board[2][0], reset, get_color(board[2][1]), board[2][1], reset, get_color(board[2][2]), board[2][2], reset,
        yellow, reset                      // Applying yellow outline for the bottom
    );

    sendto(player1, buffer, strlen(buffer), 0, (struct sockaddr *)&player1_addr, addrlen1);
    sendto(player2, buffer, strlen(buffer), 0, (struct sockaddr *)&player2_addr, addrlen2);
}

void handle_game(int player1, int player2) {
    int winner;
    char buffer[BUFFER_SIZE];
    socklen_t addrlen1 = sizeof(player1_addr);
    socklen_t addrlen2 = sizeof(player2_addr);
    initialize_board();
    player_turn = 0; // Player 1 starts

    while (1) {
        send_board(player1, player2);
        
        // Inform current player to make a move
        sprintf(buffer, "Your move, Player %d: ", player_turn == 0 ? 1 : 2);
        
        // Determine the current player's socket and address
        int current_player = player_turn == 0 ? player1 : player2;
        struct sockaddr_in* current_addr = player_turn == 0 ? &player1_addr : &player2_addr;
        socklen_t* current_addr_len = player_turn == 0 ? &addrlen1 : &addrlen2;

        sendto(current_player, buffer, strlen(buffer), 0, (struct sockaddr *)current_addr, *current_addr_len);
        
        // Receive move from current player
        int valread = recvfrom(current_player, buffer, BUFFER_SIZE, 0, (struct sockaddr *)current_addr, current_addr_len);
        if (valread <= 0) {
            printf("Player %d disconnected\n", player_turn + 1);
            break;
        }

        // Parse row and column
        int row, col;
        int scan_count = sscanf(buffer, "%d %d", &row, &col); // Check how many values were scanned

        // Validate input
        if (scan_count != 2 || row < 0 || row > 2 || col < 0 || col > 2 || board[row][col] != ' ') {
            sendto(current_player, "Invalid move, please enter two valid numbers between 0 and 2 for row and column, separated by a space.\n", 100, 0, (struct sockaddr *)current_addr, *current_addr_len);
            continue;
        }
        if (board[row][col] == ' ') {
            board[row][col] = player_turn == 0 ? 'X' : 'O'; // Place X or O
            winner = check_winner();
            
            if (winner != -1) {
                break;
            }
            player_turn = 1 - player_turn; // Switch turns
        } else {
            sendto(current_player, "Invalid move, try again.\n", 25, 0, (struct sockaddr *)current_addr, *current_addr_len);
        }
    }
    
    // Inform clients of the result
    if (winner == 1) {
        printf("Player 1 Wins!\n");
        sendto(player1, "Player 1 Wins!\n", 15, 0, (struct sockaddr *)&player1_addr, addrlen1);
        sendto(player2, "Player 1 Wins!\n", 15, 0, (struct sockaddr *)&player2_addr, addrlen2);
    } else if (winner == 2) {
        printf("Player 2 Wins!\n");
        sendto(player1, "Player 2 Wins!\n", 15, 0, (struct sockaddr *)&player1_addr, addrlen1);
        sendto(player2, "Player 2 Wins!\n", 15, 0, (struct sockaddr *)&player2_addr, addrlen2);
    } else {
        printf("It's a draw!\n");
        sendto(player1, "It's a draw!\n", 13, 0, (struct sockaddr *)&player1_addr, addrlen1);
        sendto(player2, "It's a draw!\n", 13, 0, (struct sockaddr *)&player2_addr, addrlen2);
    }

    // Ask for replay
        int r_player1=0;
        int r_player2=0;
        sendto(player1, "Do you want to play again? (yes/no):", 38, 0, (struct sockaddr *)&player1_addr, addrlen1);
        recvfrom(player1, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&player1_addr, &addrlen1);
        if(strncmp(buffer, "yes", 3) == 0){
            // handle_game(player1, player2);
        }
        else{ 
            r_player1=-1;
            // close(player1);
              
        }
        sendto(player2, "Do you want to play again? (yes/no):", 38, 0, (struct sockaddr *)&player2_addr, addrlen2);
        recvfrom(player2, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&player2_addr, &addrlen2);
        if(strncmp(buffer, "yes", 3) == 0){
            // handle_game(player1, player2);
        }
        else{ 
            r_player2=-1;
            // close(player2);
              
        }
        if(r_player1==-1 && r_player2==-1){
            printf("Both players have chosen to exit. Closing game...\n");
        }
        else if(r_player1==-1){
            sendto(player2, "Your opponent chose not to play again. Closing game...\n", 58, 0, (struct sockaddr *)&player2_addr, addrlen2);
            // close(player2);
        }
        else if(r_player2==-1){
            sendto(player1, "Your opponent chose not to play again. Closing game...\n", 58, 0, (struct sockaddr *)&player1_addr, addrlen1);
            // close(player1);
        }
        else{
            printf("Both players want to play again. Starting new game...\n");
            handle_game(player1, player2);
        }
}

// Main function to setup server and handle player connections
int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&player1_addr, 0, sizeof(player1_addr));
    memset(&player2_addr, 0, sizeof(player2_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");

    // Wait for two players to connect
    recvfrom(server_fd, NULL, 0, 0, (struct sockaddr *)&player1_addr, &addrlen1);
    printf("Player 1 connected.\n");
    recvfrom(server_fd, NULL, 0, 0, (struct sockaddr *)&player2_addr, &addrlen2);
    printf("Player 2 connected.\n");

    handle_game(server_fd, server_fd);
    const char *shutdown_message = "Server is shutting down";
    sendto(server_fd, shutdown_message, strlen(shutdown_message), 0, (struct sockaddr *)&player1_addr, addrlen1);
    sendto(server_fd, shutdown_message, strlen(shutdown_message), 0, (struct sockaddr *)&player2_addr, addrlen2);
    close(server_fd);
    return 0;
}
