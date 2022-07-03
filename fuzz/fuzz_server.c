#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>

struct Fuzzer{
    uint16_t    port;    
    char*       file;

    FILE*       inFile;
    uint64_t    size;
    uint8_t*    buffer;

    pthread_t   thread;
    int         socket;
};
typedef struct Fuzzer Fuzzer;

static uint8_t pre_encoded_table[] = {
    0x00, 0x00, 0x00, 0xff, 0x07, 0x6c, 0x6f, 0x6e, 0x67, 0x73};

void fuzzinit(Fuzzer *fuzzer){
    
    {//File
        fuzzer->inFile = fopen(fuzzer->file,"rb");

        fseek(fuzzer->inFile, 0L, SEEK_END);
        fuzzer->size = ftell(fuzzer->inFile);
        fseek(fuzzer->inFile, 0L, SEEK_SET);

        fuzzer->buffer = (uint8_t*)calloc(fuzzer->size, sizeof(char));

        fread(fuzzer->buffer, sizeof(char), fuzzer->size, fuzzer->inFile);
    }

    {//Server
        struct sockaddr_in server_addr;
        fuzzer->socket = socket(AF_INET, SOCK_STREAM, 0);

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(fuzzer->port);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        setsockopt(fuzzer->socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

        bind(fuzzer->socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
        listen(fuzzer->socket,1);
    }
}

void *Server(void *args){

    Fuzzer *fuzzer = (Fuzzer*)args;
    {
        int client;
        char clientData[10240];
        struct sockaddr_in clientAddr;
        uint32_t clientSZ = sizeof(clientAddr);

        client = accept(fuzzer->socket, (struct sockaddr*)&clientAddr, &clientSZ);

        recv(client, clientData, sizeof(clientData), 0);

        if(fuzzer->size < 9){
            send(client, pre_encoded_table, sizeof(pre_encoded_table), 0);
        }else{
            send(client, fuzzer->buffer, fuzzer->size, 0);
        }

        close(client);
    }
    pthread_exit(NULL);
}

/*
void clientTest(Fuzzer *fuzzer){// For Testing 
    
    int sock, client_fd;
    struct sockaddr_in serv_addr;
    char* hello = "Hello from client";
    char buffer[10240] = { 0 };
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(fuzzer->port);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return;
    }
    client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    if (client_fd < 0) {
        printf("\nConnection Failed \n");
        return;
    }
    send(sock, hello, strlen(hello), 0);
    read(sock, buffer, sizeof(buffer));

    close(client_fd);
    return;
}
*/
void client(Fuzzer *fuzzer){
    char const *hostname;
    int status;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;

    hostname = "localhost";

    conn = amqp_new_connection();

    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        exit(1);
    }

    status = amqp_socket_open(socket, hostname, fuzzer->port);
    if (status) {
        exit(1);
    }

    amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,"guest", "guest");

    amqp_destroy_connection(conn);
}

void clean(Fuzzer *fuzzer){
    {//File
        free(fuzzer->buffer);
        fclose(fuzzer->inFile);
    }
    {//Server
        close(fuzzer->socket);
    }
    free(fuzzer);
}

int main(int argc, char *argv[]){

    if(argc < 3){
        printf("Server-port,Input-file \n");
        return 0;
    }

    Fuzzer *fuzzer = (Fuzzer*)malloc(sizeof(Fuzzer));
    fuzzer->port = atoi(argv[1]);
    fuzzer->file = argv[2];

    fuzzinit(fuzzer);

    pthread_create(&fuzzer->thread, NULL,Server,fuzzer);

    client(fuzzer);

    pthread_join(fuzzer->thread, NULL);

    clean(fuzzer);

    return 0;
}
