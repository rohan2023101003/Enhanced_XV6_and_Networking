# xv6 and Networking Projects

## Some pointers
- main xv6 source code is present inside `initial_xv6/src` directory. This is where you will be making all the additions/modifications necessary for the xv6 part of the Mini Project. 
- work inside the `networks/` directory for the Networking part of the Mini Project.
This README covers the implementation details and usage instructions for two major projects:
1. Enhancements to the xv6 operating system, including new system calls and scheduling policies.
2. A networked Tic-Tac-Toe game and a chat application with data sequencing and retransmissions.

---

## xv6 Operating System Enhancements

### 1. System Calls Implementation

#### **getSysCount**
The `getSysCount` system call counts the number of times a specific system call was invoked by a process. A corresponding user program, `syscount`, uses this call to count and print the occurrences of a specified system call.

#### How to Run
```bash
syscount <mask> command [args]
```

#### Example
```bash
$ syscount 66536 echo hello world
hello world
PID 4 called unknown 4 times
```

#### **sigalarm and sigreturn**
The `sigalarm` system call sets up a periodic alarm that triggers a user-defined handler function after a specified number of CPU ticks. The `sigreturn` system call restores the process state to before the handler was executed.

#### How to Run
```bash
$ alarmtest
```

#### Result
```bash
test0 start
......................................alarm!
test0 passed
test1 start
....alarm!
....alarm!
...alarm!
....alarm!
...alarm!
....alarm!
...alarm!
....alarm!
...alarm!
....alarm!
test1 passed
test2 start
...........................................................!
test2 passed
test3 start
test3 passed
```

### Summary of Changes

- **getSysCount**: Counts the number of times a system call was invoked by a process and its children.
  - Modified files: `kernel/syscall.h`, `kernel/syscall.c`, `kernel/proc.h`, `kernel/proc.c`, `kernel/sysproc.c`, `user/syscount.c`, `user/user.h`, `user/usys.pl`, `Makefile`.

- **sigalarm and sigreturn**: Enables periodic alarms and restores process state after handler execution.
  - Modified files: `kernel/syscall.h`, `kernel/syscall.c`, `kernel/proc.h`, `kernel/proc.c`, `kernel/trap.c`, `user/alarmtest.c`, `user/user.h`, `user/usys.pl`, `Makefile`.

---

### 2. Scheduling Policies

The default xv6 scheduler uses round-robin scheduling. This project introduces four additional scheduling policies, selectable at compile time. The default policy is round-robin (RR), and the number of CPUs used can also be configured.

#### How to Compile and Run

- **Lottery-Based Scheduler (LBS):**
  ```bash
  make qemu SCHEDULER=LBS
  ```

- **Configuring Number of CPUs:**
  ```bash
  make qemu SCHEDULER=LBS CPUS=x
  ```

- **Multi-Level Feedback Queue (MLFQ):**
  - Runs only on one CPU:
    ```bash
    make qemu SCHEDULER=MLFQ CPUS=1
    ```

#### Notes:
- If no scheduler or CPU count is specified, the default is Round Robin (RR) with 2 CPUs.
- Refer to `Report.pdf` for detailed implementation notes.

---

## Networking Projects

### 1. Tic-Tac-Toe Game

This project implements a networked Tic-Tac-Toe game using both TCP and UDP protocols. The server manages the game state, and clients connect to play the game.

#### Files

- **TCP Implementation:**
  - `networks/partA/tcp/client_tcp.c`: TCP client.
  - `networks/partA/tcp/server_tcp.c`: TCP server.

- **UDP Implementation:**
  - `networks/partA/udp/udp_client.c`: UDP client.
  - `networks/partA/udp/udp_server.c`: UDP server.

- **IP.h**: Contains the IP address for communication. Update the IP address to match your network before running the programs.

#### How to Run

##### TCP Version
1. Navigate to the TCP directory:
   ```bash
   cd networks/partA/tcp/
   ```

2. Compile the server and client:
   ```bash
   gcc -o server_tcp server_tcp.c
   gcc -o client_tcp client_tcp.c
   ```

3. Run the server:
   ```bash
   ./server_tcp
   ```

4. Run the clients (on separate terminals or machines):
   ```bash
   ./client_tcp
   ```

##### UDP Version
1. Navigate to the UDP directory:
   ```bash
   cd networks/partA/udp/
   ```

2. Compile the server and client:
   ```bash
   gcc -o server_udp udp_server.c
   gcc -o client_udp udp_client.c
   ```

3. Run the server:
   ```bash
   ./server_udp
   ```

4. Run the clients (on separate terminals or machines):
   ```bash
   ./client_udp
   ```

#### Features
- Game board initialization.
- Player move handling.
- Win condition checks.
- Game board state updates.
- Replay option.
- Player statistics display.

#### Notes
- The server must run before starting the clients.
- Ensure the client IP matches the server's IP.
- Both clients and server should be on the same network.

---

### 2. Chat Application with Data Sequencing and Retransmissions

This project implements a chat application using UDP with features for data sequencing and retransmissions. Messages are sent in chunks with acknowledgments to ensure reliable delivery.

#### Files

- **Chat Application:**
  - `networks/partB/client.c`: UDP client.
  - `networks/partB/server.c`: UDP server.

#### How to Run

1. Navigate to the Chat Application directory:
   ```bash
   cd networks/partB/
   ```

2. Compile the server and client:
   ```bash
   gcc -o server server.c
   gcc -o client client.c
   ```

3. Run the server:
   ```bash
   ./server
   ```

4. Run the client:
   ```bash
   ./client
   ```

#### Functionalities

1. **Data Sequencing:**
   - Messages are divided into chunks, each assigned a sequence number.
   - The receiver reassembles chunks based on sequence numbers.

2. **Retransmissions:**
   - The receiver sends ACK packets for received chunks.
   - The sender retransmits chunks if acknowledgments are not received within a timeout.

#### Notes
- Ensure the server is running before starting the client.
- The client IP address must match the server's IP.
- Both client and server must be on the same network.

---

Enjoy exploring these projects and the functionalities they provide!




