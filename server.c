#include <netinet/in.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "modbus.h"
#include "packet.h"

#define PORT 502
#define EPOLL_SIZE 20
#define BACKLOG 5
#define MAXLINE 1024
#define MAXCLIENT 10  // 연결 가능한 클라이언트 수
#define MAXRESLEN 260 // 응답 패킷의 data 최대 길이

extern struct MODBUS_CLIENT_INFO;
extern struct MODBUS_TCP_PACKET;

int client_fds[MAXCLIENT];
int requestcnt = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

ModbusMappingTbl tbl;

void handleClientThread(void *arg)
{
    ModbusCliInfo *client_info = (ModbusCliInfo *)arg;
    uint8_t buf[MAXLINE];
    int readn;

    while (1)
    {
        memset(buf, 0x00, MAXLINE);

        if ((readn = read(client_info->fd, buf, MAXLINE)) == -1)
        {
            perror("[Error] read() error");
            break;
        }
        else if (readn == 0)
        {
            printf("client disconnected (%d) \n", client_info->fd);
            break;
        }

        // 요청 처리
        printf("\n-------------- Request %d --------------\n", ++requestcnt);
        printf("Request from client fd %d \n\n", client_info->fd);

        size_t reqlen = getPacketLength(buf);

        for (size_t i = 0; i < reqlen + HEADER_LEN - 1; i++)
        {
            printf("%02X ", buf[i]);
            if ((i + 1) > 0 && (i + 1) % 10 == 0)
                printf("\n");
        }
        printf("\n");

        ModbusPkt req;
        req.pdu = (ModbusPDU *)malloc(reqlen);

        if (decodeModbusPacket(buf, &req) == -1)
        {
            perror("[Error] decodeModbusPacket() error");
            break;
        }

        client_info->u_id = req.u_id;

        // 응답 보내기
        uint8_t result[MAXRESLEN];
        int resdatalen;

        if ((resdatalen = handleRequest(&tbl, req.pdu, result)) == -1)
        {
            perror("[Error] handleRequest() error");
            return 1;
        }

        ModbusPkt res;
        res.pdu = (ModbusPDU *)malloc(resdatalen + sizeof(req.pdu->f_code));
        uint8_t *resbuf = (uint8_t *)malloc(resdatalen + HEADER_LEN);
        createResponsePacket(&req, &res, result, resdatalen, resbuf);

        printf("\nResponse\n");
        size_t reslen = resdatalen + HEADER_LEN + sizeof(res.pdu->f_code);
        for (size_t i = 0; i < reslen; i++)
        {
            printf("%02X ", resbuf[i]);
            if ((i + 1) > 0 && (i + 1) % 10 == 0)
                printf("\n");
        }
        printf("\n");

        if (write(client_info->fd, resbuf, reslen) == -1)
        {
            perror("[Error] send() error\n");
            return 1;
        }

        if (req.pdu != NULL)
        {
            printf("req.pdu\n");
            free(req.pdu);
            req.pdu = NULL;
        }

        if (resbuf != NULL)
        {
            printf("res_msg\n");
            free(resbuf);
            resbuf = NULL;
        }

        if (res.pdu != NULL)
        {
            printf("res.pdu\n");
            free(res.pdu);
            res.pdu = NULL;
        }

        sleep(1);
    }
}

int main()
{
    initMappingTable(&tbl, 4096, 4096, 4096, 4096);

    struct sockaddr_in serv_addr, cli_addr;
    struct epoll_event ev, *events;

    int serverfd;
    int clientfd;
    int epollfd;

    int eventnum;

    socklen_t servlen, clilen;

    events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * EPOLL_SIZE);
    if ((epollfd = epoll_create(100)) == -1)
    {
        perror("[Error] epoll_create() error");
        return 1;
    }

    servlen = sizeof(serv_addr);
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[Error] socket() error");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverfd, (struct sockaddr *)&serv_addr, servlen) == -1)
    {
        perror("[Error] bind() error\n");
        return 1;
    }

    if (listen(serverfd, BACKLOG) == -1)
    {
        perror("[Error] listen() error\n");
        return 1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = serverfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &ev);
    memset(client_fds, -1, sizeof(int) * MAXCLIENT);

    while (1)
    {
        eventnum = epoll_wait(epollfd, events, EPOLL_SIZE, -1);
        if (eventnum == -1)
        {
            perror("[Error] epoll_wait() error\n");
            return 1;
        }

        for (int i = 0; i < eventnum; i++)
        {
            if (events[i].data.fd == serverfd) // listen socket 에서 이벤트 발생
            {
                clilen = sizeof(struct sockaddr);
                if ((clientfd = accept(serverfd, (struct sockaddr *)&cli_addr, &clilen)) == -1)
                {
                    perror("[Error] accept() error\n");
                    return 1;
                }
                client_fds[clientfd] = 1;
                ModbusCliInfo *client_info = (ModbusCliInfo *)malloc(sizeof(ModbusCliInfo));
                client_info->fd = clientfd;

                ev.events = EPOLLIN | EPOLLET;
                ev.data.ptr = client_info;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev) == -1)
                {
                    perror("[Error] epoll_ctl() error");
                    return 1;
                }

                printf("*** Connected client fd: %d\n", clientfd);
            }
            else
            {
                ModbusCliInfo *client_info = events[i].data.ptr;

                if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
                {
                    if (events[i].events & EPOLLHUP) // hang up
                        printf("client disconnected: %d\n", client_info->fd);

                    else if (events[i].events & EPOLLRDHUP) // read hang up
                        printf("client half-closed connection: %d\n", client_info->fd);

                    else if (events[i].events & EPOLLERR)
                    {
                        int error = 0;
                        socklen_t errlen = sizeof(error);
                        if (getsockopt(client_info->fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
                        {
                            printf("Socket error code: %d\n", error);
                        }
                    }

                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, client_info->fd, NULL) == -1)
                    {
                        perror("[Error] epoll_ctl() error");
                        return 1;
                    }
                    close(client_info->fd);
                    pthread_mutex_unlock(&lock);
                    free(client_info);
                }
                else
                {
                    pthread_t thread_id;
                    pthread_create(&thread_id, NULL, handleClientThread, (void *)client_info);
                    pthread_detach(thread_id);
                }
            }
        }
    }

    close(serverfd);
    freeMappingTable(&tbl);
    return 0;
}