
MPCC (Multi‑Party Chat Conference)
MPCC is a lightweight, multi‑client chat system implemented in C++ using TCP sockets and a thread‑per‑client server architecture. It enables real‑time message exchange between multiple connected users over a simple, text‑based protocol.
This project demonstrates practical socket programming, multithreading, and robust connection lifecycle handling, including graceful shutdowns and client reconnections.


 Features
Server

TCP listener supporting multiple simultaneous client connections
Dedicated ClientHandler thread per client
User registration and login support
Broadcast messaging to all connected users
Graceful handling of client EXIT commands
Clean shutdown on Ctrl+C (SIGINT)
Stable across multiple restart cycles (verified via logs)
Client

Connects to server using configurable host and port
User registration / login via text‑based commands
Asynchronous receive loop for incoming messages
Clean disconnection using EXIT
Reliable reconnection after server restarts


 Architecture Overview
+-------------------+            +-------------------+
|    MPCC Client    | <——TCP——>  |    MPCC Server    |
|-------------------|            |-------------------|
| • Connect/Login   |            | • Accept clients  |
| • Send messages   |            | • Spawn threads   |
| • Receive loop    |            | • Route messages  |
| • EXIT handling   |            | • Handle EXIT     |
+-------------------+            +-------------------+



The server listens on a specified port and accepts incoming TCP connections.
Each client connection is handled by a dedicated ClientHandler thread.
Messages from one client are broadcast to all other connected clients.
A simple text‑based protocol defines all interactions.


 Project Structure
/mpcc
│── server/
│   ├── mpcc_server.cpp      # Server entry point
│   ├── client_handler.cpp   # Per‑client thread logic
│   ├── client_handler.h
│   └── ...
│
│── client/
│   ├── mpcc_client.cpp      # Client implementation
│   └── ...
│
│── logs/
│   ├── mpcc_server.log      # Server runtime logs
│   └── mpcc_client.log      # Client runtime logs
│
│── README.md
└── Makefile / build scripts




 Communication Protocol
Client → Server Commands

Command	Description
REGISTER <name>	Register as a new user
LOGIN <name>	Login as an existing user
MSG <text>	Broadcast a message to all users
EXIT	Disconnect gracefully

Server → Client Messages

System notifications (login success, welcome messages)
Broadcast messages from other users
Error messages for invalid commands or formats


 Build & Run
Prerequisites

GCC / G++ with C++11 or later
POSIX‑compatible OS (Linux / macOS)
Compile
# Build server
g++ server/mpcc_server.cpp server/client_handler.cpp -o mpcc_server -pthread

# Build client
g++ client/mpcc_client.cpp -o mpcc_client


Run Server
./mpcc_server <port>
# Example
./mpcc_server 8080


Run Client
./mpcc_client <server_ip> <port>
# Example
./mpcc_client 127.0.0.1 8080




 Verified Behavior
Based on runtime logs and testing:

Multiple clients connect successfully from 127.0.0.1
Each client is managed by a separate server thread
Registration and login flows work as expected
Messages are broadcast reliably in real time
EXIT command triggers clean client disconnection
Server shuts down gracefully and restarts without issues
Clients reconnect successfully after server restarts


🔧 Future Enhancements
Potential extensions to the MPCC system include:

Chat rooms / channels
Private (one‑to‑one) messaging
Password‑based authentication
TLS‑encrypted communication
Persistent chat history (database‑backed)
File transfer support
GUI or Web‑based client (WebSockets)


 License
This project is intended for educational and learning purposes. You are free to modify and extend it as needed.


 Author
Vijayan
Software Engineer


MPCC demonstrates a clean, reliable, and extensible foundation for multi‑client networked applications using C++ sockets and threads.
