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
    // list = realloc(list, (n + 1) * sizeof(char **));
    // list[n] = NULL;
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

void check_request(int n, char **query_string, int *user_index, int *subject_index) {
    int i, j;
    for (i = 0; i < 2; i++) {
        for (j = 1; j < n; j += 2) {
            if (!strcmp(query_string[j], "user")) {
                *user_index = j;
            }
            if (!strcmp(query_string[j], "subject")) {
                *subject_index = j;
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
            puts("Please enter a valid username");
            return -1;
        }
    }
    if (subject_index >= 0) {
        for (i = 0; list != NULL && list[0] != NULL && list[0][i] != NULL; i++) {
            if (!strcmp(list[0][i], query[subject_index + 1]))
                *column_num = i;
        }
        if (*column_num < 0) {
            printf("%s %s\n", query[user_index + 1], query[subject_index + 1]);
            puts("Please enter a valid discipline");
            return -1;
        }
    }
    return 0;
}

void print_row_or_column(char ***data_table, int row_num, int column_num) {
    int i;
    if (row_num >= 0) {
        for (i = 1; data_table != NULL && data_table[row_num] != NULL && data_table[row_num][i] != NULL; i++) {
            printf("%s ", data_table[row_num][i]);
        }
        puts("");
    }
    if (column_num >= 0) {
        for (i = 1; data_table != NULL && data_table[i] != NULL && data_table[i][column_num] != NULL; i++) {
            printf("%s ", data_table[i][column_num]);
        }
        puts("");
    }
}

void send_data(char ***data_table, char **query, int user_index, int subject_index) {
    int row_num = -1, column_num = -1;
    if (user_index < 0 && subject_index < 0) {
        // for (i = 0; data_table != NULL && data_table[i] != NULL; i++) {
        //     for (j = 0; data_table[i][j] != NULL; j++) {
        //         printf("%s ", data_table[i][j]);
        //     }
        //     puts("");
        // }
        puts("You can find out the marks in:");
        print_row_or_column(data_table, 0, -1);
        puts("from the following users:");
        print_row_or_column(data_table, -1, 0);
    }
    if (user_index >= 0 && subject_index >= 0) {
        if (search_data(data_table, query, user_index, subject_index, &row_num, &column_num) >= 0)
            printf("The mark of the user %s in the subject of %s: %s\n", query[user_index + 1], query[subject_index + 1], data_table[row_num][column_num]);
    } else {
        if (user_index >= 0) {
            if (search_data(data_table, query, user_index, subject_index, &row_num, &column_num) >= 0) {
                printf("User %s can have marks in subjects: ", query[user_index + 1]);
                print_row_or_column(data_table, 0, column_num - 1);
            }
        } else {
            if (subject_index >= 0) {
                if (search_data(data_table, query, user_index, subject_index, &row_num, &column_num) >= 0) {
                    printf("The following users may have marks in %s: ", query[subject_index + 1]);
                    print_row_or_column(data_table, row_num - 1, 0);
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    int fd, user_index = -1, subject_index = -1;
    if (argc > 5) {
        puts("Too many arguments");
        return 0;
    }
    if (!(argc % 2)) {
        puts("Enter your query in the format key=value");
        return 0;
    }
    check_request(argc, argv, &user_index, &subject_index);
    if ((argc == 4 && (user_index < 0 || subject_index < 0)) ||
       (argc == 2 && user_index < 0 && subject_index < 0)) {
        puts("Invalid arguments");
        return 0;
    }
    char ***data_table = NULL;
    fd = open("resource/database/marks.csv", O_RDONLY);
    data_table = get_data_table(fd);
    send_data(data_table, argv, user_index, subject_index);
    close(fd);
 //   print(data_table);
    clear(data_table);
    return 0;
}
