#include "server.h"
// lwIP libraries
#include "lwip/inet.h"
#include "lwip/sockets.h"



/// Logging struct for TCP server
static tcp_server_logs_S logs =
{
    .server_port      = 0,
    .socket           = -1,
    .state            = socket_unconnected,
    .queue_size       = 0,
    .packets_received = 0,
};

/**
 *  Creates a TCP socket configured as a server
 *  @returns : Success status
 */
static bool tcp_create_socket(void)
{
    bool success = false;

    if (logs.socket < 0)
    {
        // Create socket
        logs.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (logs.socket == -1) 
        {
            ESP_LOGE("tcp_create_socket", "Error creating socket: %s", strerror(errno));
        }
        else 
        {
            // Set socket option to re-usable
            const int option = 1;
            setsockopt(logs.socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
            ESP_LOGI("server::tcp_create_socket", "Socket successfully created on port %d", logs.server_port);
            logs.state = socket_created;
            success = true;
        }        
    }
    else
    {
        ESP_LOGE("tcp_create_socket", "Socket already created, cannot re-create.");
    }

    return success;
}

/**
 *  Closes the created TCP server socket
 */
static void tcp_close_socket(void)
{
    if (logs.socket > 0)
    {
        // Close socket
        shutdown(logs.socket, SHUT_RDWR);
        close(logs.socket);
        // Reset values
        logs.socket = -1;
        logs.state  = socket_closed;
    }
}

/**
 *  Binds the TCP server socket to a port number
 *  @returns : Success status
 */
static bool tcp_bind_socket(void)
{
    bool success = false;

    const struct sockaddr_in server_address =
    {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(logs.server_port),
    };

    if (bind(logs.socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) 
    {
        // If failed, close socket
        tcp_close_socket();
        ESP_LOGE("tcp_bind_socket", "Error binding: %s", strerror(errno));
    }
    else 
    {
        logs.state = socket_binded;
        ESP_LOGI("server::tcp_bind_socket", "Socket successfully binded.");
        success = true;
    }

    return success;
}

/**
 *  Starts a TCP server for listening for incoming client connections
 *  @param queue_size : Size of queue to listen in
 *  @returns          : Success status
 */
static bool tcp_start_listening(uint8_t queue_size)
{
    bool success = false;

    if (listen(logs.socket, queue_size) < 0) 
    {
        logs.queue_size = queue_size;
        ESP_LOGE("tcp_start_listening", "Error starting to listen on socket. Error: %s", strerror(errno));
    }
    else 
    {
        logs.state = socket_listening;
        ESP_LOGI("server::tcp_start_listening", "Listening on socket port %u", logs.server_port);
        success = true;
    }

    return success;
}

bool tcp_server_init(uint32_t port, uint8_t queue_size)
{
    bool success = true;

    // Store port
    logs.server_port = port;

    // Create socket first
    success &= tcp_create_socket();

    // Then bind socket to port
    if (success) success &= tcp_bind_socket();

    // Start listening
    if (success) success &= tcp_start_listening(queue_size);

    // If failed, clear variables
    if (!success)
    {
        logs.server_port      = 0;
        logs.socket           = -1;
        logs.state            = socket_unconnected;
        logs.queue_size       = 0;
        logs.packets_received = 0;
    }

    return success;
}

bool tcp_server_receive(int client_sock, uint8_t *buffer, uint16_t *size)
{
    bool success = true;

    *size = 0;

    ssize_t size_read = 1;

    // ESP_LOGI("server::tcp_server_receive", "Starting receive...");

    // While there is still data to be read from socket
    while (size_read > 0)
    {
        // Read data from socket
        size_read = recv(client_sock,      ///< Socket
                         buffer + *size,   ///< Address in buffer to start writing to
                         RECV_BUFFER_SIZE, ///< Size to read
                         0);               ///< Flags

        // Add number of bytes read
        *size += size_read;

        if (size_read < 0)
        {
            ESP_LOGE("tcp_server_receive", "Error receiving on socket %i. Error: %s", client_sock, strerror(errno));
            success = false;
            break;
        }
    }

    if (*size > 0)
    {
        logs.packets_received++;
    }

    // Make sure the buffer is null terminated for printing afterwards
    if (*size >= RECV_BUFFER_SIZE)
    {
        buffer[RECV_BUFFER_SIZE - 1] = '\0';
    }
    else
    {
        buffer[*size] = '\0';
    }

    // LOG_INFO("Thanks for sending me a message");
    // printf("[%u] ", *size);
    // for (uint8_t byte = 0; byte <= *size; byte++)
    // {
    //     // Convert to ascii integer
    //     printf("%c", buffer[byte] + '0');
    // }

    return success;
}

int tcp_server_get_socket(void)
{
    return logs.socket;
}

tcp_server_logs_S * tcp_server_get_logs(void)
{
    return &logs;
}