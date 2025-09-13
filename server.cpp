#include "threadpool.h"
#include "hangman.h"

#include <iostream>
#include <sys/socket.h> // Para sockets en general
#include <sys/un.h> // Para los sockets UNIX 
#include <unistd.h>
#include <thread>
#include <algorithm>
#include <iterator>
#include <condition_variable>
#include <chrono>

using namespace std;

int MAX_CLIENTS = 0;
int MAX_MSG_SIZE = 1024;

// ----------------

// DISTINCIÓN IMPORTANTE!!: LOS JUGADORES SON AQUELLAS CONEXIONES QUE INTRODUJERON UN NOMBRE AL ENTRAR A LA SALA

// fd de la conexión admin
int admin_connection = 0;

// fd de todas las conexiones
vector<int> connections_fd;
mutex connections_fd_mutex;

// aqueyas conexiones registradas como jugadores (aquellas que introdujeron un nombre)
struct PlayerConnection {
    int player_fd;
    string name;
    bool is_admin;

    PlayerConnection(string player_name, int fd, bool is_adm) : name(player_name), player_fd(fd), is_admin(is_adm) {};
};
vector<PlayerConnection> connections_players;
mutex connections_players_mutex;

// -----------------

// juego
Hangman game;
mutex hangman_mutex;

// condition variables
condition_variable cv_action;
mutex cv_action_mutex;

// -----------------

// cierra una conexión
void kickConnection(int client_fd) {
    connections_fd_mutex.lock();
    const char* message = "/client_quit";

    ssize_t bytes_sent = send(client_fd, message, strlen(message), 0);
    if (bytes_sent == -1) {
        cerr << "Error al enviar datos al socket" << endl;
        exit(1);
    }

    auto it = find(connections_fd.begin(), connections_fd.end(), client_fd);
    if (it != connections_fd.end()) {
        connections_fd.erase(it);
    }
    connections_fd_mutex.unlock();

    // se cierra la conexion con el cliente
    close(client_fd);

    cout << "Se cerró una conexión de cliente." << endl;
}

// lógica de un thread de conexión a cliente
void connectedThread(int client_fd) {
    char buffer[MAX_MSG_SIZE];

    // introducir nombre del jugador.
    const char* message = "Introducí tu nombre:";
    ssize_t bytes_sent = send(client_fd, message, strlen(message), 0);
    if (bytes_sent < 0) {
        cerr << "Error al enviar datos al socket" << endl;
        exit(1);
    }

    // recibe el nombre del jugador
    ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_received < 0) {
        cerr << "Error al recibir datos del socket" << endl;
        exit(1);
    }

    const string player_name = string(buffer, bytes_received);

    // se intenta añadir el jugador a la partida
    // si el jugador no se puede añadir a la partida, se kickea la conexión
    unique_lock<mutex> lock_hangman(hangman_mutex);

    bool player_join_successful = game.addPlayer(player_name);
    if (player_join_successful) {
        unique_lock<mutex> lock_connections_players(connections_players_mutex);
        unique_lock<mutex> lock_connections_fd(connections_fd_mutex);

        PlayerConnection new_player(player_name, client_fd, false);
        connections_players.emplace_back(new_player);

        // si no hay admin de la sala, se establece este jugador como admin
        if (admin_connection == 0) {
            admin_connection = client_fd;
            cout << "Nuestro administrador es " << player_name << endl;
        }
    } else {
        kickConnection(client_fd);
        return;
    }

    lock_hangman.unlock();

    // A PARTIR DE ACÁ, LA CONEXIÓN ES UN JUGADOR EN LA PARTIDA.

    // comandos. No utilicé case switch, no funciona con strings
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

        if (message_received == "/start" && admin_connection == client_fd) {
            {
                unique_lock<mutex> lock_hangman(hangman_mutex);
                game.startGame();
            }
            cv_action.notify_all();

            continue;
        }

        cout << "\n* Mensaje de " << player_name <<": \n" << message_received << "\n" << endl;
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
        cout << "Esperando jugadores...\nNota: cada jugador debe ingresar su nombre para jugar." << endl;
    }

    while (1) {
        // espera y acepta conexiones (se rechazan despues si no hay espacio o no se reciben jugadores)
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            cerr << "Error al aceptar la conexión" << endl;
            exit(1);
        }

        // si no se estan esperando jugadores nuevos o hay 5 clientes se kickea
        unique_lock<mutex> lock_hangman(hangman_mutex);
        unique_lock<mutex> lock_connections(connections_fd_mutex);
        if (connections_fd.size() >= 5 || game.getGameState() != GameState::WAITING_FOR_PLAYERS) {
            // se devuelve un mensaje que indica un cierre de conexion, el cliente se cierra automaticamente
            const char* message = "/client_quit";

            ssize_t bytes_sent = send(client_fd, message, strlen(message), 0);
            if (bytes_sent == -1) {
                cerr << "Error al enviar datos al socket" << endl;
                exit(1);
            }

            // se cierra la conexion con el cliente
            close(client_fd);
            cout << "Se cerró una conexión de cliente." << endl;
            continue;
        }

        // se pasa la conexion a un thread del pool
        pool.enqueue([client_fd] {connectedThread(client_fd);});

        // se añade la conexion a la lista de clientes actuales
        connections_fd.emplace_back(client_fd);
        cout << "Se unió " << client_fd << endl;
    }

    close(server_fd);
}

// enviar un mensaje a todas las conexiones
void broadcast_message(string message_to_send){
    connections_fd_mutex.lock();
    for (int i = 0; i < connections_fd.size(); i++) {
        // 6. Escribimos datos de vuelta al cliente usando send
        int client_fd_i = connections_fd[i];
        //cout << "Redirigiendo a " << client_fd_i << endl;

        const char* message = message_to_send.c_str();
        ssize_t bytes_sent = send(client_fd_i, message, strlen(message), 0);
        if (bytes_sent < 0) {
            cerr << "Error al enviar datos al socket" << endl;
            exit(1);
        }
    }
    connections_fd_mutex.unlock();
}

// enviar un mensaje a todos los jugadores de la partida
void broadcast_message_players(string message_to_send){
    connections_players_mutex.lock();
    for (int i = 0; i < connections_players.size(); i++) {
        // 6. Escribimos datos de vuelta al cliente usando send
        PlayerConnection player_i = connections_players[i];
        //cout << "Redirigiendo a " << player_i.player_fd << endl;

        const char* message = message_to_send.c_str();
        ssize_t bytes_sent = send(player_i.player_fd, message, strlen(message), 0);
        if (bytes_sent < 0) {
            cerr << "Error al enviar datos al socket" << endl;
            exit(1);
        }
    }
    connections_players_mutex.unlock();
}

int main() {
    // runner

    cout << "=== JUEGO DEL AHORCADO: Servidor ===" << endl;
    cout << "Temática: Sistemas Operativos, Redes, IPC, Sincronización\n" << endl;

    thread connection_handler_thread(connection_handler);

    while (1) {
        switch (game.getGameState())
        {
        case GameState::WAITING_FOR_PLAYERS:
            unique_lock<mutex> lock_hangman(hangman_mutex);

            // condition variable para esperar la accion del admin
            cv_action.wait(lock_hangman, []{ return game.isGameStarted(); });

            break;
        }
    }
    

    std::this_thread::sleep_for(chrono::seconds(10));

    hangman_mutex.lock();
    bool game_start_successful = game.startGame();
    hangman_mutex.unlock();

    broadcast_message("HOLA A TODOS LOS CONECTADOS!");

    if (!game_start_successful) {
        cout << "La partida no puede comenzar porque hay menos de 2 jugadores." << endl;
    } else {
        cout << "\nNuestros jugadores:" << endl;
        for (const string& player : game.getPlayerNames()) {
            cout << player << " ";
        }
        cout << "\n" << endl;
        broadcast_message_players("¡¡¡¡A jugar!!!!");
    }

    connection_handler_thread.join();
    return 0;
}
