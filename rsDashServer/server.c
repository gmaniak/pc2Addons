#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define ERROR_WRONG_ARGC 7
#define DEFAULT_PORT 45805
#define RS_DASH_PORT 65301
#define DATA_BUFFER_SIZE 1000

SOCKET createServerSocket(int port) {
    int rc;

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    rc = getaddrinfo(NULL, port, &hints, &result);
    if (rc != 0) {
        printf("getaddrinfo failed: %d\n", rc);
        return INVALID_SOCKET;
    }

    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        return INVALID_SOCKET;
    }

    // Setup the TCP listening socket
    rc = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (rc == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        return INVALID_SOCKET;
    }

    freeaddrinfo(result);
    result = NULL;
    ptr = NULL;

    if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
        printf( "Listen failed with error: %ld\n", WSAGetLastError() );
        closesocket(ListenSocket);
        return INVALID_SOCKET;
    }

    return ListenSocket;
}

int main(void) {
    int rc;

    // Initialize Winsock
    rc = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (rc != 0) {
        printf("WSAStartup failed: %d\n", rc);
        return 1;
    }

    // Create server connection
    SOCKET listenSocket = INVALID_SOCKET;
    listenSocket = createServerSocket(DEFAULT_PORT);
    if (listenSocket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    // Accept connection from client connection
    clientSocket = INVALID_SOCKET;

    // Accept a client socket
    clientSocket = accept(ListenSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Create RS Transmitter Connection
    SOCKET rsTransmitter = INVALID_SOCKET;
    if((rsTransmitter = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d\n"; , WSAGetLastError());
        closesocket(listenSocket);
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // RS Transmitter Addr
    struct sockaddr_in rsTransmitterAddr;
    ZeroMemory(&rsTransmitterAddr, sizeof (rsTransmitterAddr));
    rsTransmitterAddr.sin_family = AF_INET;
    rsTransmitterAddr.sin_addr.s_addr = INADDR_ANY;
    rsTransmitterAddr.sin_port = htons( RS_DASH_PORT );

    //Bind
    if(bind(rsTransmitter, (struct sockaddr *)rsTransmitterAddr, sizeof(rsTransmitterAddr)) ==
       SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d\n;", WSAGetLastError());
        goto exit;
    }

    // Close receiving side of client socket. Not expecting to receive something.
    rc = shutdown(clientSocket, SD_RECV);
    if (rc == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        goto exit;
    }

    char *recvbuff = malloc(DATA_BUFFER_SIZE * sizeof(char));
    if (!recvbuff) {
        printf("Failed to allocate buffer\n");
        goto exit;
    }
    int recvBuffLen = DATA_BUFFER_SIZE;

    // Receive data from RS Transmitter and then send it to out client
    do {
        struct sockaddr_in rsClient;
        rc = recvfrom(rsTransmitter, recvbuff, recvBuffLen, 0,
                      (struct sockaddr *) &rsClient, sizeof(rsClient()))
        if (rc == SOCKET_ERROR) {
            printf("Failed to receive data!\n");
            break;
        }

        // Send data
        rc = send(clientSocket, recvbuff, rc, 0);
        if (rc == SOCKET_ERROR) {
            printf("send failed: %d\n", WSAGetLastError());
            break;
            return 1;
        }

    } while (true)

    free(recvbuff);
exit:
    // cleanup
    closesocket(listenSocket);
    closesocket(clientSocket);
    closesocket(rsTransmitter);
    WSACleanup();

    return 0;
}