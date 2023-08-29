#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <fileapi.h>
#include <string.h>
#include <stdio.h>
#define MINIAUDIO_IMPLEMENTATION
#include "include/miniaudio.h"

/* THIS IS NOT EXACTLY WHAT'S PRINTED IN THE HELP MESSAGE

drinn:

    --help                      Print this message.

    --set-volume <volume>       Set volume. 
                                It has to be greater than 0 and less or equal to 1.
                                (default 0.8)

    --debug                     Activate debug output.

    --set-autostart             Set the program to autostart with the same flags.

*/ 

#define DRINN_CURRENT_VERSION "0.2"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
typedef char drinn_bool;

#define DRINN_PRINT_DEBUG(...)\
do {\
    if (debug_output) printf("[DEBUG] "__VA_ARGS__);\
} while (0);

#define DRINN_PRINT_ERROR(...)\
do {\
    fprintf(stderr, "[ERROR] "__VA_ARGS__);\
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
    DRINN_STARTUP_ERROR = 10,
    DRINN_INVALID_ARGS_ERROR = 11,
    DRINN_INSUFFICIENT_BUFFER_ERROR = 12,
    DRINN_INET_NTOP_ERROR = 13,
    DRINN_GET_ADAPTERS_ERROR = 14,
    DRINN_GENERIC_ERROR = 33,
} drinn_err;

#define DRINN_RECV_BUFF_LEN 512
#define DRINN_SEND_BUFF_LEN 256

#define DRINN_STX '#'
#define DRINN_ETX '~'

#define DRINN_DEFAULT_PORT "27333"
#define DRINN_DEFAULT_VOLUME 0.8f
#define DRINN_DEFAULT_AUDIO_FILE "drinn.mp3"

#define ARR_LENGTH(arr) (sizeof((arr)) / sizeof((arr)[0]))

drinn_err
manage_connection(SOCKET client_socket);
drinn_err
terminate_socket(SOCKET socket);
drinn_bool
file_exists(const char *path);
void
print_help_message();

ma_engine drinn_sound_engine;
ma_sound drinn_sound;

drinn_bool debug_output = FALSE;

int main(int argc, char *argv[]) {
    DRINN_PRINT("################################################\n");
    DRINN_PRINT("############  WELCOME TO DRINN v"DRINN_CURRENT_VERSION" ############\n");
    DRINN_PRINT("################################################\n\n");

    // ### ARGUMENTS PARSING
    float volume_arg = -1.f;
    drinn_bool set_autostart_arg = FALSE;
    drinn_bool print_adapters_info = TRUE;

    for(int arg_index = 1; arg_index < argc; ++arg_index) {
        if (strcmp("--help", argv[arg_index]) == 0) {
            print_help_message();
            return DRINN_SUCCESS;
        } else if (strcmp("--debug", argv[arg_index]) == 0) {
            debug_output = TRUE;
        } else if (strcmp("--set-autostart", argv[arg_index]) == 0) {
            set_autostart_arg = TRUE;
        } else if (strcmp("--no-adapters-info", argv[arg_index]) == 0) {
            print_adapters_info = FALSE;
        } else if (volume_arg == -1.f &&
                    strcmp("--set-volume", argv[arg_index]) == 0) {
            if (arg_index == argc-1) {
                DRINN_PRINT_ERROR("You must provide a number after --set-volume\n");
                print_help_message();
                return DRINN_INVALID_ARGS_ERROR;
            } else {
                char *junk;
                volume_arg = strtof(argv[arg_index+1], &junk);
                if (*junk != '\0' || !(0.f < volume_arg && volume_arg <= 1.f)) {
                    DRINN_PRINT_ERROR("Invalid volume value: %s\n", argv[arg_index+1]);
                    print_help_message();
                    return DRINN_INVALID_ARGS_ERROR;
                }
                arg_index++;
            }
        } else {
            DRINN_PRINT_ERROR("Unrecognised argument: %s\n", argv[arg_index]);
            print_help_message();
            return DRINN_INVALID_ARGS_ERROR;
        }
    }
    if (volume_arg == -1.f) volume_arg = DRINN_DEFAULT_VOLUME;
    // ### END OF ARGUMENTS PARSING

    // ### SETTING CORRECT WORKING DIRECTORY
    int work_dir_path_size = 8192;
    char *work_dir_path = (char *) calloc(work_dir_path_size, sizeof(char));
    int full_exe_path_size = 4096;
    char *full_exe_path = (char *) calloc(full_exe_path_size, sizeof(char));
    DWORD filename_result =
        GetModuleFileNameA(NULL, full_exe_path, full_exe_path_size-1);
    if (filename_result != 0) {
        char drive[64] = "";
        char dir[512] = "";
        _splitpath_s(full_exe_path,
                     drive, sizeof(drive),
                     dir, sizeof(dir),
                     NULL, 0,
                     NULL, 0);
        _makepath_s(work_dir_path, work_dir_path_size-1,
                    drive, dir, NULL, NULL);
        if (SetCurrentDirectory(work_dir_path) == 0) {
            DRINN_PRINT_ERROR("Could not current working directory: %ld\n", GetLastError());
            if (work_dir_path) free(work_dir_path);
            if (full_exe_path) free(full_exe_path);
            return DRINN_GENERIC_ERROR;
        }
        DRINN_PRINT_DEBUG("Successfully set current working directory: %s\n",
                          work_dir_path);
    } else {
        DRINN_PRINT_ERROR("Could not get full executable path: %ld\n", GetLastError());
        if (work_dir_path) free(work_dir_path);
        if (full_exe_path) free(full_exe_path);
        return DRINN_GENERIC_ERROR;
    }

    DRINN_PRINT_DEBUG("### INFO FROM ARGS ###\n");
    DRINN_PRINT_DEBUG("working_dir: %s\n", work_dir_path);
    DRINN_PRINT_DEBUG("debug: %s\n", (debug_output)?"true":"false");
    DRINN_PRINT_DEBUG("volume: %f\n", volume_arg);
    DRINN_PRINT_DEBUG("autostart: %s\n", (set_autostart_arg)?"true":"false");
    // ### END OF SETTING CORRECT WORKING DIRECTORY

    // ### SETTING AUTOSTART - REGISTRY KEY CREATION
    if (set_autostart_arg) {
        int full_command_size = 8192;
        char *full_command = (char *) calloc(full_command_size, sizeof(char));
        int concat_res =
            snprintf(full_command, full_command_size,
                    "%s --set-volume %f %s %s",
                    full_exe_path, volume_arg,
                    (debug_output) ? " --debug" : "",
                    (print_adapters_info) ? "" : " --no-adapters-info");
        if (concat_res > full_command_size) {
            DRINN_PRINT_ERROR("Insufficient buffer size to hold the full autostart command\n");
            if (full_command) free(full_command);
            if (work_dir_path) free(work_dir_path);
            if (full_exe_path) free(full_exe_path);
            return DRINN_INSUFFICIENT_BUFFER_ERROR;
        }
        if (concat_res < 0) {
            DRINN_PRINT_ERROR("An encoding error occured while composing the autostart command\n");
            if (full_command) free(full_command);
            if (work_dir_path) free(work_dir_path);
            if (full_exe_path) free(full_exe_path);
            return DRINN_INSUFFICIENT_BUFFER_ERROR;
        }
        HKEY drinn_autostart_key = NULL;
        LSTATUS create_reg_result =
            RegCreateKeyA(
                HKEY_CURRENT_USER,
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                &drinn_autostart_key);
        if (create_reg_result == 0) {
            LSTATUS set_reg_result =
                RegSetValueExA(
                    drinn_autostart_key,
                    "DRINN", 0, REG_SZ,
                    (unsigned char *) full_command,
                    strlen(full_command)+1);
            if (set_reg_result != 0) {
                DRINN_PRINT_ERROR("Error setting the value of the registry key to autostart drinn: %ld\n", set_reg_result);
                if (full_command) free(full_command);
                if (work_dir_path) free(work_dir_path);
                if (full_exe_path) free(full_exe_path);
                return DRINN_GENERIC_ERROR;
            }
        } else {
            DRINN_PRINT_ERROR(
                "Error creating the registry key to autostart drinn: %ld\n",
                create_reg_result);
            if (full_command) free(full_command);
            if (work_dir_path) free(work_dir_path);
            if (full_exe_path) free(full_exe_path);
            return DRINN_GENERIC_ERROR;
        }
        DRINN_PRINT_DEBUG("Successfully set autostart register key with content: %s\n", full_command);
        if (full_command) free(full_command);
    }
    if (work_dir_path) free(work_dir_path);
    if (full_exe_path) free(full_exe_path);
    // ### END OF SETTING AUTOSTART

    // ### SOUND INITIALIZATION
    ma_result ma_ret;
    ma_ret = ma_engine_init(NULL, &drinn_sound_engine);
    if (ma_ret != MA_SUCCESS) {
        DRINN_PRINT_ERROR("Could not initialize the audio engine\n");
        return DRINN_STARTUP_ERROR;
    }
    DRINN_PRINT_DEBUG("Audio engine successfully created\n");
    ma_ret = ma_sound_init_from_file(&drinn_sound_engine,
                                     DRINN_DEFAULT_AUDIO_FILE,
                                     MA_SOUND_FLAG_DECODE, NULL, NULL,
                                     &drinn_sound);
    if (ma_ret != MA_SUCCESS) {
        DRINN_PRINT_ERROR("Could not initialize the drinn sound\n");
        return DRINN_STARTUP_ERROR;
    }
    ma_sound_set_volume(&drinn_sound, volume_arg);
    DRINN_PRINT_DEBUG("Sound \"%s\" successfully loaded\n", DRINN_DEFAULT_AUDIO_FILE);
    // ### END OF SOUND INITIALIZATION

    // ### WSA INITIALIZATION AND SOCKET OPENING
    drinn_bool running = TRUE;
    int result;

    SOCKET listen_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;

    WSADATA wsa_data;

    result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (result) {
        DRINN_PRINT_ERROR("WSAStartup failed: %d\n", result);
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
        DRINN_PRINT_ERROR("getaddrinfo failed: %d\n", result);
        WSACleanup();
        return DRINN_GETADDRINFO_ERROR;
    }
    DRINN_PRINT_DEBUG("Getaddrinfo successful!\n");

    // ### PRINTING ADAPTER INFO
    if (print_adapters_info) {
        DWORD return_value, addresses_size;
        PIP_ADAPTER_ADDRESSES adapter_addresses = NULL;
        return_value = GetAdaptersAddresses(result_info->ai_family,
                                            GAA_FLAG_SKIP_ANYCAST |
                                            GAA_FLAG_SKIP_MULTICAST |
                                            GAA_FLAG_SKIP_DNS_SERVER |
                                            GAA_FLAG_INCLUDE_PREFIX,
                                            NULL, NULL,
                                            &addresses_size);
        if (return_value != ERROR_BUFFER_OVERFLOW) {
            DRINN_PRINT_ERROR("GetAdaptersAddresses failed: %lu\n", return_value);
            freeaddrinfo(result_info);
            WSACleanup();
            return DRINN_GET_ADAPTERS_ERROR;
        }
        adapter_addresses = (PIP_ADAPTER_ADDRESSES) malloc(addresses_size);
        return_value = GetAdaptersAddresses(result_info->ai_family,
                                            GAA_FLAG_SKIP_ANYCAST |
                                            GAA_FLAG_SKIP_MULTICAST |
                                            GAA_FLAG_SKIP_DNS_SERVER |
                                            GAA_FLAG_INCLUDE_PREFIX,
                                            NULL, adapter_addresses,
                                            &addresses_size);
        if (return_value != ERROR_SUCCESS) {
            DRINN_PRINT_ERROR("GetAdaptersAddresses failed: %lu\n", return_value);
            free(adapter_addresses);
            freeaddrinfo(result_info);
            WSACleanup();
            return DRINN_GET_ADAPTERS_ERROR;
        }
        DRINN_PRINT("###################\n");
        DRINN_PRINT("## Adapters info ##\n");
        DRINN_PRINT("###################\n");
        DRINN_PRINT("#\n");
        for(PIP_ADAPTER_ADDRESSES aa = adapter_addresses;
            aa != NULL; aa = aa->Next) {
            char buf[BUFSIZ];
            memset(buf, 0, BUFSIZ);
            WideCharToMultiByte(CP_ACP, 0,
                                aa->FriendlyName,wcslen(aa->FriendlyName),
                                buf, BUFSIZ, NULL, NULL);
            DRINN_PRINT("# Adapter: %s\n", buf);
            for(PIP_ADAPTER_UNICAST_ADDRESS ua = aa->FirstUnicastAddress;
                ua != NULL; ua = ua->Next) {
                memset(buf, 0, BUFSIZ);
                getnameinfo(ua->Address.lpSockaddr,
                            ua->Address.iSockaddrLength,
                            buf, sizeof(buf),
                            NULL, 0, NI_NUMERICHOST);
                DRINN_PRINT("#   %s\n", buf);
            }
            DRINN_PRINT("#\n");
        }
        DRINN_PRINT("###################\n\n");
        free(adapter_addresses);
    }
    // ### END OF PRINTING ADAPTER INFO

    listen_socket = socket(result_info->ai_family,
                           result_info->ai_socktype,
                           result_info->ai_protocol);

    if (listen_socket == INVALID_SOCKET) {
        DRINN_PRINT_ERROR("socket failed: %d\n", WSAGetLastError());
        freeaddrinfo(result_info);
        WSACleanup();
        return DRINN_SOCKET_ERROR;
    }
    DRINN_PRINT_DEBUG("Socket successful!\n");

    result = bind(listen_socket, result_info->ai_addr,
                  (int)result_info->ai_addrlen);
    freeaddrinfo(result_info); // free wether bind was successful or not
    if (result) {
        DRINN_PRINT_ERROR("bind failed: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return DRINN_BIND_ERROR;
    }
    DRINN_PRINT_DEBUG("Bind successful!\n");

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        DRINN_PRINT_ERROR("listen failed: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return DRINN_LISTEN_ERROR;
    }
    DRINN_PRINT_DEBUG("Listen successful!\n");
    // ### END OF WSA INITIALIZATION AND SOCKET OPENING

    // DRINN_PRINT("Server local IP: %s\n", server_ip);

    while (running) {
        DRINN_PRINT("Ready to DRINN...\n");
        client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            DRINN_PRINT_ERROR("accept failed: %d\n", WSAGetLastError());
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
        DRINN_PRINT_ERROR("shutdown failed: %d\n", WSAGetLastError());
        closesocket(socket);
        return DRINN_SHUTDOWN_ERROR;
    }
    closesocket(socket);
    DRINN_PRINT("The connection was closed normally.\n");
    return DRINN_SUCCESS;
}

drinn_err manage_connection(SOCKET client_socket) {
    drinn_bool receive_error = FALSE;
    drinn_bool receiving = TRUE;

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
            DRINN_PRINT_ERROR("recv failed: %d\n", WSAGetLastError());
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
        DRINN_PRINT_ERROR("shutdown failed: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return DRINN_SHUTDOWN_ERROR;
    }

    send_result = send(client_socket, send_buffer, strlen(send_buffer), 0);
    if (send_result == SOCKET_ERROR) {
        DRINN_PRINT_ERROR("send failed: %d\n", WSAGetLastError());
        closesocket(client_socket);
        return DRINN_SEND_ERROR;
    }

    DRINN_PRINT_DEBUG("Bytes sent: %s\n", send_buffer);

    // Closing the connection with this client
    return terminate_socket(client_socket);
}

drinn_bool file_exists(const char *path) {
    DWORD dwAttrib = GetFileAttributes(path);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void print_help_message() {
    DRINN_PRINT("\nUsage: drinn.exe [OPTIONS]\n\n");
    DRINN_PRINT("        --help                      Print this message\n\n");
    DRINN_PRINT("        --set-volume <volume>       Set volume.\n");
    DRINN_PRINT("                                    Must be greater than 0 and less or equal to 1.\n");
    DRINN_PRINT("                                    (default: 0.8)\n\n");
    DRINN_PRINT("        --debug                     Activate debug output\n\n");
    DRINN_PRINT("        --no-adapters-info          Do not print adapter info\n\n");
    DRINN_PRINT("        --set-autostart             Set the program to autostart with the same flags\n\n");
}
