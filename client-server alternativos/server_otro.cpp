#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <algorithm> 
#include <mutex>

using namespace std;

int MAX_MSG_SIZE = 1024;
int PORT = 8080;

vector<int> clients;      // Lista de clientes conectados
mutex clients_mutex;      // Mutex para acceso seguro a la lista

// Función que maneja la comunicación con un cliente
void handle_client(int client_fd) {
    char buffer[MAX_MSG_SIZE];

    while (true) {
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            // Cliente desconectado o error
            cout << "Cliente desconectado: " << client_fd << endl;
            close(client_fd);

            // Remover cliente de la lista
            clients_mutex.lock();
            clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
            clients_mutex.unlock();

            break;
        }

        string msg(buffer, bytes_received);
        cout << "Mensaje recibido de " << client_fd << ": " << msg << endl;

        // Reenviar el mensaje a todos los demás clientes
        clients_mutex.lock();
        for (int other_fd : clients) {
            if (other_fd != client_fd) {
                send(other_fd, buffer, bytes_received, 0);
            }
        }
        clients_mutex.unlock();
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        cerr << "Error al crear el socket" << endl;
        return 1;
    }

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (::bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Error al vincular el socket al puerto" << endl;
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        cerr << "Error al escuchar en el socket" << endl;
        return 1;
    }

    cout << "Servidor escuchando en el puerto " << PORT << "..." << endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            cerr << "Error al aceptar la conexión" << endl;
            continue;
        }

        cout << "Nuevo cliente conectado: " << client_fd << endl;

        // Añadir cliente a la lista protegida por mutex
        clients_mutex.lock();
        clients.push_back(client_fd);
        clients_mutex.unlock();

        // Crear un nuevo thread para manejar este cliente
        thread t(handle_client, client_fd);
        t.detach();  // Desvincula el thread para que corra independiente
    }

    close(server_fd);
    return 0;
}
