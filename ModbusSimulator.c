#include <errno.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

#include "modbus.h"
#include "packet.h"

#define PORT 502
#define SERVER_IP "127.0.0.1"
#define MAXBUF 1024
#define MAX_PACKET_SIZE 260

int random_range(int min, int max)
{
    return min + rand() % (max - min + 1);
}

int generate_modbus_request(uint8_t *pkt, int fcode)
{
    uint16_t tid = (uint16_t)random_range(0, 65535);
    uint16_t pid = 0;
    uint8_t uid = (uint8_t)random_range(1, 255);
    uint16_t length = 6;
    int pkt_sz = 0;

    // MBAP HEADER
    pkt[pkt_sz++] = (tid >> 8) & 0xFF;
    pkt[pkt_sz++] = tid & 0xFF;

    pkt[pkt_sz++] = (pid >> 8) & 0xFF;
    pkt[pkt_sz++] = pid & 0xFF;

    pkt[pkt_sz++] = (length >> 8) & 0xFF;
    pkt[pkt_sz++] = length & 0xFF;

    pkt[pkt_sz++] = uid;

    // PDU
    pkt[pkt_sz++] = (uint8_t)fcode;

    uint16_t start_addr, quantity, value;
    uint8_t byte_cnt;

    switch (fcode)
    {
    case 1:
    case 2:
    case 3:
    case 4:
        start_addr = (uint16_t)random_range(0, 65535);
        quantity = (uint16_t)random_range(1, 125);
        pkt[pkt_sz++] = (start_addr >> 8) & 0xFF;
        pkt[pkt_sz++] = start_addr & 0xFF;
        pkt[pkt_sz++] = (quantity >> 8) & 0xFF;
        pkt[pkt_sz++] = quantity & 0xFF;
        break;
    case 5:
        start_addr = (uint16_t)random_range(0, 65535);
        value = random_range(0, 1) ? 0xFF00 : 0X0000;
        pkt[pkt_sz++] = (start_addr >> 8) & 0xFF;
        pkt[pkt_sz++] = start_addr & 0xFF;
        pkt[pkt_sz++] = (value >> 8) & 0xFF;
        pkt[pkt_sz++] = value & 0xFF;
        break;
    case 6:
        start_addr = (uint16_t)random_range(0, 65535);
        value = (uint16_t)random_range(0, 65535);
        pkt[pkt_sz++] = (start_addr >> 8) & 0xFF;
        pkt[pkt_sz++] = start_addr & 0xFF;
        pkt[pkt_sz++] = (value >> 8) & 0xFF;
        pkt[pkt_sz++] = value & 0xFF;
        break;
    case 10:
        start_addr = (uint16_t)random_range(0, 65535);
        quantity = (uint16_t)random_range(1, 123);
        byte_cnt = quantity * 2;

        pkt[pkt_sz++] = (start_addr >> 8) & 0xFF;
        pkt[pkt_sz++] = start_addr & 0xFF;
        pkt[pkt_sz++] = (quantity >> 8) & 0xFF;
        pkt[pkt_sz++] = quantity & 0xFF;
        pkt[pkt_sz++] = byte_cnt;

        for (int i = 0; i < quantity; ++i)
        {
            value = (uint16_t)random_range(0, 65535);
            pkt[pkt_sz++] = (value >> 8) & 0xFF;
            pkt[pkt_sz++] = value & 0xFF;
        }
        break;
    case 15:
        start_addr = (uint16_t)random_range(0, 65535);
        quantity = (uint16_t)random_range(1, 1968);
        byte_cnt = (quantity + 7) / 8;

        pkt[pkt_sz++] = (start_addr >> 8) & 0xFF;
        pkt[pkt_sz++] = start_addr & 0xFF;
        pkt[pkt_sz++] = (quantity >> 8) & 0xFF;
        pkt[pkt_sz++] = quantity & 0xFF;
        pkt[pkt_sz++] = byte_cnt;

        for (int i = 0; i < byte_cnt; ++i)
        {
            value = (uint16_t)random_range(0, 255); // 코일 데이터는 8비트씩 처리
            pkt[pkt_sz++] = value;
        }
        break;

    default:
        printf("Unsupported function code: %d\n", fcode);
        return 0;
    }

    length = pkt_sz - 6;
    pkt[4] = (length >> 8) & 0xFF;
    pkt[5] = length & 0xFF;

    return pkt_sz;
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("socket() error\n");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) < 0)
    {
        printf("inet_pton() error\n");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("connected error\n");
        return -1;
    }

    int pkt_num = 100;
    if (argc > 1)
    {
        pkt_num = atoi(argv[1]);
    }

    uint8_t pkt[MAX_PACKET_SIZE];
    int function_codes[] = {1, 2, 3, 4, 5, 6, 10, 15};
    int num_function_codes = sizeof(function_codes) / sizeof(function_codes[0]);

    srand(time(NULL));

    for (int i = 0; i < pkt_num; i++)
    {
        int fcode = function_codes[random_range(0, num_function_codes - 1)];
        int pkt_sz = generate_modbus_request(pkt, fcode);

        if (pkt_sz > 0)
        {
            printf("Packet %d (Function code %d): ", i + 1, fcode);
            for (int j = 0; j < pkt_sz; j++)
            {
                printf("%02X ", pkt[j]);
            }
            printf("\n");

            // send
            if (write(sockfd, pkt, pkt_sz) == -1)
            {
                perror("write error\n");
            }

            sleep(3);
        }
    }

    close(sockfd);

    return 0;
}
