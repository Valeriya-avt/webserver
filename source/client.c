#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_CONNECT
};

int init_socket(const char *ip, int port) {
    //open socket, result is socket descriptor
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Fail: open socket");
        _exit(ERR_SOCKET);
    }

    //prepare server address
    struct hostent *host = gethostbyname(ip);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    memcpy(&server_address.sin_addr, host -> h_addr_list[0], sizeof(server_address.sin_addr));

    //connection
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    memcpy(&sin.sin_addr, host->h_addr_list[0], sizeof(sin.sin_addr));
    if (connect(server_socket, (struct sockaddr*) &sin, (socklen_t) sizeof(sin)) < 0) {
        perror("Fail: connect");
        _exit(ERR_CONNECT);
    }
    return server_socket;
}

char *get_word(int *size, char separator) {
    char *word = NULL, alpha;
    int n = 0;
    if (read(0, &alpha, sizeof(char)) <= 0) {
        perror("read");
        return word;
    }
    while (alpha != separator) {
        word = realloc(word, (n + 1) * sizeof(char));
        word[n] = alpha;
        n++;
        if (read(0, &alpha, sizeof(char)) <= 0) {
            perror("read");
            return word;
        }
    }
    word = realloc(word, (n + 1) * sizeof(char));
    word[n] = '\0';
    *size = n;
    return word;
}

char **get_first_string() {
    int i, size;
    char *word_1 = "GET", *word_3 = "HTTP/1.1";
    char **list = NULL;
    for (i = 0; i < 3; i++) {
        list = realloc(list, (i + 1) * sizeof(char *));
        if (!i) {
            size = strlen(word_1);
            list[i] = malloc((size + 1) * sizeof(char));
            strcpy(list[i], word_1);
            list[i][size] = '\0';
        }
        if (i == 1) {
            list[i] = get_word(&size, '\n');
        }
        if (i > 1) {
            size = strlen(word_3);
        //    printf("strlen(HTTP/1.1/n) = %d\n", size);
            list[i] = malloc((size + 1)* sizeof(char));
            strcpy(list[i], word_3);
            list[i][size] = '\0';
        }
    }
    list = realloc(list, (i + 1) * sizeof(char *));
    list[i] = NULL;
    return list;
}

char **get_second_string(char *ip) {
    int i, size;
    char *word_1 = "Host:";
    char **list = NULL;
    list = realloc(list, 3 * sizeof(char *));
    for (i = 0; i < 2; i++) {
        if (!i) {
            size = strlen(word_1);
            list[i] = malloc((size + 1) * sizeof(char));
            strcpy(list[i], word_1);
            list[i][size] = '\0';
        }
        if (i) {
            size = strlen(ip);
            list[i] = malloc((size + 1) * sizeof(char));
            strcpy(list[i], ip);
            list[i][size] = '\0';
        }
    }
    list[i] = NULL;
    return list;
}

char ***get_list(char *ip) {
    int i;
    char ***list = NULL;
    for (i = 0; i < 2; i++) {
        list = realloc(list, (i + 1) * sizeof(char **));
        if (!i)
            list[i] = get_first_string();
        else
            list[i] = get_second_string(ip);
    }
    list = realloc(list, (i + 1) * sizeof(char **));
    list[i] = NULL;
    return list;
}

void send_data(char ***list, int server) {
    int i, j;
  //  char *end_of_str = "\r\n";
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++) {
            if (write(server, list[i][j], strlen(list[i][j]) * sizeof(char)) <= 0) {
                perror("write");
                exit(1);
            }
            if (list[i][j + 1] != NULL)
                write(server, " ", sizeof(char));
        }
        if (write(server, "\r\n", sizeof(char) * 2) <= 0)
            return;
    }
    if (write(server, "\r\n", sizeof(char) * 2) <= 0)
        return;
}

void print(int server) {
    char ch;
    while (read(server, &ch, sizeof(char)) > 0) {
        if (write(1, &ch, sizeof(char)) <= 0)
            return;
    }
}

void print_list(char ***list) {
    int i, j;
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++)
            printf("list[%d][%d] = %s", i, j, list[i][j]);
    }
}

void clear_list(char ***list) {
    int i, j;
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++)
            free(list[i][j]);
        free(list[i]);
    }
    free(list);
}

int main(int argc, char **argv) {
    int size;
    while (1) {
        puts("Please enter your request");
        char *ip = get_word(&size, ':');
        char *port_str = get_word(&size, '/');
        int port = atoi(port_str);
        int server = init_socket(ip, port);
        char ***list = NULL;
        list = get_list(ip);
    //    print_list(list);
        send_data(list, server);
        print(server);
        clear_list(list);
        close(server);
    }
    return OK;
}
