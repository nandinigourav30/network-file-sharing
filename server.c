#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8080
#define FILE_DIR "server_files"
#define XOR_KEY 0x5A

void *handle_client(void *arg);
void send_file_list(int client_sock);
void handle_upload(int client_sock, char *filename);
void handle_download(int client_sock, char *filename);
void xor_encrypt_decrypt(char *data, int len);

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    pthread_t tid;

    mkdir(FILE_DIR, 0777);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_sock, 5) == 0)
        printf("🚀 Server listening on port %d...\n", PORT);
    else {
        perror("Listen failed");
        exit(1);
    }

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

        buffer[strcspn(buffer, "\n")] = 0;

        if (strncasecmp(buffer, "LIST", 4) == 0) {
            send_file_list(client_sock);
        } else if (strncasecmp(buffer, "UPLOAD", 6) == 0) {
            char filename[256];
            sscanf(buffer + 7, "%s", filename);
            handle_upload(client_sock, filename);
        } else if (strncasecmp(buffer, "DOWNLOAD", 8) == 0) {
            char filename[256];
            sscanf(buffer + 9, "%s", filename);
            handle_download(client_sock, filename);
        } else if (strncasecmp(buffer, "EXIT", 4) == 0) {
            printf("[-] Client disconnected.\n");
            break;
        } else {
            send(client_sock, "Unknown command\n", 16, 0);
        }
    }

    close(client_sock);
    printf("[x] Client thread exiting.\n");
    return NULL;
}

void send_file_list(int client_sock) {
    DIR *d;
    struct dirent *dir;
    d = opendir(FILE_DIR);
    if (!d) {
        send(client_sock, "Error opening directory\n", 25, 0);
        return;
    }

    char buffer[1024];
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) {
            snprintf(buffer, sizeof(buffer), "%s\n", dir->d_name);
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }
    closedir(d);
    send(client_sock, "__END__", 7, 0);
}

void handle_upload(int client_sock, char *filename) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", FILE_DIR, filename);

    FILE *fp = fopen(full_path, "wb");
    if (!fp) {
        perror("File open error");
        send(client_sock, "ERR", 3, 0);
        return;
    }

    char buffer[1024];
    int bytes_received;
    while ((bytes_received = recv(client_sock, buffer, sizeof(buffer), 0)) > 0) {
        if (strstr(buffer, "EOF") != NULL)
            break;
        xor_encrypt_decrypt(buffer, bytes_received);
        fwrite(buffer, 1, bytes_received, fp);
    }

    fclose(fp);
    printf("[+] Received file: %s\n", filename);
    send(client_sock, "OK", 2, 0);
}

void handle_download(int client_sock, char *filename) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", FILE_DIR, filename);

    FILE *fp = fopen(full_path, "rb");
    if (!fp) {
        perror("File open error");
        send(client_sock, "ERR", 3, 0);
        return;
    }

    char buffer[1024];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        xor_encrypt_decrypt(buffer, bytes_read);
        send(client_sock, buffer, bytes_read, 0);
    }

    fclose(fp);
    send(client_sock, "EOF", 3, 0);
    printf("[+] Sent file: %s\n", filename);
}

void xor_encrypt_decrypt(char *data, int len) {
    for (int i = 0; i < len; i++) {
        data[i] ^= XOR_KEY;
    }
}

    char buffer[1024];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        xor_encrypt_decrypt(buffer, bytes_read);
        send(client_sock, buffer, bytes_read, 0);
    }

    fclose(fp);
    send(client_sock, "EOF", 3, 0);
    printf("[+] Sent file: %s\n", filename);
}

void xor_encrypt_decrypt(char *data, int len) {
    for (int i = 0; i < len; i++) {
        data[i] ^= XOR_KEY;
    }
}
