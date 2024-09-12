#ifndef MODBUS_H
#define MODBUS_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* function code */
#define RC 0x01  // Read Coils
#define RDI 0x02 // Read Discrete Inputs
#define RMR 0x03 // Read Multiple Registers
#define RIR 0x04 // Read Input Registers

#define WSC 0x05 // Write Single Coil
#define WSR 0x06 // Write Single Register
#define WMR 0x10 // Write Multiple Registers
#define WMC 0x0F // Write Multiple Coils

typedef struct MODBUS_CLIENT_INFO
{
    int fd;
    uint8_t u_id;
} ModbusCliInfo;

typedef struct MODBUS_MAPPING_TABLE
{
    int nb_bits;
    int nb_input_bits;
    int nb_registers;
    int nb_input_registers;

    uint8_t *tab_bits;
    uint8_t *tab_input_bits;
    uint16_t *tab_input_registers;
    uint16_t *tab_registers;
} ModbusMappingTbl;

int initMappingTable(ModbusMappingTbl *tbl, int nb_bits, int nb_input_bits,
                     int nb_registers, int nb_input_registers);
void resetMappingTable(ModbusMappingTbl *tbl);
void freeMappingTable(ModbusMappingTbl *tbl);

/* function code 처리 함수 */
// 읽기
int readCoils(ModbusMappingTbl *tbl, uint16_t start_addr, uint16_t quantity,
              uint8_t *response); // 0x01
int readDiscreteInputs(ModbusMappingTbl *tbl, uint16_t start_addr,
                       uint16_t quantity, uint8_t *response); // 0x02
int readHoldingRegisters(ModbusMappingTbl *tbl, uint16_t start_addr,
                         uint16_t quantity, uint8_t *response); // 0x03
int readInputRegisters(ModbusMappingTbl *tbl, uint16_t start_addr,
                       uint16_t quantity, uint8_t *response); // 0x04
// 쓰기
int writeSingleCoil(ModbusMappingTbl *tbl, uint16_t start_addr,
                    uint8_t bit, uint8_t *response); // 0x05
int writeSingleRegister(ModbusMappingTbl *tbl, uint16_t start_addr,
                        uint16_t regis, uint8_t *response); // 0x06
int writeMultipleRegisters(ModbusMappingTbl *tbl, uint16_t start_addr,
                           uint16_t quantity, uint8_t *regi, uint8_t *response); // 0x10
int writeMultipleCoils(ModbusMappingTbl *tbl, uint16_t start_addr,
                       uint16_t quantity, uint8_t *bits, uint8_t *response); // 0x0F

#endif
