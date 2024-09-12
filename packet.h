#ifndef PACKET_H
#define PACKET_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "modbus.h"

#define HEADER_LEN 7

typedef struct MODBUS_TCP_PDU
{
    uint8_t f_code;
    uint8_t data[];
} ModbusPDU;

typedef struct MODBUS_TCP_PACKET
{
    uint16_t t_id;
    uint16_t p_id;
    uint16_t len;
    uint8_t u_id;
    ModbusPDU *pdu;

} ModbusPkt;

uint16_t getPacketLength(const uint8_t *msg);

int decodeModbusPacket(const uint8_t *msg, ModbusPkt *pkt);
int encodeModbusPacket(const ModbusPkt *pkt, uint8_t *msg);

int handleRequest(ModbusMappingTbl *tbl, const ModbusPDU *pdu, uint8_t *response); // server
int handleResponse(const ModbusPDU *pdu);                                          // client

int createResponsePacket(const ModbusPkt *req, ModbusPkt *res, uint8_t *result,
                         uint8_t res_len, uint8_t *msg);
int createRequestPacket(const ModbusPkt *req, uint8_t *msg);

#endif