#include <iostream>
#include <sys/socket.h> // Para sockets en general
#include <sys/un.h> // Para los sockets UNIX 
#include <unistd.h>

using namespace std;

int MAX_CLIENTS = 5;
int MAX_MSG_SIZE = 1024;

// Ejemplo con UNIX sockets
int main(){
    
    // 1. Creamos el socket
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        cerr << "Error al crear el socket" << endl;
        return 1;
    }

    // 2. Vinculamos el socket a una dirección (en este caso, como un socket UNIX, usamos un archivo)
    sockaddr_un address;
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, "/tmp/socket-example");
    unlink(address.sun_path);  // Aseguramos que el socket no exista 

    // el :: es para salir del namespace std en esta llamada ya que existe un std::bind que no es el que queremos usar
    int bind_result = ::bind(server_fd, (sockaddr*)&address, sizeof(address)); 
    if (bind_result < 0) {
        cerr << "Error al vincular el socket" << endl;
        return 1;
    }

    // 3. Escuchamos conexiones entrantes
    int listen_result = listen(server_fd, MAX_CLIENTS); // 5 es el número máximo de conexiones en cola
    if (listen_result < 0) {
        cerr << "Error al escuchar en el socket" << endl;
        return 1;
    } else {
        cout << "Escuchando en el socket..." << endl;
    }

    while(1){
        // 4. Aceptamos conexiones entrantes
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            cerr << "Error al aceptar la conexión" << endl;
            return 1;
        }

        // 5. Leemos datos del cliente usando recv
        char buffer[MAX_MSG_SIZE];
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            cerr << "Error al recibir datos del socket" << endl;
            return 1;
        }

        cout << "Mensaje recibido del cliente: " << string(buffer, bytes_received) << endl;

        // 6. Escribimos datos de vuelta al cliente usando send
        ssize_t bytes_sent = send(client_fd, buffer, bytes_received, 0);
        if (bytes_sent < 0) {
            cerr << "Error al enviar datos al socket" << endl;
            return 1;
        }

        // 7. Cerramos los sockets
        close(client_fd);
    }
    close(server_fd);

    return 0;
}
