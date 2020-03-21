#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define ERROR_WRONG_ARGC 7
#define DEFAULT_PORT 45805
#define RS_DASH_PORT 65301
#define RS_DASH_IP "127.0.0.1"
#define DATA_BUFFER_SIZE 1000

SOCKET createConnection(char* ip_addr, int port) {
    int rc;
    // Init connection to Server
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    rc = getaddrinfo(ip_addr, port, &hints, &result);
    if (rc != 0) {
        printf("getaddrinfo failed: %d\n", rc);
        return INVALID_SOCKET;
    }

    // Create Socket to server
    SOCKET ConnectSocket = INVALID_SOCKET;

    // Attempt to connect to the first address returned by
    // the call to getaddrinfo
    ptr=result;

    // Create a SOCKET for connecting to server
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        return INVALID_SOCKET;
    }

    // Connect to server.
    rc = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (rc == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }

    freeaddrinfo(result);
    result = NULL;
    ptr = NULL;

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        return INVALID_SOCKET;
    }

    return ConnectSocket;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf("Wrong number of arguments!\n");
        return ERROR_WRONG_ARGC;
    }

    int rc;

    // Initialize Winsock
    rc = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (rc != 0) {
        printf("WSAStartup failed: %d\n", rc);
        return 1;
    }

    // Create server connection
    SOCKET server = INVALID_SOCKET;
    server = createConnection(argv[1], DEFAULT_PORT);
    if (server == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    // Create RS Dash Connection
    SOCKET rsDash = INVALID_SOCKET;
	if((rsDash = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d\n"; , WSAGetLastError());
        closesocket(server);
        WSACleanup();
        return 1;
	}

    // Set RS Dash Connection info
    struct sockaddr_in rsDashAddr;
	ZeroMemory( &rsDashAddr, sizeof(rsDashAddr) );
	rsDashAddr.sin_family = AF_INET;
	rsDashAddr.sin_port = htons(RS_DASH_PORT);
	rsDashAddr.sin_addr.S_un.S_addr = inet_addr(RS_DASH_IP);


    // Close server connection to send. We don't need to send info at the moment
    rc = shutdown(server, SD_SEND);
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

    // Receive data from server and then send it to rs dash
    do {
        int rcSend;
        rc = recv(server, recvbuff, recvBuffLen, 0);
        if (rc > 0) {
            rcSend = sendto(rsDash, recvbuff, rc , 0 ,
                            (struct sockaddr *) &rsDashAddr, sizeof(rsDashAddr));
            if (rcSend == SOCKET_ERROR) {
                printf("Failed to send data to RS Dash\n");
            }
        } else if (rc == 0) {
            printf("Connection closed\n");
        } else {
            printf("recv failed: %d\n", WSAGetLastError());
        }
    } while (rc > 0);

    free(recvbuff);
exit:
    // cleanup
    closesocket(server);
    closesocket(rsDash);
    WSACleanup();

    return 0;
}