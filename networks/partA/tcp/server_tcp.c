// tictactoe_server.c

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

// Function to initialize the game board
void initialize_board() {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            board[i][j] = ' ';
}
void broadcast_message(int player1, int player2, const char *message) {
    send(player1, message, strlen(message), 0);
    send(player2, message, strlen(message), 0);
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


    // printf("%s", buffer);
    send(player1, buffer, strlen(buffer), 0);
    send(player2, buffer, strlen(buffer), 0);
}

// Function to handle replay
int ask_replay(int player) {
    char buffer[BUFFER_SIZE];
    send(player, "Do you want to play again? (yes/no): ", 38, 0);
    recv(player, buffer, BUFFER_SIZE, 0);
    return (strncmp(buffer, "yes", 3) == 0);
}

void handle_game(int player1, int player2) {
    int winner;
    char buffer[BUFFER_SIZE];
    
    initialize_board();
    player_turn = 0; // Player 1 starts
    
    while (1) {
        send_board(player1, player2);
        
        // Inform current player to make a move
        sprintf(buffer, "Your move, Player %d: ", player_turn == 0 ? 1 : 2);
        send(player_turn == 0 ? player1 : player2, buffer, strlen(buffer), 0);
        
        // Receive move from current player
        // recv(player_turn == 0 ? player1 : player2, buffer, BUFFER_SIZE, 0);
         int valread = read(player_turn == 0 ? player1 : player2, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            printf("Player %d disconnected\n", player_turn+1);
            break;
        }
        // Parse row and column
        int row, col;
        sscanf(buffer, "%d %d", &row, &col);
        if (row < 0 || row > 2 || col < 0 || col > 2 || board[row][col] != ' ') {
            send(player_turn == 0 ? player1 : player2, "Invalid move, try again.\n", 25, 0);
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
            send(player_turn == 0 ? player1 : player2, "Invalid move, try again.\n", 25, 0);
        }
    }
    
    // Inform clients of the result
    if (winner == 1) {
        printf("Player 1 Wins!\n");
        send(player1, "Player 1 Wins!\n", 15, 0);
        send(player2, "Player 1 Wins!\n", 15, 0);
        send_board(player1, player2);
    } else if (winner == 2) {
        printf("Player 2 Wins!\n");
        send(player1, "Player 2 Wins!\n", 15, 0);
        send(player2, "Player 2 Wins!\n", 15, 0);
        send_board(player1, player2);
    } else {
        printf("It's a Draw!\n");
        send(player1, "It's a Draw!\n", 13, 0);
        send(player2, "It's a Draw!\n", 13, 0);
        send_board(player1, player2);
    }


    // Ask both clients if they want to play again
    int replay1 = ask_replay(player1);
    int replay2 = ask_replay(player2);

    if (replay1 && replay2) {
        printf("Both players want to play again. Starting new game...\n");
        handle_game(player1, player2); // Recursion to start new game
    } else {
        // Notify both players
        if (!replay1 && !replay2) {
            broadcast_message(player1, player2, "Both players have chosen to exit. Closing game...\n");
        } else {
            if (replay1) send(player1, "Your opponent chose not to play again. Closing game...\n", 58, 0);
            if (replay2) send(player2, "Your opponent chose not to play again. Closing game...\n", 58, 0);
        }
        close(player1);
        close(player2); // Close connections for both
    }

}

int main() {
    int server_fd, new_socket1, new_socket2;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Binding the socket to the network address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to join...\n");

    // Accept connections from two players
    if ((new_socket1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Player 1 connected\n");
    }

    if ((new_socket2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Player 2 connected\n");
    }

    printf("Game started!\n");

    // Handle the game logic
    handle_game(new_socket1, new_socket2);
    close(server_fd);
    return 0;
}
