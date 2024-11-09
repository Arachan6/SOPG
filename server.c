#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 5000
#define BUFFER_SIZE 128
#define RESPONSE_SIZE 256

void close_socket(int sock){
    if (close(sock) < 0) {
        perror("No se pudo cerrar el socket\n");
    }
}

void execute_command(const char *command, int client_socket) {
    char response[RESPONSE_SIZE] = {0}; 
    char key[BUFFER_SIZE] = {0}; 
    char value[BUFFER_SIZE] = {0}; 
    FILE *file;

    if (sscanf(command, "SET %s %s", key, value) == 2) {
        file = fopen(key, "w");
        if (file) {
            fprintf(file, "%s", value);
            fclose(file);
            snprintf(response, sizeof(response), "OK\n");
        } else {
            snprintf(response, sizeof(response), "Error del servidor\n");
            perror("No se pudo crear el archivo\n");
        }
    } else if (sscanf(command, "GET %s", key) == 1) {
        file = fopen(key, "r");
        if (file) {
            fgets(value, sizeof(value), file);
            fclose(file);
            snprintf(response, sizeof(response), "OK\n%s\n", value);
        } else {
            if (errno == ENOENT) {
                snprintf(response, sizeof(response), "NOTFOUND\n");
            } else {
                snprintf(response, sizeof(response), "Error del servidor\n");
                perror("No se pudo abrir el archivo\n");
            }
        }
    } else if (sscanf(command, "DEL %s", key) == 1) {
        if (remove(key) == 0) {
            snprintf(response, sizeof(response), "OK\n");
        } else {
            if (errno == ENOENT) {
                snprintf(response, sizeof(response), "OK\n");
            } else {
                snprintf(response, sizeof(response), "Error del servidor\n");
                perror("No se pudo remover el archivo\n");
            }
        }
    } else {
        fprintf(stdout, "Comando no reconocido\n");
    }

    write(client_socket, response, strlen(response));
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int n = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (n < 0) {
        perror("No se pudo leer los datos enviados por el cliente\n");
        close_socket(client_socket); 
        return;
    }
    buffer[n] = '\0';
    fprintf(stdout, "Mensaje recibido: %s\n", buffer);

    execute_command(buffer, client_socket);
    close_socket(client_socket); 
}

int main(void) {

    // Se crea el socket del servidor
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("No se pudo crear el socket del servidor\n");
        exit(EXIT_FAILURE);
    }

    // Definición, inicialización y configuración de estructura sockaddr_in
    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(PORT);

    // Se asocia el socket a la dirección y puerto especificados en serveraddr
    if (bind(server_socket, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("No se pudo abrir el socket del servidor\n");
        close_socket(server_socket); 
        exit(EXIT_FAILURE);
    }

    // Se configura el socket para escuchar conexiones entrantes
    if (listen(server_socket, 1) < 0) {
        perror("No se pudo configurar el socket en listen\n");
        close_socket(server_socket); 
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Servidor TCP activo\n");
    fprintf(stdout, "Esperando conexiones en el puerto %d...\n", PORT);

    while (1) {
        // Se espera la conexión de algun cliente
        struct sockaddr_in clientaddr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        int client_socket = accept(server_socket, (struct sockaddr *)&clientaddr, &addr_len);
        if (client_socket < 0) {
            perror("No se pudo aceptar conexión entrante\n");
            continue;
        }

        // Se imprime dirección de cliente
        char ipClient[32];
        inet_ntop(AF_INET, &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
        fprintf(stdout, "Cliente conectado desde:  %s\n", ipClient);

        handle_client(client_socket);
    }

    close_socket(server_socket); 
    return EXIT_SUCCESS;
}
