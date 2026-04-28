
💬 MPCC (Multi‑Party Chat Conference)


📌 Project Overview
MPCC is a C++‑based multi‑client chat system built using TCP socket programming and a thread‑per‑client server architecture. It enables real‑time message exchange between multiple users through a simple, text‑based protocol.
This project demonstrates strong fundamentals of computer networking, multithreading, and robust connection lifecycle handling, including graceful server shutdowns and reliable client reconnections.


🧩 Features
🖥️ Server

TCP listener supporting multiple simultaneous client connections
Dedicated ClientHandler thread for each connected client
User registration and login handling
Broadcast messaging to all connected clients
Graceful processing of client EXIT commands
Clean shutdown on Ctrl + C (SIGINT)
Stable across multiple restart cycles (verified via logs)
💻 Client

Connects to server using configurable IP address and port
Supports user registration and login via text‑based commands
Asynchronous receive loop for real‑time incoming messages
Clean disconnection using the EXIT command
Successful reconnection after server restarts


🏗️ Architecture Overview
+-------------------+            +-------------------+
|    MPCC Client    | <——TCP——>  |    MPCC Server    |
|-------------------|            |-------------------|
| • Connect / Login |            | • Accept clients  |
| • Send messages   |            | • Spawn threads   |
| • Receive loop    |            | • Route messages  |
| • EXIT handling   |            | • Handle EXIT     |
+-------------------+            +-------------------+



The server listens on a specified TCP port and accepts incoming connections.
Each connected client is managed by a dedicated ClientHandler thread.
Messages from one client are broadcast to all other connected clients.
All interactions follow a simple text‑based communication protocol.


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
EXIT	Disconnect gracefully

➜ Server → Client Messages

System notifications (login success, welcome messages)
Broadcast messages from other connected users
Error messages for invalid commands or incorrect formats


🛠️ Technologies Used

Technology	Usage
C++ (C++11)	Core implementation language
POSIX Sockets	TCP networking
std::thread	Multi‑client handling
STL	Data structures & utilities
pthread	Thread synchronization



🚀 How to Run
✅ Prerequisites

GCC / G++ (C++11 or later)
POSIX‑compatible OS (Linux / macOS)
🔨 Compile
# Build Server
g++ server/mpcc_server.cpp server/client_handler.cpp -o mpcc_server -pthread

# Build Client
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

Multiple clients successfully connect from 127.0.0.1
Each client is handled by a separate server thread
Registration and login workflows function correctly
Messages are delivered in real time via broadcast
EXIT command triggers clean client disconnection
Server shuts down gracefully and restarts reliably
Clients reconnect successfully after server restarts


💡 Future Scope

Chat rooms / channels
Private (one‑to‑one) messaging
Password‑based authentication
TLS‑encrypted communication
Persistent chat history (database‑backed)
File transfer support
GUI or Web‑based client (WebSockets)


📜 License
This project is intended for educational and learning purposes. You are free to modify and extend it as needed.


👤 Author
Vijayan P
Software Engineer


MPCC serves as a clean, reliable, and extensible foundation for multi‑client networked applications using C++ sockets and multithreading.
