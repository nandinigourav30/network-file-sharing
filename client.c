#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define XOR_KEY 0x5A
#define BUFFER_SIZE 1024

void upload_file(int sock, char *filename);
void download_file(int sock, char *filename);
void list_files(int sock);
void xor_encrypt_decrypt(char *data, int len);
void to_upper(char *str);

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE], command[256];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

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
        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = 0;

        if (strlen(command) == 0) continue;

        char cmd[256] = {0}, filename[256] = {0};
        sscanf(command, "%s %255s", cmd, filename);
        to_upper(cmd);

        if (strcmp(cmd, "LIST") == 0) {
            /* send LIST and read until server's __END__ */
            send(sock, "LIST", strlen("LIST"), 0);
            list_files(sock);
        } else if (strcmp(cmd, "UPLOAD") == 0) {
            if (strlen(filename) == 0) {
                printf("Usage: UPLOAD <filename>\n");
                continue;
            }
            char full_command[512];
            snprintf(full_command, sizeof(full_command), "UPLOAD %s", filename);
            send(sock, full_command, strlen(full_command), 0);
            upload_file(sock, filename);

            /* Optionally read server ACK (OK or ERR). */
            memset(buffer, 0, sizeof(buffer));
            int n = recv(sock, buffer, sizeof(buffer)-1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                printf("Server: %s\n", buffer);
            }
        } else if (strcmp(cmd, "DOWNLOAD") == 0) {
            if (strlen(filename) == 0) {
                printf("Usage: DOWNLOAD <filename>\n");
                continue;
            }
            char full_command[512];
            snprintf(full_command, sizeof(full_command), "DOWNLOAD %s", filename);
            send(sock, full_command, strlen(full_command), 0);
            download_file(sock, filename);
        } else if (strcmp(cmd, "EXIT") == 0) {
            send(sock, "EXIT", strlen("EXIT"), 0);
            printf("👋 Disconnected from server.\n");
            break;
        } else {
            printf("⚠️ Unknown command.\n");
        }
    }

    close(sock);
    return 0;
}

/* Convert string to upper case (in-place) */
void to_upper(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

/* XOR encrypt/decrypt (in-place) */
void xor_encrypt_decrypt(char *data, int len) {
    for (int i = 0; i < len; i++) {
        data[i] ^= XOR_KEY;
    }
}

/* Upload: read file, encrypt each chunk, send; then send EOF marker */
void upload_file(int sock, char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("File open failed");
        return;
    }

    printf("📤 Uploading %s to server...\n", filename);
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        /* encrypt before sending */
        xor_encrypt_decrypt(buffer, bytes_read);
        int sent = 0;
        while (sent < bytes_read) {
            int n = send(sock, buffer + sent, bytes_read - sent, 0);
            if (n <= 0) {
                perror("Send error");
                fclose(fp);
                return;
            }
            sent += n;
        }
    }

    fclose(fp);

    /* Send EOF marker (3 bytes) so server knows transfer ended */
    send(sock, "EOF", 3, 0);
    printf("✅ File upload finished (encrypted and sent).\n");
}

/* Download: receive chunks until EOF marker, decrypt and write to file */
void download_file(int sock, char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("File create failed");
        return;
    }

    printf("📥 Downloading %s from server...\n", filename);
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while (1) {
        bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            /* connection closed or error */
            break;
        }

        /* Check for EOF marker coming as exact 3 bytes */
        if (bytes_received == 3 && memcmp(buffer, "EOF", 3) == 0) {
            break;
        }

        /* It's possible (unlikely) for EOF to be appended to data; check strstr if buffer is printable */
        if (bytes_received >= 3) {
            /* Create a temporary nul-terminated copy for substring check only if data is text-like */
            char tmp[BUFFER_SIZE + 1];
            int copy_len = bytes_received < BUFFER_SIZE ? bytes_received : BUFFER_SIZE - 1;
            memcpy(tmp, buffer, copy_len);
            tmp[copy_len] = '\0';
            char *p = strstr(tmp, "EOF");
            if (p != NULL) {
                int pos = p - tmp;
                if (pos > 0) {
                    /* decrypt and write only the data before EOF */
                    xor_encrypt_decrypt(buffer, pos);
                    fwrite(buffer, 1, pos, fp);
                }
                break;
            }
        }

        /* decrypt and write */
        xor_encrypt_decrypt(buffer, bytes_received);
        fwrite(buffer, 1, bytes_received, fp);
    }

    fclose(fp);
    printf("✅ File downloaded and saved as %s\n", filename);
}

/* Read and print LIST output until server's __END__ sentinel */
void list_files(int sock) {
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while (1) {
        bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("[-] Connection closed by server or error while listing.\n");
            break;
        }
        buffer[bytes_received] = '\0';

        /* Check for sentinel __END__ anywhere in buffer */
        char *endpos = strstr(buffer, "__END__");
        if (endpos != NULL) {
            /* print up to sentinel and stop */
            *endpos = '\0';
            if (strlen(buffer) > 0) printf("%s", buffer);
            break;
        } else {
            /* print full chunk and continue */
            printf("%s", buffer);
            /* continue reading */
        }
    }
}
