MPCC (Multi‑Party Chat Conference) is a multi‑client chat system built using TCP sockets.
It supports real‑time messaging between multiple users, with each user handled by a dedicated thread on the server.
The system consists of:

A Server → manages client connections, registration/login, message routing, and disconnection.
A Client → connects to the server, registers/logs in, sends/receives messages, and can exit gracefully.

The project uses a simple text‑based protocol over sockets.

✅ Key Features
Server

Accepts multiple clients via a TCP listener.
Spawns a ClientHandler thread for each connection. [capgemini-...epoint.com]
Handles:

User registration or login
Message receive loops
Exit commands


Gracefully shuts down on Ctrl+C.
Supports multiple restart cycles (verified from logs).

Client

Connects to the server on a specified port.
Registers/logs in with a username.
Enters a receive loop for incoming messages.
Can send EXIT to disconnect cleanly.
Successfully reconnects after server restarts. [capgemini-...epoint.com]


🗂️ Project Structure (Typical Layout)
/mpcc
│── server/
│   ├── mpcc_server.cpp
│   ├── client_handler.cpp
│   ├── client_handler.h
│   └── ...
│
│── client/
│   ├── mpcc_client.cpp
│   └── ...
│
│── logs/
│   ├── mpcc_server.log
│   └── mpcc_client.log
│
│── README.md
└── Makefile / build scripts


⚙️ How It Works
1. Server Startup
Server begins listening on a port (e.g., 8080, 8088, 9999).
Seen in logs:
MPCC Server listening on port 8080
Press Ctrl-C to shut down.
```[1](https://capgemini-my.sharepoint.com/personal/vijayan_b_vijayan_capgemini_com/Documents/Microsoft%20Copilot%20Chat%20Files/mpcc_client.log)

---

### **2. Client Connection**
Clients connect from `127.0.0.1` as shown consistently:  

Connected to 127.0.0.1:8080
C++Show more lines
New connection accepted from 127.0.0.1
ClientHandler::run() – new client
[1](https://capgemini-my.sharepoint.com/personal/vijayan_b_vijayan_capgemini_com/Documents/Microsoft%20Copilot%20Chat%20Files/mpcc_client.log)

---

### **3. User Registration / Login**
Server detects user registration/login:  

New user registered: Shiva
User logged in: Shiva
C++Show more lines
recv_loop started for user: Shiva
[1](https://capgemini-my.sharepoint.com/personal/vijayan_b_vijayan_capgemini_com/Documents/Microsoft%20Copilot%20Chat%20Files/mpcc_client.log)

---

### **5. Graceful Exit**
When a client sends `EXIT`, server logs:  

User Niya sent EXIT.
ClientHandler: client Niya disconnecting.
C++Show more lines
Server::shutdown() – setting running=false
Server shutdown complete.
C++Show more lines
Client
Shellg++ mpcc_client.cpp -o client./client 127.0.0.1 8080Show more lines

📡 Protocol
Client → Server Commands

CommandDescriptionREGISTER <name>First‑time user registrationLOGIN <name>User loginMSG <text>Broadcast message to other usersEXITDisconnect gracefully
Server → Client Messages

TypeDescriptionSystem messagesLogin success, welcome, etc.Broadcast messagesMessages from other connected usersError messagesInvalid command / format

✅ Verified Behavior (Based on Logs)
The logs you provided confirm:

Multiple clients connect successfully.
Server handles each with its own thread.
Registration and login are functioning.
EXIT command works flawlessly.
Server is stable across multiple restarts.
No crashes, no socket errors, no unexpected disconnects.
 [capgemini-...epoint.com], [capgemini-...epoint.com]


🔧 Future Enhancements
You can extend MPCC with:

Chat rooms / channels
Private messaging
File sharing
WebSocket layer
GUI client
Authentication with passwords
Encrypted transport (TLS)
Persistent chat history
