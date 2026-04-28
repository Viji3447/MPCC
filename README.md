
MPCC (Multi‑Party Chat Conference)
MPCC is a lightweight, multi‑client chat system implemented in C++ using TCP sockets and a thread‑per‑client server architecture. It enables real‑time message exchange between multiple connected users through a simple, text‑based protocol.
The project demonstrates practical socket programming, multithreading, and robust connection lifecycle management, including graceful server shutdowns and reliable client reconnections.


✨ Features
🖥️ Server

TCP listener supporting multiple simultaneous client connections
Dedicated ClientHandler thread for each connected client
User registration and login functionality
Broadcast messaging to all connected users
Graceful handling of client EXIT commands
Clean shutdown using Ctrl + C (SIGINT)
Stable across multiple restart cycles (verified via runtime logs)
💻 Client

Connects to the server using a configurable host and port
Supports user registration and login via text‑based commands
Asynchronous receive loop for incoming messages
Clean disconnection using the EXIT command
Reliable reconnection after server restarts


🏗️ Architecture Overview
+-------------------+            +-------------------+
|    MPCC Client    | <——TCP——>  |    MPCC Server    |
|-------------------|            |-------------------|
| • Connect / Login |            | • Accept clients  |
| • Send messages   |            | • Spawn threads   |
| • Receive loop    |            | • Route messages  |
| • EXIT handling   |            | • Handle EXIT     |
+-------------------+            +-------------------+



The server listens on a specified TCP port and accepts incoming client connections.
Each client connection is handled by a dedicated ClientHandler thread.
Messages sent by one client are broadcast to all other connected clients.
All communication follows a simple, human‑readable text protocol.


📂 Project Structure
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




📡 Communication Protocol
➜ Client → Server Commands

Command	Description
REGISTER <name>	Register as a new user
LOGIN <name>	Login as an existing user
MSG <text>	Broadcast a message to all connected users
EXIT	Disconnect gracefully from the server

➜ Server → Client Messages

System messages (login success, welcome notifications)
Broadcast messages from other connected users
Error messages for invalid commands or incorrect formats


▶️ Build & Run
✅ Prerequisites

GCC / G++ (C++11 or later)
POSIX‑compatible OS (Linux or macOS)
🔨 Compile
Build the server:
g++ server/mpcc_server.cpp server/client_handler.cpp -o mpcc_server -pthread


Build the client:
g++ client/mpcc_client.cpp -o mpcc_client


▶️ Run Server
./mpcc_server <port>

# Example
./mpcc_server 8080


▶️ Run Client
./mpcc_client <server_ip> <port>

# Example
./mpcc_client 127.0.0.1 8080




✅ Verified Behavior
Based on runtime logs and testing:

Multiple clients connect successfully from 127.0.0.1
Each client connection is handled by a separate server thread
User registration and login function as expected
Messages are broadcast reliably in real time
EXIT command triggers clean client disconnection
Server shuts down gracefully and restarts without issues
Clients reconnect successfully after server restarts


🔧 Future Enhancements
Potential extensions to the MPCC system include:

Chat rooms or channels
Private (one‑to‑one) messaging
Password‑based authentication
TLS‑encrypted communication
Persistent chat history (database‑backed)
File transfer support
GUI or web‑based client (WebSockets)


📜 License
This project is intended for educational and learning purposes. You are free to modify and extend it as needed.


👤 Author
Vijayan
Software Engineer


MPCC provides a clean, reliable, and extensible foundation for building multi‑client networked applications using C++ sockets and multithreading.
