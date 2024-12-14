# Networking

## Tic-Tac-Toe Game

This project implements a networked Tic-Tac-Toe game using both TCP and UDP protocols. The game allows two players to connect to a server and play Tic-Tac-Toe. The server manages the game state and communicates with the clients to update them on the game progress.

### Files

The files are located in the following directories:

- *TCP Implementation*: networks/partA/tcp/
  1. *client_tcp.c*: TCP client implementation.
  2. *server_tcp.c*: TCP server implementation.

- *UDP Implementation*: networks/partA/udp/
  1. *udp_client.c*: UDP client implementation.
  2. *udp_server.c*: UDP server implementation.
- IP.h 

  1. It has IP address defined for communication within various devices on the same network. change the IP to your network before running.

### How to Run

#### TCP Version

1. *Navigate to the TCP directory*:
    sh
    cd networks/partA/tcp/
    

2. *Compile the server and client*:
    sh
    gcc -o server_tcp server_tcp.c
    gcc -o client_tcp client_tcp.c
    

3. *Run the server*:
    sh
    ./server_tcp
    

4. *Run the clients* (on separate terminals or machines):
    sh
    ./client_tcp
    

#### UDP Version

1. *Navigate to the UDP directory*:
    sh
    cd networks/partA/udp/
    

2. *Compile the server and client*:
    sh
    gcc -o server_udp udp_server.c
    gcc -o client_udp udp_client.c
    

3. *Run the server*:
    sh
    ./server_udp
    

4. *Run the clients* (on separate terminals or machines):
    sh
    ./client_udp
    

### Part A: XOXO

This part of the project involves implementing the core Tic-Tac-Toe game logic and ensuring that the game runs smoothly over a network using both TCP and UDP protocols. The game includes features such as:

- Initializing the game board.
- Handling player moves.
- Checking for win conditions.
- Displaying the game board state.
- Asking players if they want to play again.
- Displaying player statistics.


### Notes

- Ensure that the server is running before starting the clients.
- The IP address in the client files should be set to the server's IP address.
- Both clients and server should be on the same network .

Enjoy playing Tic-Tac-Toe!

## Part B: Chat Application with Data Sequencing and Retransmissions

This part of the project implements a chat application using UDP with data sequencing and retransmissions. The application allows a client and server to send and receive messages in chunks, ensuring reliable communication through acknowledgments and retransmissions.

### Files

The files are located in the following directories:

- *Chat Application*: networks/partB/
  1. *client.c*: UDP client implementation for the chat application.
  2. *server.c*: UDP server implementation for the chat application.

### How to Run

1. *Navigate to the Chat Application directory*:
    sh
    cd networks/partB/
    

2. *Compile the server and client*:
    sh
    gcc -o server server.c
    gcc -o client client.c

3. **Run the server**:
    sh
    ./server
    

4. **Run the client**:
    sh
    ./client
    


### Functionalities

1. **Data Sequencing**: The sender (client or server) divides the data into smaller chunks. Each chunk is assigned a sequence number and the total number of chunks is communicated. The receiver aggregates the chunks according to their sequence number and displays the text.
2. **Retransmissions**: The receiver sends an ACK packet for every data chunk received. If the sender doesn’t receive the acknowledgment for a chunk within a reasonable amount of time, it resends the data. The sender does not wait for an acknowledgment before sending the next chunk.


### Notes

- Ensure that the server is running before starting the client.
- The IP address in the client file should be set to the server's IP address.
- Both client and server should be on the same network .

Enjoy using the chat application!