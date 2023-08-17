#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdio.h>

#define MINIAUDIO_IMPLEMENTATION
#include "include/miniaudio.h"

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

#define DRINN_DEFAULT_VOLUME 1.0f

#define ARR_LENGTH(arr) (sizeof((arr)) / sizeof((arr)[0]))

drinn_err manage_connection(SOCKET client_socket);

ma_engine drinn_sound_engine;
ma_sound drinn_sound;

int main(int argc, char *argv[]) {

    ma_result ma_ret;
    ma_ret = ma_engine_init(NULL, &drinn_sound_engine);
    if (ma_ret != MA_SUCCESS) {
        DRINN_PRINT_ERROR("[ERROR] Could not initialize the audio engine\n");
        return DRINN_STARTUP_ERROR;
    }

    ma_ret = ma_sound_init_from_file(&drinn_sound_engine,
                                     "drinn.mp3",
                                     MA_SOUND_FLAG_DECODE, NULL, NULL,
                                     &drinn_sound);
    if (ma_ret != MA_SUCCESS) {
        DRINN_PRINT_ERROR("[ERROR] Could not initialize the drinn sound\n");
        return DRINN_STARTUP_ERROR;
    }

    float volume = DRINN_DEFAULT_VOLUME;
    if (argc > 1) {
        DRINN_PRINT_DEBUG("Trying to set volume of %s\n", argv[1]);
        char *junk;
        volume = strtof(argv[1], &junk);
        if (*junk != '\0') {
            volume = DRINN_DEFAULT_VOLUME;
        }
    }
    DRINN_PRINT("Set volume of %f\n", volume);
    ma_sound_set_volume(&drinn_sound, volume);



    bool running = TRUE;
    int result;

    SOCKET listen_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;

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
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, DRINN_DEFAULT_PORT, &hints, &result_info);
    if (result) {
        DRINN_PRINT_ERROR("[ERROR] getaddrinfo failed: %d\n", result);
        WSACleanup();
        return DRINN_GETADDRINFO_ERROR;
    }

    DRINN_PRINT_DEBUG("Getaddrinfo successful!\n");

    listen_socket = socket(result_info->ai_family,
                           result_info->ai_socktype,
                           result_info->ai_protocol);

    if (listen_socket == INVALID_SOCKET) {
        DRINN_PRINT_ERROR("[ERROR] socket failed: %d\n", WSAGetLastError());
        freeaddrinfo(result_info);
        WSACleanup();
        return DRINN_SOCKET_ERROR;
    }

    DRINN_PRINT_DEBUG("Socket successful!\n");

    result = bind(listen_socket, result_info->ai_addr,
                  (int)result_info->ai_addrlen);
    freeaddrinfo(result_info); // free wether bind was successful or not
    if (result) {
        DRINN_PRINT_ERROR("[ERROR] bind failed: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return DRINN_BIND_ERROR;
    }

    DRINN_PRINT_DEBUG("Bind successful!\n");

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        DRINN_PRINT_ERROR("[ERROR] listen failed: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return DRINN_LISTEN_ERROR;
    }

    DRINN_PRINT_DEBUG("Listen successful!\n");

    DRINN_PRINT("Socket successfully opened!\n");

    while (running) {
        DRINN_PRINT("Waiting to accept a connection...\n");
        client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            DRINN_PRINT_ERROR("[ERROR] accept failed: %d\n", WSAGetLastError());
            closesocket(listen_socket);
            WSACleanup();
            return 1;
        }

        DRINN_PRINT("Accepted a connection!\n");

        drinn_err conn_err = manage_connection(client_socket);
        DRINN_PRINT_DEBUG("manage_connection returned: %d\n", conn_err);
    }

    closesocket(listen_socket);
    WSACleanup();

    DRINN_PRINT("Finished!\n");
    return DRINN_SUCCESS;
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

drinn_err manage_connection(SOCKET client_socket) {
    bool receive_error = FALSE;
    bool receiving = TRUE;

    int send_result;
    int result;

    char receive_buffer[DRINN_RECV_BUFF_LEN+1];
    memset(receive_buffer, 0, ARR_LENGTH(receive_buffer));

    char receive_msg[DRINN_RECV_BUFF_LEN * 4 + 1];
    memset(receive_msg, 0, ARR_LENGTH(receive_msg));

    char send_buffer[DRINN_SEND_BUFF_LEN + 1] = "#DRINN~\0";

    while (receiving) {
        result = recv(client_socket, receive_buffer,
                      ARR_LENGTH(receive_buffer)-1, 0);

        if (result > 0) {
            receive_buffer[result] = '\0';
            DRINN_PRINT_DEBUG("Number of bytes received: %d\n", result);
            DRINN_PRINT_DEBUG("Bytes: %s\n", receive_buffer);

            // Invalid start of message format, close connection
            if (strlen(receive_msg) == 0 &&
                receive_buffer[0] != DRINN_STX) {

                receive_error = TRUE;
                receiving = FALSE;
                DRINN_PRINT_ERROR(
                    "[CLIENT_ERROR] Invalid STX (should be %c, found %c). Closing connection.\n",
                    DRINN_STX, receive_buffer[0]);
                strncpy(send_buffer, "#ERR:INVALID_STX~\0",
                        ARR_LENGTH(send_buffer));

            } else { // STX was found

                char *etx = strchr(receive_buffer, DRINN_ETX);

                if (etx) { // ETX was found
                    receiving = FALSE;

                    int etx_index = (int)(etx - receive_buffer)+1;

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
                            "[CLIENT_ERROR] Max msg length reached. No ETX was found. Closing connection.\n");
                        strncpy(send_buffer, "#ERR:MAX_MSG_LENGTH~\0",
                                ARR_LENGTH(send_buffer));
                    }
                }
            }
        } else if (result == 0) {
            receiving = FALSE;
            receive_error = TRUE;
            DRINN_PRINT_ERROR("[CLIENT_ERROR] The client closed the connection.\n");
            strncpy(send_buffer, "#ERR:NO_ETX_FOUND~\0",
                    ARR_LENGTH(send_buffer));
        } else {
            DRINN_PRINT_ERROR("[ERROR] recv failed: %d\n", WSAGetLastError());
            closesocket(client_socket);
            return DRINN_RECV_ERROR;
        }
    }

    if (receive_error == FALSE) {
        DRINN_PRINT_DEBUG("Message: %s\n", receive_msg);
        if (strcmp(receive_msg, "#DRINN~") == 0) {
            DRINN_PRINT("DRINN\n");
            ma_sound_seek_to_pcm_frame(&drinn_sound, 0);
            ma_sound_start(&drinn_sound);
        }
    }

    // Closing the receive socket, can still send
    result = shutdown(client_socket, SD_RECEIVE);
    if (result == SOCKET_ERROR) {
        DRINN_PRINT_ERROR("[ERROR] shutdown failed: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return DRINN_SHUTDOWN_ERROR;
    }

    send_result = send(client_socket, send_buffer, strlen(send_buffer), 0);
    if (send_result == SOCKET_ERROR) {
        DRINN_PRINT_ERROR("[ERROR] send failed: %d\n", WSAGetLastError());
        closesocket(client_socket);
        return DRINN_SEND_ERROR;
    }

    DRINN_PRINT_DEBUG("Bytes sent: %s\n", send_buffer);

    // Closing the connection with this client
    return terminate_socket(client_socket);
}
