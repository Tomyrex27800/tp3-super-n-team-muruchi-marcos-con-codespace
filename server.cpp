#include "threadpool.h"
#include "hangman.h"

#include <iostream>
#include <sys/socket.h> // Para sockets en general
#include <sys/un.h> // Para los sockets UNIX 
#include <unistd.h>
#include <thread>
#include <algorithm>
#include <iterator>

#include <chrono>

using namespace std;

int MAX_CLIENTS = 0;
int MAX_MSG_SIZE = 1024;

bool joinable = false;
int admin_connection = 0;
vector<int> connections_fd;
mutex connections_fd_mutex;
mutex joinable_mutex;

Hangman game;
mutex hangman_mutex;


void connectedThread(int client_fd) {
    char buffer[MAX_MSG_SIZE];


    const char* message = "Introducí tu nombre:";
    ssize_t bytes_sent = send(client_fd, message, strlen(message), 0);
    if (bytes_sent < 0) {
        cerr << "Error al enviar datos al socket" << endl;
        exit(1);
    }

    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_received < 0) {
        cerr << "Error al recibir datos del socket" << endl;
        exit(1);
    }

    const string player_name = string(buffer, bytes_received);

    hangman_mutex.lock();
    game.addPlayer(player_name);
    hangman_mutex.unlock();




    while (1) {
        // 5. Leemos datos del cliente usando recv
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            cerr << "Error al recibir datos del socket" << endl;
            exit(1);
        }

        const string message_received = string(buffer, bytes_received);

        if (message_received == "/quit") {
            // 7. Cerramos el socket
            connections_fd_mutex.lock();

            auto it = find(connections_fd.begin(), connections_fd.end(), client_fd);
            if (it != connections_fd.end()) {
                connections_fd.erase(it);
            }
            connections_fd_mutex.unlock();

            close(client_fd);
            break;
        }

        cout << "Mensaje recibido del cliente: " << message_received << endl;
    }
}

// Ejemplo con UNIX sockets
void connection_handler(){
    // 0. pool
    ThreadPool pool;
    
    // 1. Creamos el socket
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        cerr << "Error al crear el socket" << endl;
        exit(1);
    }

    // 2. Vinculamos el socket a una dirección (en este caso, como un socket UNIX, usamos un archivo)
    sockaddr_un address;
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, "/tmp/socket-hangman");
    unlink(address.sun_path);  // Aseguramos que el socket no exista 

    // el :: es para salir del namespace std en esta llamada ya que existe un std::bind que no es el que queremos usar
    int bind_result = ::bind(server_fd, (sockaddr*)&address, sizeof(address)); 
    if (bind_result < 0) {
        cerr << "Error al vincular el socket" << endl;
        exit(1);
    }

    // 3. Escuchamos conexiones entrantes
    int listen_result = listen(server_fd, MAX_CLIENTS); // 5 es el número máximo de conexiones en cola
    if (listen_result < 0) {
        cerr << "Error al escuchar en el socket" << endl;
        exit(1);
    } else {
        cout << "Escuchando en el socket..." << endl;
    }

    while (1) {
        // espera y acepta conexiones (se rechazan despues si no hay espacio o hay partida en curso)
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            cerr << "Error al aceptar la conexión" << endl;
            exit(1);
        }

        // si hay una partida en curso o hay 5 clientes se kickea
        unique_lock<mutex> lock_joinable(joinable_mutex);
        unique_lock<mutex> lock_connections(connections_fd_mutex);
        if (connections_fd.size() >= 5 || !joinable) {
            // se devuelve un mensaje que indica un cierre de conexion, el cliente se cierra automaticamente
            const char* message = "/client_quit";

            ssize_t bytes_sent = send(client_fd, message, strlen(message), 0);
            if (bytes_sent == -1) {
                cerr << "Error al enviar datos al socket" << endl;
                exit(1);
            }

            // se cierra la conexion con el cliente
            close(client_fd);
            cout << "Se rechazó una conexión." << endl;
            continue;
        }

        // se pasa la conexion a un thread del pool
        pool.enqueue([client_fd] {connectedThread(client_fd);});

        // se añade la conexion a la lista de clientes actuales
        connections_fd.emplace_back(client_fd);
        cout << "Se unió " << client_fd << endl;

        // si no hay admin de la sala, se establece la conexion entrante como admin
        if (admin_connection == 0) {
            admin_connection = client_fd;
            cout << "Nuestro administrador es " << admin_connection << endl;
        }
    }

    close(server_fd);
}

int main() {
    thread connection_handler_thread(connection_handler);

    cout << "=== JUEGO DEL AHORCADO ===" << endl;
    cout << "Temática: Sistemas Operativos, Redes, IPC, Sincronización\n" << endl;

    cout << "Agregando jugadores..." << endl;

    joinable_mutex.lock();
    joinable = true;
    joinable_mutex.unlock();

    std::this_thread::sleep_for(chrono::seconds(10));

    joinable_mutex.lock();
    joinable = false;
    joinable_mutex.unlock();

    connections_fd_mutex.lock();
    for (int i = 0; i < connections_fd.size(); i++) {
        // 6. Escribimos datos de vuelta al cliente usando send
        int client_fd_i = connections_fd[i];
        cout << "Redirigiendo a " << client_fd_i << endl;

        const char* message = "HOLA A LOS CONECTADOS!!";
        ssize_t bytes_sent = send(client_fd_i, message, strlen(message), 0);
        if (bytes_sent < 0) {
            cerr << "Error al enviar datos al socket" << endl;
            return 1;
        }
    }
    cout << "Hay " << connections_fd.size() << " jugadores en el servidor." << endl;
    connections_fd_mutex.unlock();

    connection_handler_thread.join();
    return 0;
}
