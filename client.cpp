#include <iostream>
#include <sys/socket.h> // Para sockets en general
#include <sys/un.h> // Para los sockets UNIX 
#include <unistd.h>
#include <thread>

using namespace std;

int MAX_CLIENTS = 5;
int MAX_MSG_SIZE = 1024;

void sender(int client_fd) {
    // 3. Enviamos un mensaje al servidor usando send

    while (1) {
        string input_usuario;

        cin >> input_usuario;

        if (input_usuario == "") {
            continue;
        }

        const char* message = input_usuario.c_str();

        ssize_t bytes_sent = send(client_fd, message, strlen(message), 0);
        if (bytes_sent == -1) {
            cerr << "Error al enviar datos al socket" << endl;
            exit(1);
        }

        if (input_usuario == "/quit") {
            // 5. Cerramos el socket
            close(client_fd);
            exit(0);
        }
    }
    exit(0);
}

void receiver(int client_fd) {
    // 4. Leemos la respuesta del servidor usando recv

    while (1) {
        char buffer[MAX_MSG_SIZE];
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received == -1) {
            cerr << "Error al recibir datos del socket" << endl;
            exit(1);
        }

        cout << "Respuesta del servidor: " << string(buffer, bytes_received) << endl;
    }
    exit(0);
}

// Ejemplo con UNIX sockets
int main(){
    // 1. Creamos el socket
    int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        cerr << "Error al crear el socket" << endl;
        return 1;
    }

    // 2. Conectamos al servidor
    sockaddr_un address;
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, "/tmp/socket-example");

    int connect_result = connect(client_fd, (sockaddr*)&address, sizeof(address));
    if (connect_result == -1) {
        cerr << "Error al conectar al servidor" << endl;
        return 1;
    }

    thread sender_t(sender, client_fd);
    thread receiver_t(receiver, client_fd);

    sender_t.join();
    receiver_t.join();

    return 0;
}