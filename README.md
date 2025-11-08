# Network File Sharing System  
### 💻 Capstone Project — Client–Server Communication using Sockets in C

---

## 📘 Project Overview
This project implements a **Client–Server File Sharing System** in the **C programming language** using **TCP sockets** on Linux.  
It demonstrates the fundamentals of **computer networking**, **socket programming**, **multi-threading**, and **basic encryption**.  
Multiple clients can connect to a central server, list available files, upload new files, or download existing ones — all handled concurrently by the server.

---

## 🎯 Objectives
- To understand and implement TCP socket communication in C  
- To enable reliable file transfer between multiple clients and a server  
- To explore multi-threaded server architecture for handling concurrent connections  
- To implement simple XOR-based encryption during data transmission  
- To gain hands-on experience with Linux system calls and networking APIs

---

## ⚙️ Features
- **LIST** — Display all available files on the server  
- **UPLOAD `<filename>`** — Send a file from client to server  
- **DOWNLOAD `<filename>`** — Retrieve a file from the server  
- **EXIT** — Disconnect safely from the server  
- **Multi-client support** using POSIX threads (`pthread`)  
- **XOR encryption** during upload/download for data privacy  
- Organized storage in a dedicated `server_files/` directory  

---

## 🧩 System Architecture
