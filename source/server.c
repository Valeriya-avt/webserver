#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h> // PF_INET, SOCK_STREAM, socket
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <wait.h>

int current_num_of_clients = 0;

enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_SETSOCKETOPT,
    ERR_BIND,
    ERR_LISTEN
};

enum types {
    TEXT_OR_HTML,
    BINARY,
    JPEG,
    WRONG_TYPE
};

int init_socket(int port, int num_of_clients) {
    // open socket, return socket descriptor
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Fail: open socket");
        _exit(ERR_SOCKET);
    }

    //set socket option
    int socket_option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_option, (socklen_t) sizeof(socket_option));
    if (server_socket < 0) {
        perror("Fail: set socket options");
        _exit(ERR_SETSOCKETOPT);
    }

    //set socket address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0) {
        perror("Fail: bind socket address");
        _exit(ERR_BIND);
    }

    //listen mode start
    if (listen(server_socket, num_of_clients) < 0) {
        perror("Fail: bind socket address");
        _exit(ERR_LISTEN);
    }
    return server_socket;
}

char *get_client_word(int client_socket, char *end) {
    char *word = NULL, alpha;
    int n = 0;
    if (read(client_socket, &alpha, sizeof(char)) < 0) {
        return NULL;
    }
    while (alpha != ' ' && alpha != '\0' && alpha != '\r') {
        word = realloc(word, (n + 1) * sizeof(char));
        word[n] = alpha;
        n++;
        if (read(client_socket, &alpha, sizeof(char)) < 0) {
            return NULL;
        }
    }
    if (alpha == '\r')
        read(client_socket, &alpha, sizeof(char));
    word = realloc(word, (n + 1) * sizeof(char));
    word[n] = '\0';
    *end = alpha;
    return word;
}

char **get_client_string(int client_socket) {
    char **list = NULL;
    char end;
    int n = 0;
    do {
        list = realloc(list, (n + 1) * sizeof(char *));
        list[n] = get_client_word(client_socket, &end);
        n++;
    } while (end != '\n' && list[n - 1] != NULL);
    list = realloc(list, (n + 1) * sizeof(char *));
    list[n] = NULL;
    return list;
}

char ***get_client_list(int client_socket) {
    int i;
    char ***list = NULL;
    for (i = 0; i < 3; i++) {
        list = realloc(list, (i + 1) * sizeof(char **));
        list[i] = get_client_string(client_socket);
    }
    list = realloc(list, (i + 1) * sizeof(char **));
    list[i] = NULL;
    return list;
}

char **get_first_string() {
    int i, size;
    char **str = NULL;
    char word_1[] = "HTTP/1.1", word_2[] = "200";
    str = realloc(str, 3 * sizeof(char *));
    for (i = 0; i < 2; i++) {
        if (!i) {
            size = strlen(word_1);
            str[i] = malloc((size + 1) * sizeof(char));
            strcpy(str[i], word_1);
            str[i][size] = '\0';
        }
        if (i) {
            size = strlen(word_2);
            str[i] = malloc((size + 1) * sizeof(char));
            strcpy(str[i], word_2);
            str[i][size] = '\0';
        }

    }
    str[i] = NULL;
    return str;
}

int file_type(char *file_name) {
    int i = 0, j = 0, type_flag;
    char *type = NULL;
    while (1) {
        if (file_name[i] == '\0')
            return BINARY;
        if (file_name[i] == '.') {
            if (i != 0 && i != 1)
                break;
        }
        i++;
    }
    do {
        i++;
        type = realloc(type, (j + 1) * sizeof(char));
        type[j] = file_name[i];
        j++;
    } while (file_name[i] != '\0');
    if (!strcmp(type, "txt") || !strcmp(type, "html"))
        type_flag = TEXT_OR_HTML;
    else
        type_flag = WRONG_TYPE;
    free(type);
    return type_flag;
}

char *content_type(int type_flag) {
    switch(type_flag) {
        case TEXT_OR_HTML:
            return "text/html";
        case BINARY:
            return "application/octet-stream";
        case JPEG:
            return "image/jpeg";
    }
    return "wrong type";
}

char **get_second_string(int type_flag) {
    int i, size;
    char *word_1 = "content-type:";
    char **str = NULL;
    str = realloc(str, 3 * sizeof(char *));
    for (i = 0; i < 2; i++) {
        if (!i) {
            size = strlen(word_1);
            str[i] = malloc((size + 1) * sizeof(char));
            strcpy(str[i], word_1);
            str[i][size] = '\0';
        }
        if (i) {
            char *word_2 = content_type(type_flag);
            size = strlen(word_2);
            str[i] = malloc((size + 1) * sizeof(char));
            strcpy(str[i], word_2);
            str[i][size] = '\0';
        }
    }
    str[i] = NULL;
    return str;
}

int get_content_length(int fd) {
    int file_size;
    struct stat file;
    fstat(fd, &file);
    file_size = file.st_size;
    return file_size;
}

char *get_length_and_rewrite(int fd, int *content_length) {
    int n = 0;
    char *data = NULL, ch;
    while (read(fd, &ch, sizeof(char)) > 0) {
        data = realloc(data, (n + 1) * sizeof(char));
        data[n] = ch;
        n++;
    }
    data = realloc(data, (n + 1) * sizeof(char));
    data[n] = '\0';
    *content_length = n;
    return data;
}

char **get_third_string(char *file_name, int type_flag, int fd, int content_length) {
    int i, j = 0, size, tmp;
    tmp = content_length;
    do {
        tmp /= 10;
        j++;
    } while (tmp != 0);
    char **str = NULL;
    str = realloc(str, 3 * sizeof(char *));
    char *word_1 = "content-length:", *word_2 = malloc((j + 1) * sizeof(char));
    for (i = 0; i < 2; i++) {
        if (!i) {
            size = strlen(word_1);
            str[i] = malloc((size + 1) * sizeof(char));
            strcpy(str[i], word_1);
            str[i][size] = '\0';
        }
        if (i) {
            sprintf(word_2, "%d", content_length);
            str[i] = malloc((j + 1) * sizeof(char));
            strcpy(str[i], word_2);
            str[i][j] = '\0';
        }
    }
    str[i] = NULL;
    free(word_2);
    return str;
}

char ***response_to_invalid_request() {
    int i, size;
    char ***list = NULL;
    char *word_1 = "HTTP/1.1", *word_2 = "404";
    list = realloc(list, 2 * sizeof(char **));
    list[0] = malloc(3 * sizeof(char *));
    for (i = 0; i < 2; i++) {
        if (!i) {
            size = strlen(word_1);
            list[0][i] = malloc((size + 1) * sizeof(char));
            strcpy(list[0][i], word_1);
            list[0][i][size] = '\0';
        }
        if (i) {
            size = strlen(word_2);
            list[0][i] = malloc((size + 1) * sizeof(char));
            strcpy(list[0][i], word_2);
            list[0][i][size] = '\0';
        }
    }
    list[0][i] = NULL;
    list[1] = NULL;
    return list;
}

char ***get_server_list(char *file_name, int type_flag, int fd, int content_length) {
    int i = 1, num_of_str = 3;
    char ***list = NULL;
    for (i = 0; i < num_of_str; i++) {
        list = realloc(list, (i + 1) * sizeof(char **));
        if (!i) {
            list[i] = get_first_string();
        }
        if (i == 1) {
            list[i] = get_second_string(type_flag);
        }
        if (i > 1) {
            list[i] = get_third_string(file_name, type_flag, fd, content_length);
        }
    }
    list = realloc(list, (i + 1) * sizeof(char **));
    list[i] = NULL;
    return list;
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

void send_header(char ***list, int client_socket) {
    int i, j;
    char *separator = " ";
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++) {
            if (write(client_socket, list[i][j], (strlen(list[i][j]) + 1) * sizeof(char)) <= 0) {
                perror("write");
                return;
            }
            if (write(client_socket, separator, sizeof(char)) < 0)
                return;;
        }
        if (write(client_socket, "\n", sizeof(char)) < 0)
            return;
    }
    if (write(client_socket, "\n", sizeof(char)) < 0)
        return;
}

void send_data(int client_socket, int fd) {
    char ch;
    while (read(fd, &ch, sizeof(char)) > 0) {
        if (write(client_socket, &ch, sizeof(char)) <= 0)
            return;
    }
}

void send_data_array(int client_socket, char *data) {
    int n = 0;
    while (data[n] != '\0') {
        if (write(client_socket, &data[n], sizeof(char)) <= 0)
            return;
        n++;
    }
}

int check_client_list(char ***list) {
    int i, j, counter = 0;
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++) {
            if (!i && !j) {
                counter += strcmp(list[i][j], "GET");
            }
            if (!i && j == 2) {
                counter += strcmp(list[i][j], "HTTP/1.1");
            }
            if (i == 1 && !j) {
                counter += strcmp(list[i][j], "Host:");
            }
        }
    }
    if (!counter)
        return 0;
    else
        return 1;
}

void print(char ***list) {
    int i, j;
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++)
            printf("list[%d][%d] = %s", i, j, list[i][j]);
    }
}

void run_binary(char *file, int *flag, int *pipe_read_fd) {
    int size, pipefd[2] = {0};
    pipe(pipefd);
    pid_t pid;
    char **cmd_list = NULL, *final_symbol = NULL;
    cmd_list = realloc(cmd_list, 2 * sizeof(char *));
    size = strlen(file);
    cmd_list[0] = malloc((size + 1) * sizeof(char));
    strcpy(cmd_list[0], file);
    cmd_list[0][size] = '\0';
    cmd_list[1] = NULL;
    pid = fork();
    if (pid == 0) {
        dup2(pipefd[1], 1);
        if (execvp(cmd_list[0], cmd_list) < 0) {
            *flag = 1;
            close(pipefd[0]);
            close(pipefd[1]);
            exit(1);
        }
    } else {
        waitpid(pid, NULL, 0);
        write(pipefd[1], final_symbol, sizeof(char));
        *pipe_read_fd = pipefd[0];
    }
    close(pipefd[1]);
    free(cmd_list[0]);
    free(cmd_list);
}

void print_list(char ***list) {
    int i, j;
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++)
            printf("list[%d][%d] = %s", i, j, list[i][j]);
    }
}

void request_is_text(char *file_name, int client_socket, int fd) {
    int content_length;
    char ***server_list = NULL;
    content_length = get_content_length(fd);
    server_list = get_server_list(file_name, TEXT_OR_HTML, fd, content_length);
    send_header(server_list, client_socket);
    send_data(client_socket, fd);
    clear_list(server_list);
}

void request_is_binary(char *file_name, int client_socket, int fd) {
    int exec_flag = 0, pipe_read_fd, content_length;
    char ***server_list = NULL;
    run_binary(file_name, &exec_flag, &pipe_read_fd);
    char *data = get_length_and_rewrite(pipe_read_fd, &content_length);
    if (exec_flag) {
        server_list = response_to_invalid_request();
        send_header(server_list, client_socket);
        return;
    }
    else {
        server_list = get_server_list(file_name, BINARY, pipe_read_fd, content_length);
    }
    send_header(server_list, client_socket);
    send_data_array(client_socket, data);
    close(pipe_read_fd);
    clear_list(server_list);
    free(data);
}

void interaction_with_client(int client_socket) {
    int invalid_flag, type_flag, fd;
    char ***client_list = NULL, ***server_list = NULL;
    client_list = get_client_list(client_socket);
  //  print_list(client_list);
    invalid_flag = check_client_list(client_list);
    if ((invalid_flag) || (fd = open(client_list[0][1], O_RDONLY)) < 0) { //client_list[0][1] == file_name
        server_list = response_to_invalid_request();
        send_header(server_list, client_socket);
        clear_list(server_list);
    } else {
        type_flag = file_type(client_list[0][1]);
        switch(type_flag) {
            case TEXT_OR_HTML:
                request_is_text(client_list[0][1], client_socket, fd);
                break;
            case BINARY:
                request_is_binary(client_list[0][1], client_socket, fd);
                break;
            case WRONG_TYPE:
                server_list = response_to_invalid_request();
                send_header(server_list, client_socket);
                break;
        }
    }
    clear_list(client_list);
}

void connect_to_clients(int *client_sockets, struct sockaddr_in *client_addresses,
    int *num_of_clients, int server_socket, pid_t *pids) {
    int i, j;
    socklen_t size;
    puts("Wait for connection");
    for (i = 0; i < *num_of_clients; i++) {
        size = sizeof(struct sockaddr_in);
        client_sockets[i] = accept(server_socket, (struct sockaddr *) &client_addresses[i], (socklen_t *) &size);
        if (client_sockets[i] < 0) {
            perror("accept");
            *num_of_clients = i + 1;
            return;
        }
        printf("connected: %s %d\n", inet_ntoa(client_addresses[i].sin_addr), ntohs(client_addresses[i].sin_port));
        pids[i] = fork();
        if (pids[i] == 0) {
            for (j = 0; j < i; j++)
                close(client_sockets[j]);
            while (1) {
                interaction_with_client(client_sockets[i]);
                close(client_sockets[i]);
                client_sockets[i] = accept(server_socket, (struct sockaddr *) &client_addresses[i], (socklen_t *) &size);
                if (client_sockets[i] < 0) {
                    perror("accept");
                    *num_of_clients = i + 1;
                    return;
                }
            }
            close(client_sockets[i]);
            close(server_socket);
            return;
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        puts("Incorrect args.");
        puts("./server <port>");
        puts("Example:");
        puts("./server 5000 (1-5)");
        return ERR_INCORRECT_ARGS;
    }
    int port = atoi(argv[1]);
    int num_of_clients = atoi(argv[2]);
    int server_socket = init_socket(port, 1);
    int i, *client_sockets = malloc(num_of_clients * sizeof(int *));
    struct sockaddr_in *client_addresses = malloc(num_of_clients * sizeof(struct sockaddr_in));
    pid_t *pids = malloc(num_of_clients * sizeof(pid_t));
    connect_to_clients(client_sockets, client_addresses, &num_of_clients, server_socket, pids);
    for (i = 0; i < num_of_clients; i++) {
        close(client_sockets[i]);
        waitpid(pids[i], NULL, 0);
    }
    close(server_socket);
    free(pids);
    free(client_sockets);
    free(client_addresses);
    return OK;
}
