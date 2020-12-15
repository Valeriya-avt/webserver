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
    PNG,
    JPEG,
    WRONG_TYPE
};

enum methods {
    GET,
    POST
};

enum value {
    OKAY,
    ERROR
};

char HEADER_HTTP[] = "HTTP/1.1";
char HEADER_HOST[] = "Host:";
char SEPARATOR[] = "\r\n";
char HEADER_LENGTH[] = "content-length:";
char HEADER_TYPE[] = "content-type:";

char HTTP_VALUE[][4] = {
    "200",
    "404"
};

char HTTP_METHOD[][7] = {
    "GET",
    "POST"
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
    if (read(client_socket, &alpha, sizeof(char)) <= 0) {
        return NULL;
    }
    if (alpha == '/') {
        if (read(client_socket, &alpha, sizeof(char)) <= 0) {
            return NULL;
        }
    }
    while (alpha != ' ' && alpha != '?' && alpha != '\0' && alpha != '\r' && alpha != '\n') {
        word = realloc(word, (n + 1) * sizeof(char));
        word[n] = alpha;
        n++;
        if (read(client_socket, &alpha, sizeof(char)) <= 0) {
            return NULL;
        }
    }
    if (alpha == '\r') {
        read(client_socket, &alpha, sizeof(char));
        if (word == NULL) {
            return NULL;
        }
    }
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
    char ***list = NULL;
    int n = 0;
    do {
        list = realloc(list, (n + 1) * sizeof(char **));
        list[n] = get_client_string(client_socket);
        n++;
    } while (list[n - 1][0] != NULL);
    list = realloc(list, (n + 1) * sizeof(char **));
    list[n] = NULL;
    return list;
}

int file_type(char *file_name) {
    int i = 0, j = 0, type_flag = -1;
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
    if (!strcmp(type, "png"))
        type_flag = PNG;
    if (!strcmp(type, "jpg"))
        type_flag = JPEG;
    if (type_flag < 0)
        type_flag = WRONG_TYPE;
    free(type);
    return type_flag;
}

char *get_content_type(int type_flag) {
    switch(type_flag) {
        case TEXT_OR_HTML:
            return "text/html";
        case BINARY:
            return "text/html";
        case PNG:
            return "image/png";
        case JPEG:
            return "image/jpeg";}
    return "wrong type";
}

int get_length(int fd) {
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

char *get_content_length(int length) {
    int j = 0, tmp;
    tmp = length;
    do {
        tmp /= 10;
        j++;
    } while (tmp != 0);
    char *content_length = malloc((j + 1) * sizeof(char));
    snprintf(content_length, j + 1, "%d", length);
    printf("content_length: %s\n", content_length);
    return content_length;
}

char *get_header(char *file_name, int type_flag, int fd, int length) {
    char *content_type = get_content_type(type_flag);
    puts(content_type);
    char *content_length = get_content_length(length);
    puts(content_length);
    int size = snprintf(NULL, 0, "%s %s%s%s %s%s%s %s%s%s", HEADER_HTTP, HTTP_VALUE[OKAY], SEPARATOR,
    HEADER_TYPE, content_type, SEPARATOR, HEADER_LENGTH, content_length, SEPARATOR, SEPARATOR);
    char *header = malloc(size + 1);
    snprintf(header, size + 1, "%s %s%s%s %s%s%s %s%s%s", HEADER_HTTP, HTTP_VALUE[OKAY], SEPARATOR,
    HEADER_TYPE, content_type, SEPARATOR, HEADER_LENGTH, content_length, SEPARATOR, SEPARATOR);
    puts("puts1");
    puts(header);
    free(content_length);
    puts(header);
    return header;
}

char *response_to_invalid_request() {
    int size = snprintf(NULL, 0, "%s %s%s%s", HEADER_HTTP, HTTP_VALUE[ERROR], SEPARATOR, SEPARATOR);
    char *header = malloc(size + 1);
    snprintf(header, size + 1, "%s %s%s%s", HEADER_HTTP, HTTP_VALUE[ERROR], SEPARATOR, SEPARATOR);
    return header;
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

void send_header(char *header, int client_socket) {
    write(client_socket, header, strlen(header));
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
    int i, j;
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++) {
            if (i == 0 && j == 0) {
                if (strcmp(list[i][j], "GET") != 0 && strcmp(list[i][j], "POST") != 0)
                    return 1;
            }
            if (i == 1 && !j) {
                if (strcmp(list[i][j], "Host:") != 0)
                    return 1;
            }
        }
    }
    return 0;
}

char *get_args(char *str, int *index) {
    int i = *index, j = 0;
    char *word = NULL;
    for (j = 0; str[i] != '=' && str[i] != '&' && str[i] != '\0'; j++, i++) {
        word = realloc(word, (j + 1) * sizeof(char));
        word[j] = str[i];
    }
    word = realloc(word, (j + 1) * sizeof(char));
    word[j] = '\0';
    *index = i;
    return word;
}

char **get_list_of_args(char *file_name, char *str) {
    int n = 0, index = 0, size;
    char **args = NULL;
    if (str == NULL) {
        return NULL;
    }
    args = realloc(args, (n + 1) * sizeof(char *));
    size = strlen(file_name);
    args[n] = malloc((size + 1) * sizeof(char));
    strcpy(args[n], file_name);
    args[n][size] = '\0';
    n++;
    do {
        args = realloc(args, (n + 1) * sizeof(char *));
        args[n] = get_args(str, &index);
        n++;
        index++;
    } while (str[index - 1] != '\0');
    args = realloc(args, (n + 1) * sizeof(char *));
    args[n] = NULL;
    return args;
}

void run_binary(char *file, char *arg_str, int *flag, int *pipe_read_fd) {
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
        char **args = get_list_of_args(cmd_list[0], arg_str);
        if (execv(cmd_list[0], args) < 0) {
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
            printf("%s ", list[i][j]);
        putchar('\n');
    }
}

void request_is_text(char *file_name, int client_socket, int type_flag, int fd) {
    int length;
    length = get_length(fd);
    char *server_list = get_header(file_name, type_flag, fd, length);
    send_header(server_list, client_socket);
    send_data(client_socket, fd);
    free(server_list);
}

void send_run_binary_result(int client_socket, char *file_name, int pipe_read_fd, int exec_flag) {
    int content_length;
    char *server_list = NULL;
    char *data = get_length_and_rewrite(pipe_read_fd, &content_length);
    if (exec_flag) {
        server_list = response_to_invalid_request();
        send_header(server_list, client_socket);
        free(server_list); /////
        return;
    }
    else {
        server_list = get_header(file_name, BINARY, pipe_read_fd, content_length);
    }
    send_header(server_list, client_socket);
    send_data_array(client_socket, data);
    close(pipe_read_fd);
    free(server_list);
    free(data);
}

void request_is_binary(char *file_name, char *str, int client_socket, int fd) {
    int exec_flag = 0, pipe_read_fd;
    if (!strcmp(str, "HTTP/1.1")) {
        puts("puts2");
        run_binary(file_name, NULL, &exec_flag, &pipe_read_fd);
    }
    else
        run_binary(file_name, str, &exec_flag, &pipe_read_fd);
    send_run_binary_result(client_socket, file_name, pipe_read_fd, exec_flag);
}

void work_with_get_request(int client_socket, char ***client_list, char *file_name, int invalid_flag, int fd) {
    int type_flag;
    char *server_list = NULL;
        type_flag = file_type(file_name);
        switch(type_flag) {
            case TEXT_OR_HTML:
                request_is_text(file_name, client_socket, type_flag, fd);
                break;
            case BINARY:
                request_is_binary(file_name, client_list[0][2], client_socket, fd);
                break;
            case PNG:
                request_is_text(file_name, client_socket, type_flag, fd);
                break;
            case JPEG:
                request_is_text(file_name, client_socket, type_flag, fd);
                break;
            case WRONG_TYPE:
                server_list = response_to_invalid_request();
                send_header(server_list, client_socket);
                free(server_list);
                break;
        }
}

void work_with_post_request(int client_socket, char *file_name) {
    int exec_flag = 0, pipe_read_fd;
    char end;
    char *request_parameters = get_client_word(client_socket, &end);
    if (request_parameters != NULL)
        puts(request_parameters);
    printf("%d\n", exec_flag);
    run_binary(file_name, request_parameters, &exec_flag, &pipe_read_fd);
    send_run_binary_result(client_socket, file_name, pipe_read_fd, exec_flag);
}

void interaction_with_client(int client_socket) {
    int fd;
    int invalid_flag; //file_name_size;
    char ***client_list = NULL;
    client_list = get_client_list(client_socket);
    print_list(client_list);
    invalid_flag = check_client_list(client_list);
    if ((invalid_flag) || (fd = open(client_list[0][1], O_RDONLY)) < 0) { //client_list[0][1] == file_name
        char *server_list = response_to_invalid_request();
        send_header(server_list, client_socket);
        free(server_list);
    } else {
        if (!strcmp(client_list[0][0], "POST"))
            work_with_post_request(client_socket, client_list[0][1]);
        if (!strcmp(client_list[0][0], "GET"))
            work_with_get_request(client_socket, client_list, client_list[0][1], invalid_flag, fd);
    }
    clear_list(client_list);
}

void connect_to_clients(int *client_sockets, struct sockaddr_in *client_addresses,
    int num_of_clients, int server_socket, pid_t *pids) {
    int i, j;
    puts("Wait for connection");
    for (i = 0; i < num_of_clients; i++) {
        while (1) {
            socklen_t size = sizeof(struct sockaddr_in);
            client_sockets[i] = accept(server_socket, (struct sockaddr *) &client_addresses[i], (socklen_t *) &size);
            if (client_sockets[i] < 0) {
                perror("accept");
                return;
            }
            printf("connected: %s %d\n", inet_ntoa(client_addresses[i].sin_addr), ntohs(client_addresses[i].sin_port));
            pids[i] = fork();
            if (pids[i] == 0) {
                for (j = 0; j < i; j++)
                    close(client_sockets[j]);
                interaction_with_client(client_sockets[i]);
                close(client_sockets[i]);
                exit(0);
            } else {
                waitpid(pids[i], NULL, 0);
                close(client_sockets[i]);
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        puts("Incorrect args.");
        puts("./server <port> <num of clients>");
        puts("Example:");
        puts("./server 5000 5");
        return ERR_INCORRECT_ARGS;
    }
    int port = atoi(argv[1]);
    int num_of_clients = atoi(argv[2]);
    int server_socket = init_socket(port, 1);
    int *client_sockets = malloc(num_of_clients * sizeof(int *));
    struct sockaddr_in *client_addresses = malloc(num_of_clients * sizeof(struct sockaddr_in));
    pid_t *pids = malloc(num_of_clients * sizeof(pid_t));
    connect_to_clients(client_sockets, client_addresses, num_of_clients, server_socket, pids);
    free(pids);
    free(client_sockets);
    free(client_addresses);
    return OK;
}
