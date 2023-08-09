#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdio.h>

#define DRINN_DEBUG 1

#if DRINN_DEBUG
    #define DRINN_PRINT_DEBUG(...)\
    do {\
        printf(__VA_ARGS__);\
    } while (0);
#else
    #define DRINN_PRINT_DEBUG(...) (void)0
#endif

#define DRINN_PRINT_ERROR(...)\
do {\
    fprintf(stderr, __VA_ARGS__);\
} while (0);

#define DRINN_PRINT(...)\
do {\
    printf(__VA_ARGS__);\
} while (0);


typedef enum {
    DRINN_SUCCESS = 0,
    DRINN_RECV_ERROR = 1,
    DRINN_SEND_ERROR = 2,
    DRINN_SHUTDOWN_ERROR = 3,
    DRINN_ACCEPT_ERROR = 4,
    DRINN_SOCKET_ERROR = 5,
    DRINN_LISTEN_ERROR = 6,
    DRINN_GETADDRINFO_ERROR = 7,
    DRINN_BIND_ERROR = 8,
    DRINN_CONNECT_ERROR = 9,
    DRINN_STARTUP_ERROR = 10
} drinn_err;

#define TRUE 1
#define FALSE 0
typedef char bool;

#define DRINN_RECV_BUFF_LEN 512
#define DRINN_SEND_BUFF_LEN 256

#define DRINN_STX '#'
#define DRINN_ETX '~'

#define DRINN_DEFAULT_PORT "27333"

#define ARR_LENGTH(arr) (sizeof((arr)) / sizeof((arr)[0]))

drinn_err terminate_socket(SOCKET socket);

int main(int argc, char *argv[]) {
    char *server_addr = "localhost\0";
    if (argc > 1) {
        server_addr = argv[1];
    }

    bool receive_error = FALSE;
    bool receiving = TRUE;

    int send_result;
    int result;

    char receive_buffer[DRINN_RECV_BUFF_LEN+1];
    memset(receive_buffer, 0, ARR_LENGTH(receive_buffer));

    char receive_msg[DRINN_RECV_BUFF_LEN * 4 + 1];
    memset(receive_msg, 0, ARR_LENGTH(receive_msg));

    char send_buffer[DRINN_SEND_BUFF_LEN + 1] = "#DRINN~\0";

    SOCKET connect_socket = INVALID_SOCKET;

    WSADATA wsa_data;

    result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (result) {
        DRINN_PRINT_ERROR("[ERROR] WSAStartup failed: %d\n", result);
        return DRINN_STARTUP_ERROR;
    }

    DRINN_PRINT_DEBUG("WSAStartup successful!\n");

    struct addrinfo *result_info = NULL;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    result = getaddrinfo(server_addr, DRINN_DEFAULT_PORT, &hints, &result_info);
    if (result) {
        DRINN_PRINT_ERROR("[ERROR] getaddrinfo failed: %d\n", result);
        WSACleanup();
        return DRINN_GETADDRINFO_ERROR;
    }

    DRINN_PRINT_DEBUG("Getaddrinfo successful!\n");

    for(struct addrinfo *addr = result_info;
        addr != NULL;
        addr = addr->ai_next) {

        connect_socket = socket(result_info->ai_family,
                                result_info->ai_socktype,
                                result_info->ai_protocol);

        if (connect_socket != INVALID_SOCKET) {
            DRINN_PRINT_DEBUG("Socket successful!\n");

            result = connect(connect_socket,
                             result_info->ai_addr,
                             (int)result_info->ai_addrlen);

            if (result != SOCKET_ERROR) {
                break;
            }

            DRINN_PRINT_ERROR("[ERROR] connect failed: %d\n",
                              WSAGetLastError());

            closesocket(connect_socket);
            connect_socket = INVALID_SOCKET;

        } else {
            DRINN_PRINT_ERROR("[ERROR] socket failed: %d\n", WSAGetLastError());
            freeaddrinfo(result_info);
            WSACleanup();
            return DRINN_SOCKET_ERROR;
        }
    }

    if (connect_socket == INVALID_SOCKET) {
        DRINN_PRINT_ERROR("[ERROR] Could not connect to the server\n");
        WSACleanup();
        return DRINN_CONNECT_ERROR;
    }

    DRINN_PRINT("Successfully connected to the server.\n");

    send_result = send(connect_socket, send_buffer, strlen(send_buffer), 0);
    if (send_result == SOCKET_ERROR) {
        DRINN_PRINT_ERROR("[ERROR] send failed: %d\n", WSAGetLastError());
        closesocket(connect_socket);
        WSACleanup();
        return DRINN_SEND_ERROR;
    }

    DRINN_PRINT_DEBUG("Bytes sent: %s\n", send_buffer);

    // Closing the send socket, can still receive
    result = shutdown(connect_socket, SD_SEND);
    if (result == SOCKET_ERROR) {
        DRINN_PRINT_ERROR("[ERROR] shutdown failed: %d\n", WSAGetLastError());
        closesocket(connect_socket);
        WSACleanup();
        return DRINN_SHUTDOWN_ERROR;
    }

    // Receiving data
    while (receiving) {
        result = recv(connect_socket, receive_buffer,
                      ARR_LENGTH(receive_buffer)-1, 0);

        if (result > 0) {
            receive_buffer[result] = '\0';
            DRINN_PRINT_DEBUG("Number of bytes received: %d\n", result);
            DRINN_PRINT_DEBUG("Message: %s\n", receive_buffer);

            // Invalid start of message format, close connection
            if (strlen(receive_msg) == 0 &&
                receive_buffer[0] != DRINN_STX) {

                receive_error = TRUE;
                receiving = FALSE;
                DRINN_PRINT_ERROR(
                    "[SERVER_ERROR] Invalid STX (should be %c, found %c). Closing connection.\n",
                    DRINN_STX, receive_buffer[0]);
            } else { // STX was found

                char *etx = strchr(receive_buffer, DRINN_ETX);

                if (etx) { // ETX was found
                    receiving = FALSE;

                    int etx_index = (int)(etx - receive_buffer);

                    if (strlen(receive_msg) + etx_index <
                            ARR_LENGTH(receive_msg)-1) {
                        strncat(receive_msg, receive_buffer, etx_index);
                    } else {
                        strncat(receive_msg, receive_buffer,
                                ARR_LENGTH(receive_msg)-strlen(receive_msg)-1);
                        receive_error = TRUE;
                    }

                } else { // ETX was not found, either not finished or error

                    if (strlen(receive_msg) + result <
                            ARR_LENGTH(receive_msg)-1) {
                        strncat(receive_msg, receive_buffer, result);
                    } else {
                        strncat(receive_msg, receive_buffer,
                                ARR_LENGTH(receive_msg)-strlen(receive_msg)-1);
                        receiving = FALSE;
                        receive_error = TRUE;
                        DRINN_PRINT_ERROR(
                            "[SERVER_ERROR] Max msg length reached. No ETX was found. Closing connection.\n");
                    }
                }
            }
        } else if (result == 0) {
            receiving = FALSE;
            DRINN_PRINT_ERROR("[SERVER_ERROR] The server closed the connection.\n");
            return terminate_socket(connect_socket);
        } else {
            DRINN_PRINT_ERROR("[ERROR] recv failed: %d\n", WSAGetLastError());
            closesocket(connect_socket);
            return DRINN_RECV_ERROR;
        }
    }

    // Closing the connection with this client
    return terminate_socket(connect_socket);
}

drinn_err terminate_socket(SOCKET socket) {
    if (shutdown(socket, SD_BOTH) == SOCKET_ERROR) {
        DRINN_PRINT_ERROR("[ERROR] shutdown failed: %d\n", WSAGetLastError());
        closesocket(socket);
        return DRINN_SHUTDOWN_ERROR;
    }
    closesocket(socket);
    DRINN_PRINT("The connection was closed normally.\n");
    return DRINN_SUCCESS;
}
