#pragma once 
#include <iostream>
#include <winsock2.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib") // Linka la libreria Winsock

// Funzione per inizializzare Winsock
bool initializeWinsock(WSADATA& wsaData) {
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }
    return true;
}

// Funzione per creare un socket
SOCKET createSocket() {
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
    }
    return server_fd;
}

// Funzione per associare il socket a un indirizzo e una porta
bool bindSocket(SOCKET server_fd, struct sockaddr_in& address) {
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        return false;
    }
    return true;
}

// Funzione per mettere il server in ascolto
bool startListening(SOCKET server_fd) {
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        return false;
    }
    return true;
}

// Funzione per accettare una connessione in entrata
SOCKET acceptConnection(SOCKET server_fd, struct sockaddr_in& address, int& addr_len) {
    SOCKET new_socket = accept(server_fd, (struct sockaddr*)&address, &addr_len);
    if (new_socket == INVALID_SOCKET) {
        std::cerr << "Accept failed" << std::endl;
    }
    return new_socket;
}

// Funzione per ricevere il messaggio dal client
int receiveMessage(SOCKET new_socket, char* buffer, int buffer_size) {
    return recv(new_socket, buffer, buffer_size, 0);
}

// Funzione per inviare una risposta al client
void sendResponse(SOCKET new_socket, const char* response) {
    send(new_socket, response, strlen(response), 0);
    std::cout << "Response sent to client" << std::endl;
}

// Funzione per chiudere i socket
void closeSockets(SOCKET server_fd, SOCKET new_socket) {
    closesocket(new_socket);
    closesocket(server_fd);
}

// Funzione per terminare Winsock
void cleanupWinsock() {
    WSACleanup();
}

int main() {
    WSADATA wsaData;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addr_len = sizeof(address);
    char buffer[1024] = { 0 };

    // Inizializza Winsock
    if (!initializeWinsock(wsaData)) {
        return -1;
    }

    // Crea il socket
    server_fd = createSocket();
    if (server_fd == INVALID_SOCKET) {
        cleanupWinsock();
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Ascolta su tutte le interfacce
    address.sin_port = htons(8080);

    // Associa il socket a un indirizzo e porta
    if (!bindSocket(server_fd, address)) {
        closeSockets(server_fd, INVALID_SOCKET);
        cleanupWinsock();
        return -1;
    }

    // Inizia ad ascoltare
    if (!startListening(server_fd)) {
        closeSockets(server_fd, INVALID_SOCKET);
        cleanupWinsock();
        return -1;
    }

    std::cout << "Server listening on port 8080..." << std::endl;

    // Accetta una connessione
    new_socket = acceptConnection(server_fd, address, addr_len);
    if (new_socket == INVALID_SOCKET) {
        closeSockets(server_fd, INVALID_SOCKET);
        cleanupWinsock();
        return -1;
    }

    // Legge il messaggio inviato dal client
    int recv_size = receiveMessage(new_socket, buffer, sizeof(buffer));
    if (recv_size > 0) {
        std::cout << "Message from client: " << buffer << std::endl;

        // Invia una risposta al client
        const char* response = "Hello from server!";
        sendResponse(new_socket, response);
    }

    // Chiudi i socket
    closeSockets(server_fd, new_socket);

    // Termina Winsock
    cleanupWinsock();
    return 0;
}
