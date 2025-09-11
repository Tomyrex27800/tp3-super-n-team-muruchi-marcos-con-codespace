#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>  // inet_pton
#include "hangman.h"

using namespace std;

int MAX_MSG_SIZE = 1024;
int PORT = 8080;
string IP_LOCAL = "127.0.0.1";

int main() {
    // 1. Crear el socket (AF_INET: IPv4, SOCK_STREAM: TCP)
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        cerr << "Error al crear el socket" << endl;
        return 1;
    }

    // 2. Definir la dirección del servidor
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); // Convertimos el puerto a formato de red

    // IP del servidor: 127.0.0.1 (localhost)
    if (inet_pton(AF_INET, IP_LOCAL.c_str(), &server_addr.sin_addr) <= 0) {
        cerr << "Dirección IP inválida o no soportada" << endl;
        return 1;
    }

    // 3. Conectar al servidor
    if (connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error al conectar al servidor" << endl;
        return 1;
    }
    // 3.5 Conectarse al juego ??
    string name_player;
    cout << "Ingresa tu nombre de Jugador: ";
    cin >> name_player;
    // Aca se agrega al jugador ponele


    // 4. Enviar mensaje al servidor

    ssize_t bytes_sent = send(client_fd, name_player.c_str(), name_player.size(), 0);
    if (bytes_sent == -1) {
        cerr << "Error al enviar datos al socket" << endl;
        return 1;
    }

    // 5. Recibir respuesta del servidor
    char buffer[MAX_MSG_SIZE];
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_received == -1) {
        cerr << "Error al recibir datos del socket" << endl;
        return 1;
    }

    cout << "Respuesta del servidor: " << string(buffer, bytes_received) << endl;

    // 6. Cerrar socket
    close(client_fd);

    return 0;
}
