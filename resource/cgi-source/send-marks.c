#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

char *get_word(int fd, char *end) {
    char *word = NULL, alpha;
    int n = 0;
    if (read(fd, &alpha, sizeof(char)) <= 0) {
        return NULL;
    }
    while (alpha != ',' && alpha != ';' && alpha != '\n') {
        word = realloc(word, (n + 1) * sizeof(char));
        word[n] = alpha;
        n++;
        if (read(fd, &alpha, sizeof(char)) <= 0) {
            return NULL;
        }
    }
    word = realloc(word, (n + 1) * sizeof(char));
    word[n] = '\0';
    *end = alpha;
    return word;
}

char **get_string(int fd) {
    char **list = NULL;
    char end;
    int i, n = 0;
    do {
        list = realloc(list, (n + 1) * sizeof(char *));
        list[n] = get_word(fd, &end);
        n++;
    } while (end != '\n' && list[n - 1] != NULL);
    if (list[n - 1] == NULL) {
        for (i = 0; i < n; i++)
            free(list[i]);
        free(list);
        return NULL;
    }
    list = realloc(list, (n + 1) * sizeof(char *));
    list[n] = NULL;
    return list;
}

char ***get_data_table(int fd) {
    int n = 0;
    char ***list = NULL;
    do {
        list = realloc(list, (n + 1) * sizeof(char **));
        list[n] = get_string(fd);
        n++;
    } while (list[n - 1] != NULL);
    return list;
}

void clear(char ***list) {
    int i, j;
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++)
            free(list[i][j]);
        free(list[i]);
    }
    free(list);
}

void print(char ***list) {
    int i, j;
    for (i = 0; list != NULL && list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++)
            printf("list[%d][%d] = %s\n", i, j, list[i][j]);
    }
}

void check_request(int n, char **query_string, int *user_index, int *subject_index, int *mark_index) {
    int i, j;
    for (i = 0; i < 6; i++) {
        for (j = 1; j < n; j += 2) {
            if (!strcmp(query_string[j], "user")) {
                if (query_string[j + 1] && strcmp(query_string[j + 1], "subject") != 0)
                    *user_index = j;
            }
            if (!strcmp(query_string[j], "subject")) {
                if (query_string[j + 1] && strcmp(query_string[j + 1], "mark") != 0)
                    *subject_index = j;
            }

            if (!strcmp(query_string[j], "mark")) {
                if (strcmp(query_string[j + 1], ""))
                    *mark_index = j;
            }
        }
    }
}

int search_data(char ***list, char **query, int user_index, int subject_index, int *row_num, int *column_num) {
    int i;
    if (user_index >= 0) {
        for (i = 0; list != NULL && list[i] != NULL && list[i][0] != NULL; i++) {
            if (!strcmp(list[i][0], query[user_index + 1]))
                *row_num = i;
        }
        if (*row_num < 0) {
            return -1;
        }
    }
    if (subject_index >= 0) {
        for (i = 0; list != NULL && list[0] != NULL && list[0][i] != NULL; i++) {
            if (!strcmp(list[0][i], query[subject_index + 1]))
                *column_num = i;
        }
        if (*column_num < 0) {
            return -1;
        }
    }
    return 0;
}

char *get_new_marks(char *old_marks, char *new_marks) {
    int size = strlen(old_marks) + strlen(new_marks) + 2;
    char *marks = malloc(size);
    snprintf(marks, size, "%s %s", old_marks, new_marks);
    return marks;
}

void add_mark_to_the_file(char ***data_table, char *new_marks, int row_num, int column_num) {
    int i, j, fd;
    fd = open("resource/database/marks.csv", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    for (i = 0; data_table[i] != NULL; i++) {
        for (j = 0; data_table[i][j] != NULL; j++) {
            if (i == row_num && j == column_num)
                write(fd, new_marks, strlen(new_marks));
            else
                write(fd, data_table[i][j], strlen(data_table[i][j]));
            if (data_table[i][j + 1] != NULL)
                write(fd, ",", sizeof(char));
        }
        write(fd, "\n", sizeof(char));
    }
    close(fd);
}

void send_data(char ***data_table, char **query, int user_index, int subject_index, int mark_index) {
    int row_num = -1, column_num = -1;
    if (user_index >= 0 && subject_index >= 0 && mark_index >= 0 && strcmp(query[mark_index + 1], "")) {
        if (search_data(data_table, query, user_index, subject_index, &row_num, &column_num) >= 0) {
            char *new_marks = get_new_marks(data_table[row_num][column_num], query[mark_index + 1]);
            printf("You have marked %s for %s in %s<br />\n", query[mark_index + 1], query[user_index + 1], query[subject_index + 1]);
            printf("The mark of the user %s in the subject of %s: %s\n", query[user_index + 1],
            query[subject_index + 1], new_marks);
            add_mark_to_the_file(data_table, new_marks, row_num, column_num);
            return;
        }
    }
    puts("Please enter correct parameters");
}

int main(int argc, char **argv) {
    int fd, user_index = -1, subject_index = -1, mark_index = -1;
    if (argc > 7) {
        puts("Too many arguments");
        return 0;
    }
    if (!(argc % 2) && !strcmp(argv[1], "")) {
        puts("Enter your query in the format key=value");
        return 0;
    }
    check_request(argc, argv, &user_index, &subject_index, &mark_index);
    char ***data_table = NULL;
    fd = open("resource/database/marks.csv", O_RDONLY);
    data_table = get_data_table(fd);
    close(fd);
    send_data(data_table, argv, user_index, subject_index, mark_index);
    clear(data_table);
    return 0;
}
