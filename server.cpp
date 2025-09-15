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

// condition variables
condition_variable cv_action;

// juego
Hangman game;
mutex hangman_mutex;

// flag de acción de partida
bool actioned = false;
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

    cout << "\nSe cerró una conexión de cliente." << endl;
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
        cout << "\nSe unió " << player_name << endl;

        const char* message_c = "\n¡Te damos la bienvenida al servidor!";
        ssize_t bytes_sent = send(client_fd, message_c, strlen(message_c), 0);
        if (bytes_sent < 0) {
            cerr << "Error al enviar datos al socket" << endl;
            exit(1);
        }

        // si no hay admin de la sala, se establece este jugador como admin
        if (admin_connection == 0) {
            admin_connection = client_fd;
            cout << "\nNuestro administrador es " << player_name << endl;

            const char* message_b = "\n----- SOS ADMIN -----\nAdministrás el servidor. Escribí /jugar para comenzar la partida, deben haber 2 o más jugadores. Escribí /lobby al terminar la partida para volver a la espera de jugadores nuevos.";
            ssize_t bytes_sent = send(client_fd, message_b, strlen(message_b), 0);
            if (bytes_sent < 0) {
                cerr << "Error al enviar datos al socket" << endl;
                exit(1);
            }
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

        // cerrar conexión
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

        // comenzar partida, solo admin
        if (message_received == "/jugar" && admin_connection == client_fd && !game.isGameStarted()) {
            {
                unique_lock<mutex> lock_hangman(hangman_mutex);
                game.startGame();
            }
            cv_action.notify_all();

            continue;
        }

        // volver al lobby si finalizó la partida, solo admin
        if (message_received == "/lobby" && admin_connection == client_fd && game.isGameFinished()) {
            {
                unique_lock<mutex> lock_hangman(hangman_mutex);
                game.resetToWaiting();
            }
            cv_action.notify_all();

            continue;
        }

        // adivinar una letra si es tu turno
        if (message_received == "/letra" && game.isPlayerTurn(player_name)) {
            const char* message = "\nIntroducí una letra:";
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

            const string letra = string(buffer, bytes_received);
            if (letra.length() != 1) {
                const char* message_b = "\nPara adivinar una letra, introduzca un solo caracter. Escriba el comando /letra para intentar de nuevo.";
                ssize_t bytes_sent = send(client_fd, message_b, strlen(message_b), 0);
                if (bytes_sent < 0) {
                    cerr << "Error al enviar datos al socket" << endl;
                    exit(1);
                }
                continue;
            }

            {
                unique_lock<mutex> lock_hangman(hangman_mutex);
                unique_lock<mutex> lock_action(cv_action_mutex);

                char char_letra = letra.c_str()[0];

                actioned = game.guessLetter(player_name, char_letra);
            }
            cv_action.notify_all();
        } 

        // adivinar una palabra si es tu turno
        if (message_received == "/palabra" && game.isPlayerTurn(player_name)) {
            const char* message = "\nIntroducí una palabra:";
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

            const string palabra = string(buffer, bytes_received);

            {
                unique_lock<mutex> lock_hangman(hangman_mutex);
                unique_lock<mutex> lock_action(cv_action_mutex);
                actioned = game.guessWord(player_name, palabra);
            }
            cv_action.notify_all();
        } 

        cout << "\n* Mensaje de " << player_name <<": \n" << message_received << endl;
    }
}

// encontrar el fd de la conexión de un jugador
int findPlayerFDByName(string player_name) {
    int fd = -1;

    {
        unique_lock<mutex> lock_connections_players(connections_players_mutex);
        auto it = std::find_if(connections_players.begin(), connections_players.end(),
            [player_name](const PlayerConnection& p) { return p.name == player_name;}
        );

        if (it != connections_players.end()) {
            fd = it->player_fd;
        }
    }

    return fd;
}

// Ejemplo con UNIX sockets
void connectionHandler(){
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
    }

    close(server_fd);
}

// enviar un mensaje a todas las conexiones
void broadcastMessage(string message_to_send){
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
void broadcastMessageToPlayers(string message_to_send){
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

    thread connection_handler_thread(connectionHandler);

    while (1) {
        // no uso switch case por problemas con la condition variable

        GameState current_state;
        {
            unique_lock<mutex> lock_hangman(hangman_mutex);
            current_state = game.getGameState();
        }

        // --------- PRE PARTIDA ---------
        if (current_state == GameState::WAITING_FOR_PLAYERS)
        {
            cout << "\nEsperando nuevos jugadores...\nNota: cada jugador debe ingresar su nombre para jugar.\n" << endl;
            broadcastMessageToPlayers("Esperando nuevos jugadores...");

            {
                unique_lock<mutex> lock_hangman(hangman_mutex);

                // condition variable para esperar la accion del admin
                cv_action.wait(lock_hangman, []{ return game.isGameStarted(); });

                if (!game.isGameStarted()) {
                    cout << "La partida no puede comenzar porque hay menos de 2 jugadores." << endl;
                    continue;
                }
                
                cout << "\nNuestros jugadores:" << endl;
                for (const string& player : game.getPlayerNames()) {
                    cout << player << " ";
                }
                cout << "\n" << endl;
            }

            broadcastMessage("¡La partida está comenzando!");
            this_thread::sleep_for(chrono::seconds(5));
        }

        // --------- PARTIDA ---------

        if (current_state == GameState::IN_PROGRESS)
        {
            {
                unique_lock<mutex> lock_hangman(hangman_mutex);

                const string estado_partida = "\n------------\nQuedan " + to_string(game.getRemainingGuesses()) + " intentos.\n\nPalabra: " + game.getWordDisplay() + "\n\nTurno de " + game.getCurrentPlayerName() + "\n------------\n";

                broadcastMessageToPlayers(estado_partida);
                cout << estado_partida << endl;
                //cv_action.wait(lock_hangman, []{ return game.); });

                string current_turn = game.getCurrentPlayerName();
                int current_turn_fd = findPlayerFDByName(current_turn);
                if (current_turn_fd == -1) {
                    cout << "Error, el jugador no está en la partida" << endl;
                    return 1;
                }

                const char* message = "\nTu turno! Escribe /letra o /palabra para adivinar una letra o palabra.";
                ssize_t bytes_sent = send(current_turn_fd, message, strlen(message), 0);
                if (bytes_sent < 0) {
                    cerr << "Error al enviar datos al socket" << endl;
                    return 1;
                }
            }

            {
                unique_lock<mutex> lock_actioned(cv_action_mutex);

                // condition variable para esperar la accion del jugador del turno
                cv_action.wait(lock_actioned, []{ return actioned == true; });

                // se resetea el flag de acción de partida
                actioned = false;
            }
        }

        // --------- POST PARTIDA (PARTIDA TERMINADA) ---------
        if (current_state == GameState::FINISHED)
        {
            unique_lock<mutex> lock_hangman(hangman_mutex);

            const string end_msg = "\n----- FIN DE LA PARTIDA -----\n\nLa palabra era " + game.getCurrentWord();
            broadcastMessageToPlayers(end_msg);
            cout << end_msg << endl;

            if (game.getWinner().size() > 0) {
                const string winner_msg = "\n¡El ganador es " + game.getWinner() + " y ganó un punto!";
                broadcastMessageToPlayers(winner_msg);
            } else {
                broadcastMessageToPlayers("\n¡No se adivinó la palabra!");
            }

            string point_msg = "\n----- PUNTAJE -----";
            for (int i = 0; i < game.getPlayerCount(); i++) {
                point_msg = point_msg + "\n" + game.getPlayersWithPoints()[i].first + " tiene " + to_string(game.getPlayersWithPoints()[i].second) + " puntos.";
            }
            cout << point_msg << endl;
            broadcastMessageToPlayers(point_msg);

            // condition variable para esperar la accion del admin
            cv_action.wait(lock_hangman, []{
                return game.getGameState() == GameState::WAITING_FOR_PLAYERS ||
                game.isGameStarted();
            });
        }
    }

    connection_handler_thread.join();
    return 0;
}
