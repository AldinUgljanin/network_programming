// Header files for both Windows and Unix
#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

// Make code compatible for both Windows and Unix
#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif

// Some universal headers
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(){
    // initilize Winsock
#if defined(_WIN32)
    WSADATA d;
    if(WSAStartup (MAKEWORD(2, 2), &d)){
	fprintf(stderr, "Failed to initialize.\n");
	return 1;
    }
#endif

    // Figure out on what local address out web server should bind
    puts("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    // SOCK_STREAM is TCP and SOCK_DGRAM is UDP
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);

    // Creating Socket
    puts("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
	    bind_address->ai_socktype, bind_address->ai_protocol);

    // Checking if the call to socket was successful
    if (!ISVALIDSOCKET(socket_listen)){
	fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
	return 1;
    }
    
    puts("Binding socket to local address..");
    if(bind(socket_listen,
		bind_address->ai_addr, bind_address->ai_addrlen)){
	fprintf(stderr,"bind() failed. (%d)\n", GETSOCKETERRNO());
	return 1;
    }
    freeaddrinfo(bind_address);

    // Listening for conections
    puts("Listening...");
    if(listen(socket_listen, 10) < 0){
	fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
	return 1;
    }

    // Accepting concections
    puts("Waiting for connection...");
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client = accept(socket_listen,
	    (struct sockaddr*) &client_address, &client_len);
    if(!ISVALIDSOCKET(socket_client)){
	fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
	return 1;
    }

    // Printing clients addres 
    puts("Client is connected...");
    char address_buffer[100];
    getnameinfo((struct sockaddr*)&client_address,
	    client_len, address_buffer, sizeof(address_buffer), 0, 0,
	    NI_NUMERICHOST);
    printf("%s\n", address_buffer);

    // Reciving requests
    puts("Reading request...");
    char request[1024];
    int bytes_received = recv(socket_client, request, 1024, 0);
    printf("Recived %d bytes.\n", bytes_received);
    
    // Response to request
    puts("Sending response...");
    const char *response =
	"HTTP/1.1 200 OK\r\n"
	"Connection: close\r\n"
	"Content-Type: text/plain\r\n\r\n"
	"Local time is: ";
    int bytes_sent = send(socket_client, response, strlen(response), 0);
    printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

    // Sending Time too the client
    time_t timer;
    time(&timer);
    char *time_msg = ctime(&timer);
    bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
    printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));

    // Closing connection
    puts("Closing connection...");
    CLOSESOCKET(socket_client);

    puts("Closing listening socket");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif

    puts("Finished.");

    return 0;
}
