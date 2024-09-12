#include <errno.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
// #include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "modbus.h"
#include "packet.h"

#define PORT 502
#define SERVER_IP "127.0.0.1"
#define MAXBUF 1024
#define MAX_EPOLL_EVENTS 20

void hexStringToUint8array(const char *hexStr, uint8_t *arr)
{
    size_t len = strlen(hexStr);
    size_t cnt = 0;

    for (size_t i = 0; i < len; i += 3)
    {
        if (hexStr[i] == ' ')
            continue;
        uint8_t val;
        if (sscanf(hexStr + i, "%2hhx", &val) != 1)
        {
            fprintf(stderr, "[Error] Error parsing hex string\n");
            return;
        }

        arr[cnt++] = val;
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("socket() error\n");
        return 1; // error
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) < 0)
    {
        printf("inet_pton() error\n");
        return -1; // error
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("connect() error\n");
        return -1; // errror
    }

    char reqstr[MAXBUF];
    uint8_t reqarr[MAXBUF];

    while (1)
    {
        // 요청 보내기
        memset(reqstr, 0, MAXBUF);
        memset(reqarr, 0, MAXBUF);
        printf("input: ");
        if (fgets(reqstr, sizeof(reqstr), stdin) == NULL)
        {
            fprintf(stderr, "[Error] Error reading input\n");
            return -1;
        }

        reqstr[strcspn(reqstr, "\n")] = '\0';
        hexStringToUint8array(reqstr, reqarr);

        size_t reqlen = getPacketLength(reqarr);

        if (write(sockfd, reqarr, reqlen + 6) == -1)
        {
            perror("write error\n");
        }

        // 응답 받기
        printf("\n-------------- Response --------------\n");
        uint8_t resarr[MAXBUF];
        if (read(sockfd, resarr, MAXBUF) == -1)
        {
            perror("read error\n");
        }
        size_t reslen = getPacketLength(resarr);
        for (size_t i = 0; i < reslen + 6; i++)
        {
            printf("%02X ", resarr[i]);
            if ((i + 1) > 0 && (i + 1) % 10 == 0)
                printf("\n");
        }
        printf("\n\n");

        ModbusPkt response;
        response.pdu = (ModbusPDU *)malloc(reslen);
        if (decodeModbusPacket(resarr, &response) == -1)
        {
            printf("[Error] stringToPacket error\n");
        }
        if (handleResponse(response.pdu) == -1)
        {
            printf("[Error] handleResponse error\n");
        }

        sleep(1);
    }

    close(sockfd);

    return 0;
}