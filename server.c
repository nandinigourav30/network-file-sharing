#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>

#define PORT 8080
#define FILE_DIR "server_files"

// Function declarations
void *handle_client(void *arg);
void send_file_list(int client_sock);
void handle_upload(int client_sock, char *filename);
void handle_download(int client_sock, char *filename);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    pthread_t tid;

    // Create directory if not exists
    mkdir(FILE_DIR, 0777);

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    // Listen
    if (listen(server_sock, 5) == 0)
        printf("🚀 Server listening on port %d...\n", PORT);
    else {
        perror("Listen failed");
        exit(1);
    }

    // Accept clients
    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock >= 0) {
            printf("[+] Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
            pthread_create(&tid, NULL, handle_client, (void*)&client_sock);
            pthread_detach(tid);
        }
    }

    close(server_sock);
    return 0;
}

// Handle each client
void *handle_client(void *arg) {
    int client_sock = *((int*)arg);
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            printf("[-] Client disconnected.\n");
            break;
        }

        // Trim newline
        buffer[strcspn(buffer, "\n")] = 0;

        if (strncmp(buffer, "LIST", 4) == 0) {
            send_file_list(client_sock);
        }
        else if (strncmp(buffer, "UPLOAD", 6) == 0) {
            char filename[256];
            sscanf(buffer + 7, "%s", filename);
            handle_upload(client_sock, filename);
        }
        else if (strncmp(buffer, "DOWNLOAD", 8) == 0) {
            char filename[256];
            sscanf(buffer + 9, "%s", filename);
            handle_download(client_sock, filename);
        }
        else if (strncmp(buffer, "EXIT", 4) == 0) {
            printf("[-] Client disconnected.\n");
            break;
        }
        else {
            send(client_sock, "Unknown command\n", 16, 0);
        }
    }

    close(client_sock);
    printf("[x] Client thread exiting.\n");
    return NULL;
}

// Send list of files in server_files
void send_file_list(int client_sock) {
    system("ls server_files > temp.txt");
    FILE *fp = fopen("temp.txt", "r");
    if (fp == NULL) {
        send(client_sock, "Error listing files\n", 20, 0);
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp)) {
        send(client_sock, buffer, strlen(buffer), 0);
    }
    fclose(fp);
    remove("temp.txt");
    send(client_sock, "END", 3, 0);
}

// Handle file upload
void handle_upload(int client_sock, char *filename) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", FILE_DIR, filename);

    FILE *fp = fopen(full_path, "wb");
    if (fp == NULL) {
        perror("File open error");
        send(client_sock, "ERR", 3, 0);
        return;
    }

    char file_buffer[1024];
    int bytes_received;

    while ((bytes_received = recv(client_sock, file_buffer, sizeof(file_buffer), 0)) > 0) {
        if (strncmp(file_buffer, "EOF", 3) == 0)
            break;
        fwrite(file_buffer, 1, bytes_received, fp);
        if (bytes_received < sizeof(file_buffer))
            break;
    }

    fclose(fp);
    printf("[+] Received file: %s\n", filename);
    send(client_sock, "OK", 2, 0);
}

// Handle file download
void handle_download(int client_sock, char *filename) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", FILE_DIR, filename);

    FILE *fp = fopen(full_path, "rb");
    if (fp == NULL) {
        perror("File open error");
        send(client_sock, "ERR", 3, 0);
        return;
    }

    char buffer[1024];
    int bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(client_sock, buffer, bytes_read, 0);
    }

    fclose(fp);
    send(client_sock, "EOF", 3, 0);
    printf("[+] Sent file: %s\n", filename);
}
