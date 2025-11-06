#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

void upload_file(int sock, char *filename);
void download_file(int sock, char *filename);
void list_files(int sock);

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024], command[256];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    printf("✅ Connected to the server.\n");
    printf("Available commands: LIST | UPLOAD <file> | DOWNLOAD <file> | EXIT\n");

    while (1) {
        printf("\nEnter command: ");
        fflush(stdout);

        memset(command, 0, sizeof(command));
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;  // remove newline

        if (strncmp(command, "LIST", 4) == 0) {
            send(sock, command, strlen(command), 0);
            list_files(sock);
        }
        else if (strncmp(command, "UPLOAD", 6) == 0) {
            send(sock, command, strlen(command), 0);
            char filename[256];
            sscanf(command + 7, "%s", filename);
            upload_file(sock, filename);
        }
        else if (strncmp(command, "DOWNLOAD", 8) == 0) {
            send(sock, command, strlen(command), 0);
            char filename[256];
            sscanf(command + 9, "%s", filename);
            download_file(sock, filename);
        }
        else if (strncmp(command, "EXIT", 4) == 0) {
            send(sock, command, strlen(command), 0);
            printf("👋 Disconnected from server.\n");
            break;
        }
        else {
            printf("⚠️ Unknown command.\n");
        }
    }

    close(sock);
    return 0;
}

// Upload file to server
void upload_file(int sock, char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("File open failed");
        return;
    }

    printf("📤 Uploading %s to server...\n", filename);

    char buffer[1024];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(sock, buffer, bytes_read, 0);
    }

    // Signal end of file
    send(sock, "EOF", 3, 0);
    fclose(fp);

    char response[100];
    recv(sock, response, sizeof(response), 0);
    printf("✅ Server response: %s\n", response);
}

// Download file from server
void download_file(int sock, char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("File create failed");
        return;
    }

    printf("📥 Downloading %s from server...\n", filename);

    char buffer[1024];
    int bytes_received;

    while ((bytes_received = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        if (strncmp(buffer, "EOF", 3) == 0)
            break;
        fwrite(buffer, 1, bytes_received, fp);
        if (bytes_received < sizeof(buffer))
            break;
    }

    fclose(fp);
    printf("✅ File %s downloaded successfully.\n", filename);
}

// List files from server
void list_files(int sock) {
    printf("📂 Files on server:\n");
    char buffer[1024];
    int bytes_received;
    while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        if (strncmp(buffer, "END", 3) == 0)
            break;
        printf("%s", buffer);
    }
}
