#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>


#define BUFFER_SIZE 1024
#define MAX_CLIENT 10
#define CLIENT_ADDR "127.0.0.1"

void handle_zombie_child(int signum)
{
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}

struct Info {
    std::string password_;
    u_int16_t port_;
};

using Client = std::map<std::string, Info>;

void registration(
        struct sockaddr_in client_address, int new_socket, std::string login)
{
    char serv_send[BUFFER_SIZE];
    char serv_recv[BUFFER_SIZE];
    int registration = 0;

    bzero(serv_send, sizeof(BUFFER_SIZE));
    bzero(serv_recv, sizeof(BUFFER_SIZE));

    sprintf(serv_send, "1 - Регистрация\n2 - Авторизация");
    if (send(new_socket, serv_send, sizeof(serv_send), 0) < 0) {
        perror("Не удалось отдать сообщение");
        exit(EXIT_FAILURE);
    }

    if (recv(new_socket, serv_recv, sizeof(serv_recv), 0) < 0) {
        perror("Не удалось принять сообщение");
        exit(EXIT_FAILURE);
    }

    if (serv_recv[0] == '1') {
        registration = 1;
    } else if (serv_recv[0] == '2') {
        registration = 2;
    } else {
        perror("Не удалось распознать команду");
        exit(EXIT_FAILURE);
    }

    sprintf(serv_send, "Введите логин: ");
    if (send(new_socket, serv_send, sizeof(serv_send), 0) < 0) {
        perror("Не удалось отдать сообщение");
        exit(EXIT_FAILURE);
    }

    if (recv(new_socket, serv_recv, sizeof(serv_recv), 0) < 0) {
        perror("Не удалось принять сообщение");
        exit(EXIT_FAILURE);
    }

    std::string temp_login = serv_recv;
    sprintf(serv_send, "Введите пороль: ");
    if (send(new_socket, serv_send, sizeof(serv_send), 0) < 0) {
        perror("Не удалось отдать сообщение");
        exit(EXIT_FAILURE);
    }

    if (recv(new_socket, serv_recv, sizeof(serv_recv), 0) < 0) {
        perror("Не удалось принять сообщение");
        exit(EXIT_FAILURE);
    }

    std::ifstream read_file("clients.txt");
    std::string line;
    Client clients;
    while (std::getline(read_file, line)) {
        std::string temp_login_ = line.substr(0, line.find(' '));
        line.erase(0, line.find(' ') + 1);

        Info temp_info_;
        temp_info_.password_ = line.substr(0, line.find(' '));
        line.erase(0, line.find(' ') + 1);

        temp_info_.port_ = std::stoi(line);

        clients.insert({temp_login_, temp_info_});
    }
    read_file.close();

    if (registration == 1) {
        if (clients.find(temp_login) != clients.end()) {
            sprintf(serv_send, "Такой логин уже существует");
            if (send(new_socket, serv_send, sizeof(serv_send), 0) < 0) {
                perror("Не удалось отдать сообщение");
                exit(EXIT_FAILURE);
            }
            perror("Такой логин уже существует");
            exit(EXIT_FAILURE);
        }

        Info info = {serv_recv, ntohs(client_address.sin_port)};
        clients.insert({temp_login, info});
        sprintf(serv_send, "Регистрация успешно завершена");
        if (send(new_socket, serv_send, sizeof(serv_send), 0) < 0) {
            perror("Не удалось отдать сообщение");
            exit(EXIT_FAILURE);
        }
    } else if (registration == 2) {
        if (clients.find(temp_login) == clients.end()) {
            sprintf(serv_send, "Такого логина не существует");
            if (send(new_socket, serv_send, sizeof(serv_send), 0) < 0) {
                perror("Не удалось отдать сообщение");
                exit(EXIT_FAILURE);
            }
            perror("Неверный логин или пароль");
            exit(EXIT_FAILURE);
        }

        if (clients.at(temp_login).password_ != serv_recv) {
            sprintf(serv_send, "Неверный логин или пароль");
            if (send(new_socket, serv_send, sizeof(serv_send), 0) < 0) {
                perror("Не удалось отдать сообщение");
                exit(EXIT_FAILURE);
            }
            perror("Неверный логин или пароль");
            exit(EXIT_FAILURE);
        }

        clients.at(temp_login).port_ = ntohs(client_address.sin_port);
        sprintf(serv_send, "Вход выполнен успешно");
        if (send(new_socket, serv_send, sizeof(serv_send), 0) < 0) {
            perror("Не удалось отдать сообщение");
            exit(EXIT_FAILURE);
        }
    }
    login = temp_login;
    std::ofstream chat_file(
            "chat.txt", std::ios::ate | std::ios::out | std::ios::app);
    chat_file << login << " подключился к чату." << std::endl;
    chat_file.close();

    std::ofstream write_file("clients.txt");
    write_file << "";
    for (auto iter = clients.begin(); iter != clients.end(); iter++) {
        write_file << iter->first << " " << iter->second.password_ << " "
                   << iter->second.port_ << "\n";
    }
    write_file.close();
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;

    char buffer[BUFFER_SIZE];
    bzero(buffer, sizeof(BUFFER_SIZE));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Сбой сокета");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = 0;

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Не удалось выполнить привязку сокета");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Не удалось прослушать сокет");
        exit(EXIT_FAILURE);
    }

    unsigned int length = sizeof(address);
    if (getsockname(server_fd, (struct sockaddr*)(&address), &length)) {
        perror("Не удалось получить имя сокета");
        exit(EXIT_FAILURE);
    }

    printf("СЕРВЕР: номер порта - %d\n", ntohs(address.sin_port));

    signal(SIGCHLD, handle_zombie_child);

    struct sockaddr_in client_address;
    unsigned int client_addrlen = sizeof(client_address);
    char login[BUFFER_SIZE];
    bzero(login, sizeof(BUFFER_SIZE));

    while (1) {
        if ((new_socket = accept(
                     server_fd,
                     (struct sockaddr*)&client_address,
                     &client_addrlen))
            < 0) {
            perror("Не удалось принять соединение");
            continue;
        } else {
            printf("Подключился клиент[%s:%d]\n",
                   inet_ntoa(client_address.sin_addr),
                   ntohs(client_address.sin_port));
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("Не удалось создать процесс");
            close(new_socket);
            continue;
        }

        if (pid == 0) {
            close(server_fd);
            ssize_t bytesRead;

            registration(client_address, new_socket, login);
            while ((bytesRead = recv(new_socket, buffer, sizeof(buffer), 0))
                   > 0) {
                buffer[bytesRead] = '\0';

                std::cout << buffer;

                std::ofstream chat_file(
                        "chat.txt",
                        std::ios::ate | std::ios::out | std::ios::app);
                chat_file << buffer;
                chat_file.close();

                if (send(new_socket, buffer, sizeof(buffer), 0) < 0) {
                    perror("Не удалось отдать сообщение");
                    exit(EXIT_FAILURE);
                }

            }

            close(new_socket);
            printf("Отключился клиент[%s:%d]\n",
                   inet_ntoa(client_address.sin_addr),
                   ntohs(client_address.sin_port));
            exit(EXIT_SUCCESS);
        } else {
            close(new_socket);
        }
    }

    close(server_fd);
    return 0;
}
