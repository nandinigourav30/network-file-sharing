# Network File Sharing (Capstone Project)

A simple Linux-based client-server file sharing program written in C.
The server stores files under `server_files/`. The client can:
- LIST files on the server
- UPLOAD `<filename>` from client -> server
- DOWNLOAD `<filename>` from server -> client
- EXIT to close connection

## Requirements
- Linux (Ubuntu / WSL recommended)
- GCC (`build-essential`)
- pthreads (part of glibc; compile with `-pthread`)

## Files
- `server.c` — multi-client TCP server (handles LIST, UPLOAD, DOWNLOAD, EXIT)
- `client.c` — client CLI to interact with the server
- `Makefile` — build helper
- `server_files/` — directory where server stores uploaded files

## Build
```bash
# from project root
make
