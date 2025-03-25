#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define CHARSET "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

typedef struct {
    int connection_id;
    int duration;
    long *messages_sent;
    char *server_ip;
    int server_port;
    int packet_size;
} thread_data_t;

char *generate_payload(int size) {
    char *payload = malloc(size + 1);
    if (!payload) {
        perror("Failed to allocate memory for payload");
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        payload[i] = CHARSET[rand() % (sizeof(CHARSET) - 1)];
    }
    payload[size] = '\0'; // Null-terminate the string
    return payload;
}

void *load_generator(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    int sock;
    struct sockaddr_in server_addr;
    time_t start_time = time(NULL);
    long messages = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->server_port);
    inet_pton(AF_INET, data->server_ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return NULL;
    }

    while (difftime(time(NULL), start_time) < data->duration) {
        char *payload = generate_payload(data->packet_size);
        if (payload) {
            send(sock, payload, data->packet_size, 0);
            messages++;
            free(payload);
        }
    }

    close(sock);
    *(data->messages_sent) += messages; // Update total messages sent
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <IP> <PORT> <TIME> <THREADS> <PACKET_SIZE>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int test_duration = atoi(argv[3]);
    int num_connections = atoi(argv[4]);
    int packet_size = atoi(argv[5]);

    pthread_t *threads = malloc(num_connections * sizeof(pthread_t));
    thread_data_t *data = malloc(num_connections * sizeof(thread_data_t));
    long messages_sent = 0;

    srand(time(NULL)); // Seed the random number generator

    for (int i = 0; i < num_connections; i++) {
        data[i].connection_id = i;
        data[i].duration = test_duration;
        data[i].messages_sent = &messages_sent;
        data[i].server_ip = server_ip;
        data[i].server_port = server_port;
        data[i].packet_size = packet_size;
        pthread_create(&threads[i], NULL, load_generator, &data[i]);
    }

    for (int i = 0; i < num_connections; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Load test completed.\n");
    printf("Total messages sent: %ld\n", messages_sent);

    free(threads);
    free(data);
    return EXIT_SUCCESS;
}