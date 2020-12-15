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

enum methods {
    GET,
    POST
};

char HEADER_HTTP[] = "HTTP/1.1";
char HEADER_HOST[] = "Host:";
char SEPARATOR[] = "\r\n";

char HTTP_METHOD[][7] = {
    "GET",
    "POST"
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

char *get_word(char separator) {
    char *word = NULL, alpha;
    int n = 0;
    if (read(0, &alpha, sizeof(char)) <= 0) {
        perror("read");
        return NULL;
    }
    while (alpha != separator) {
        word = realloc(word, (n + 1) * sizeof(char));
        word[n] = alpha;
        n++;
        if (read(0, &alpha, sizeof(char)) <= 0) {
            perror("read");
            return NULL;
        }
    }
    word = realloc(word, (n + 1) * sizeof(char));
    word[n] = '\0';
    return word;
}

void append(char *header, char *word) {
    int header_size, word_size;
    if (header == NULL)
        header_size = 0;
    else
        header_size = strlen(header);
    word_size = strlen(word);
    header = realloc(header, header_size + word_size + 1);
    snprintf(header + header_size, word_size + 1, "%s", word);
}

char *get_header(char *ip) {
    int index;
    char *path = get_word('\n');
    if (!strcmp(path, "resource/cgi-bin/send-marks")) {
        index = POST;
    }
    else
        index = GET;
    int size = snprintf(NULL, 0, "%s %s %s%s%s %s%s%s", HTTP_METHOD[index], path, HEADER_HTTP, SEPARATOR,
    HEADER_HOST, ip, SEPARATOR, SEPARATOR);
    char *header = malloc(size + 1);
    snprintf(header, size + 1, "%s %s %s%s%s %s%s%s", HTTP_METHOD[index], path, HEADER_HTTP, SEPARATOR,
    HEADER_HOST, ip, SEPARATOR, SEPARATOR);
    free(path);
    return header;
}

void print(int server) {
    char ch;
    while ((read(server, &ch, sizeof(char)) > 0) && ch != EOF) {
        if (write(1, &ch, sizeof(char)) <= 0)
            return;
    }
}

int main(int argc, char **argv) {
    while (1) {
        puts("Please enter your request");
        char *ip = get_word(':');
        char *port_str = get_word('/');
        int port = atoi(port_str);
        int server = init_socket(ip, port);
        char *header = get_header(ip);
        printf("%s", header);
        write(server, header, strlen(header));
        free(header);
        free(ip);
        free(port_str);
        print(server);
        close(server);
    }
    return OK;
}
