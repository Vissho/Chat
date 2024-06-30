#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <climits>
#include <fstream>
#include <iostream>
#include <limits>

#define BUFFER_SIZE 1024
#define CLIENT_ADDR "127.0.0.1"

bool enterence(int new_socket, char* login)
{
    char cli_send[BUFFER_SIZE];
    char cli_recv[BUFFER_SIZE];

    bzero(cli_recv, sizeof(BUFFER_SIZE));
    bzero(cli_send, sizeof(BUFFER_SIZE));

    if (recv(new_socket, cli_recv, sizeof(cli_recv), 0) < 0) {
        perror("Не удалось принять сообщение");
        exit(EXIT_FAILURE);
    }

    std::cout << cli_recv << std::endl;
    std::cin >> cli_send;
    while (strcmp(cli_send, "1") != 0 && strcmp(cli_send, "2") != 0) {
        std::cout << "Введена неправильная команда. Попробуйте ещё раз"
                  << std::endl;
        std::cin >> cli_send;
    }

    if (send(new_socket, cli_send, sizeof(cli_send), 0) < 0) {
        perror("Не удалось отдать сообщение");
        exit(EXIT_FAILURE);
    }

    if (recv(new_socket, cli_recv, sizeof(cli_recv), 0) < 0) {
        perror("Не удалось принять сообщение");
        exit(EXIT_FAILURE);
    }

    std::cout << cli_recv << std::endl;
    std::cin >> cli_send;
    strcpy(login, cli_send);

    if (send(new_socket, cli_send, sizeof(cli_send), 0) < 0) {
        perror("Не удалось отдать сообщение");
        exit(EXIT_FAILURE);
    }

    if (recv(new_socket, cli_recv, sizeof(cli_recv), 0) < 0) {
        perror("Не удалось принять сообщение");
        exit(EXIT_FAILURE);
    }

    std::cout << cli_recv << std::endl;
    std::cin >> cli_send;

    if (send(new_socket, cli_send, sizeof(cli_send), 0) < 0) {
        perror("Не удалось отдать сообщение");
        exit(EXIT_FAILURE);
    }

    if (recv(new_socket, cli_recv, sizeof(cli_recv), 0) < 0) {
        perror("Не удалось принять сообщение");
        exit(EXIT_FAILURE);
    }

    std::cout << cli_recv << std::endl;

    if ((strcmp(cli_recv, "Вход выполнен успешно") == 0)
        || (strcmp(cli_recv, "Регистрация успешно завершена") == 0)) {
        return true;
    }

    return false;
}

void chat(void)
{
    std::cout << "\033[0;0f";
    std::cout << "Добро пожаловать в мессенджер FOX.\nНапишите 'выйти' для "
                 "выхода из чата."
              << std::endl;
    std::ifstream read_file("chat.txt");
    std::string line;
    while (std::getline(read_file, line)) {
        std::cout << line << std::endl;
    }
    read_file.close();
    std::cout << "> ";
    fseek(stdout, 0, SEEK_CUR);
}

void signalhandler(__attribute__((unused)) int signo)
{
    chat();
}

int timer(void)
{
    struct itimerval nval, oval;

    signal(SIGALRM, signalhandler);

    nval.it_interval.tv_sec = 3;
    nval.it_interval.tv_usec = 0;
    nval.it_value.tv_sec = 3;
    nval.it_value.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &nval, &oval))
        return -5;

    return 0;
}

int main(int argc, char* argv[])
{
    int sock;
    struct sockaddr_in server_address;
    char cli_send[BUFFER_SIZE];
    char cli_recv[BUFFER_SIZE];

    bzero(cli_send, sizeof(BUFFER_SIZE));
    bzero(cli_recv, sizeof(BUFFER_SIZE));

    if (argc != 2) {
        printf("Введите: порт сервера\n");
        exit(1);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Сбой сокета");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(CLIENT_ADDR);
    server_address.sin_port = htons(atoi(argv[1]));

    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address))
        < 0) {
        perror("Не удалось подключиться");
        exit(EXIT_FAILURE);
    }

    char login[BUFFER_SIZE];
    bzero(login, sizeof(BUFFER_SIZE));

    char message[BUFFER_SIZE];
    bzero(message, sizeof(BUFFER_SIZE));

    if (enterence(sock, login)) {
        timer();
        std::cout << "\033[2J";
        while (1) {
            fgets(message, BUFFER_SIZE, stdin);

            if (strcmp(message, "выйти\n") == 0) {
                strcat(cli_send, login);
                strcat(cli_send, " отключается.\n");
                if (send(sock, cli_send, sizeof(cli_send), 0) < 0) {
                    perror("Не удалось отдать сообщение");
                    exit(EXIT_FAILURE);
                }
                break;
            }

            if (strcmp(message, "\n") == 0) {
                chat();
                continue;
            }

            bzero(cli_send, sizeof(BUFFER_SIZE));
            strcat(cli_send, login);
            strcat(cli_send, ": ");
            strcat(cli_send, message);

            if (send(sock, cli_send, sizeof(cli_send), 0) < 0) {
                perror("Не удалось отдать сообщение");
                exit(EXIT_FAILURE);
            }

            if (recv(sock, cli_recv, sizeof(cli_recv), 0) < 0) {
                perror("Не удалось принять сообщение");
                exit(EXIT_FAILURE);
            }

            if (strcmp(cli_recv, "Ошибка")) {
                chat();
            } else {
                std::cout << "Произошла ошибка. Перезагрузите чат" << std::endl;
            }
        }
    } else {
        std::cout << "Вы не смогли авторизоваться, повторите попытку."
                  << std::endl;
    }

    close(sock);

    return 0;
}