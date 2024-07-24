#include <iostream>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")

namespace fs =  filesystem;

const int port = 8080;
const int  BufferSize = 1024; 

void handle_client(SOCKET ClientSocket, const  string& Base) {
    char  Buffer[ BufferSize];
    int BytesReceived = recv(ClientSocket,  Buffer,  BufferSize - 1, 0);
    if (BytesReceived < 0) 
    {
         cerr << "Error receiving data" <<  endl;
        closesocket(ClientSocket);
        return;
    }
     Buffer[BytesReceived] = '\0';
     istringstream request_stream( Buffer);
     string method, path, version;
    request_stream >> method >> path >> version;
    if (method != "GET") {
         cerr << "App only support Get Method." <<  endl;
        closesocket(ClientSocket);
        return;
    }
     string FilePath = Base + path;
    if (FilePath.back() == '/') {
        FilePath += "index.html";
    }

     ifstream file(FilePath);
    if (!file) {
         string not_found = "HTTP/1.1 404 not found\r\nContent-Length: 13\r\n\r\n404 not found";
        send(ClientSocket, not_found.c_str(), not_found.length(), 0);
        closesocket(ClientSocket);
        return;
    }

     stringstream file_content;
    file_content << file.rdbuf();
     string content = file_content.str();

     string response = "HTTP/1.1 200 OK\r\nContent-Length: " +  to_string(content.length()) + "\r\n\r\n" + content;
    send(ClientSocket, response.c_str(), response.length(), 0);

    closesocket(ClientSocket);
}

void start_server(const  string& Base) {
    WSADATA wsaData;
    int StartupWAS = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (StartupWAS != 0) {
         cerr << "Error Winsock initializing." <<  endl;
        return;
    }

    SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocket == INVALID_SOCKET) {
         cerr << "Error during socket creation." <<  endl;
        WSACleanup();
        return;
    }

    sockaddr_in ServerAddress;
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons(port);
    ServerAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(ServerSocket, (sockaddr*)&ServerAddress, sizeof(ServerAddress)) == SOCKET_ERROR) {
         cerr << "Error  during socket binding" <<  endl;
        closesocket(ServerSocket);
        WSACleanup();
        return;
    }

    if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR) {
         cerr << "Error during  socket listening." <<  endl;
        closesocket(ServerSocket);
        WSACleanup();
        return;
    }

     cout << "Server start on port: " << port <<  endl;

    while (true) {
        sockaddr_in client_addr;
        int client_size = sizeof(client_addr);
        SOCKET ClientSocket = accept(ServerSocket, (sockaddr*)&client_addr, &client_size);

        if (ClientSocket == INVALID_SOCKET) {
             cerr << "Error while connection accepting." <<  endl;
            continue;
        }

         thread(handle_client, ClientSocket, Base).detach();
    }

    closesocket(ServerSocket);
    WSACleanup();
}

int main() {
     string Base = "./www";
    if (!fs::exists(Base) || !fs::is_directory(Base)) {
         cerr << "Base directory does not exist \nor \nBase directory is not a directory" <<  endl;
        return 1;
    }

    start_server(Base);
    return 0;
}
