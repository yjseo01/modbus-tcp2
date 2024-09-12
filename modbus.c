/* modbus.c */

#include "modbus.h"

#define BITS_PER_BYTE 8

// mapping table 초기화
int initMappingTable(ModbusMappingTbl *tbl, int nb_bits, int nb_input_bits,
                     int nb_registers, int nb_input_registers)
{
    if (tbl == NULL)
    {
        perror("[Error] Failed to allocate memory for Modbus_Mapping_Table");
        exit(-1);
    }

    tbl->nb_bits = nb_bits;
    tbl->nb_input_bits = nb_input_bits;
    tbl->nb_registers = nb_registers;
    tbl->nb_input_registers = nb_input_registers;

    tbl->tab_bits = (uint8_t *)malloc(nb_bits * sizeof(uint8_t));
    tbl->tab_input_bits = (uint8_t *)malloc(nb_input_bits * sizeof(uint8_t));
    tbl->tab_input_registers =
        (uint16_t *)malloc(nb_input_registers * sizeof(uint16_t));
    tbl->tab_registers = (uint16_t *)malloc(nb_registers * sizeof(uint16_t));

    if (tbl->tab_bits == NULL || tbl->tab_input_bits == NULL ||
        tbl->tab_input_registers == NULL || tbl->tab_registers == NULL)
    {
        perror("[Error] Failed to allocate memory for Modbus_Mapping_Table");
        free(tbl->tab_bits);
        free(tbl->tab_input_bits);
        free(tbl->tab_input_registers);
        free(tbl->tab_registers);
        return -1;
    }

    memset(tbl->tab_bits, 0, nb_bits * sizeof(uint8_t));
    memset(tbl->tab_input_bits, 0, nb_input_bits * sizeof(uint8_t));
    memset(tbl->tab_registers, 0, nb_registers * sizeof(uint16_t));
    memset(tbl->tab_input_registers, 0, nb_input_registers * sizeof(uint16_t));

    return 1;
}

void resetMappingTable(ModbusMappingTbl *tbl)
{
    memset(tbl->tab_bits, 0, tbl->nb_bits * sizeof(uint8_t));
    memset(tbl->tab_input_bits, 0, tbl->nb_input_bits * sizeof(uint8_t));
    memset(tbl->tab_registers, 0, tbl->nb_registers * sizeof(uint16_t));
    memset(tbl->tab_input_registers, 0, tbl->nb_input_registers * sizeof(uint16_t));
}

void freeMappingTable(ModbusMappingTbl *tbl)
{
    free(tbl->tab_bits);
    free(tbl->tab_input_bits);
    free(tbl->tab_input_registers);
    free(tbl->tab_registers);
}

/* fuction code 처리 함수 */
// 읽기
int readCoils(ModbusMappingTbl *tbl, uint16_t start_addr, uint16_t quantity,
              uint8_t *response)
{
    if (start_addr + quantity > tbl->nb_bits)
    {
        fprintf(stderr, "[Error] requested address out of bound\n");
        return -1;
    }

    uint8_t byte_count = (quantity + 7) / 8;
    response[0] = byte_count;
    memset(response + 1, 0, byte_count);

    for (int i = 0; i < quantity; i++)
    {
        int byte_index = (start_addr + i) / 8;
        int bit_index = (start_addr + i) % 8;

        if (tbl->tab_bits[byte_index] & (1 << bit_index))
        {
            response[1 + i / 8] |= (1 << (i % 8));
        }
    }

    return byte_count + 1;
}

int readDiscreteInputs(ModbusMappingTbl *tbl, uint16_t start_addr,
                       uint16_t quantity, uint8_t *response)
{
    if (start_addr + quantity > tbl->nb_bits)
    {
        fprintf(stderr, "[Error] requested address out of bound\n");
        return -1;
    }

    uint8_t byte_count = (quantity + 7) / 8;
    response[0] = byte_count;
    memset(response + 1, 0, byte_count);

    for (int i = 0; i < quantity; i++)
    {
        int byte_index = (start_addr + i) / 8;
        int bit_index = (start_addr + i) % 8;

        if (tbl->tab_bits[byte_index] & (1 << bit_index))
        {
            response[1 + i / 8] |= (1 << (i % 8));
        }
    }

    return byte_count + 1;
}

int readHoldingRegisters(ModbusMappingTbl *tbl, uint16_t start_addr,
                         uint16_t quantity, uint8_t *response)
{
    if (start_addr + quantity > tbl->nb_registers)
    {
        fprintf(stderr, "[Error] Requested address out of bound\n");
        return -1;
    }

    uint8_t byte_count = quantity * 2;
    response[0] = byte_count;

    for (int i = 0; i < quantity; i++)
    {
        uint16_t reg_value = tbl->tab_registers[start_addr + i];
        response[1 + i * 2] = (reg_value >> 8) & 0xFF;
        response[2 + i * 2] = reg_value & 0xFF;
    }

    return byte_count + 1;
}

int readInputRegisters(ModbusMappingTbl *tbl, uint16_t start_addr,
                       uint16_t quantity, uint8_t *response)
{
    if (start_addr + quantity > tbl->nb_input_registers)
    {
        fprintf(stderr, "[Error] Requested address out of bound\n");
        return -1;
    }

    uint8_t byte_count = quantity * 2;
    response[0] = byte_count;

    for (int i = 0; i < quantity; i++)
    {
        uint16_t reg_value = tbl->tab_input_registers[start_addr + i];
        response[1 + i * 2] = (reg_value >> 8) & 0xFF;
        response[2 + i * 2] = reg_value & 0xFF;
    }

    return byte_count + 1;
}

// 쓰기
int writeSingleCoil(ModbusMappingTbl *tbl, uint16_t start_addr, uint8_t bit, uint8_t *response)
{
    if (start_addr >= tbl->nb_bits)
    {
        fprintf(stderr, "[Error] requested address out of bound\n");
        return -1;
    }

    // Determine which byte and bit within that byte to modify
    int byte_index = start_addr / BITS_PER_BYTE;
    int bit_index = start_addr % BITS_PER_BYTE;

    // Set or clear the specific bit
    if (bit == 0xFF)
    { // Set the bit
        tbl->tab_bits[byte_index] |= (1 << bit_index);
    }
    else if (bit == 0x00)
    { // Clear the bit
        tbl->tab_bits[byte_index] &= ~(1 << bit_index);
    }
    else
    {
        fprintf(stderr, "[Error] invalid coil value\n");
        return -1;
    }

    // starting addr
    response[0] = (start_addr >> 8) & 0xFF;
    response[1] = start_addr & 0xFF;

    // value
    response[2] = bit;
    response[3] = 0x00;

    return 4;
}

int writeSingleRegister(ModbusMappingTbl *tbl, uint16_t start_addr,
                        uint16_t regi, uint8_t *response)
{
    if (start_addr >= tbl->nb_registers)
    {
        fprintf(stderr, "[Error] requested address out of bound\n");
        return -1;
    }

    tbl->tab_registers[start_addr] = regi;

    // starting addr
    response[0] = (start_addr >> 8) & 0xFF;
    response[1] = start_addr & 0xFF;

    // value
    response[2] = (regi >> 8) & 0xFF;
    response[3] = regi & 0xFF;

    return 4;
}

int writeMultipleRegisters(ModbusMappingTbl *tbl, uint16_t start_addr,
                           uint16_t quantity, uint8_t *regis, uint8_t *response)
{
    if (start_addr + quantity >= tbl->nb_registers)
    {
        fprintf(stderr, "[Error] requested address out of bound\n");
        return -1;
    }

    // starting addr
    response[0] = (start_addr >> 8) & 0xFF;
    response[1] = start_addr & 0xFF;
    // quantity
    response[2] = (quantity >> 8) & 0xFF;
    response[3] = quantity & 0xFF;

    for (uint16_t i = 0; i < quantity; i++)
    {
        tbl->tab_registers[start_addr + i] = (regis[i * 2] << 8) | regis[i * 2 + 1];
    }

    return 4;
}

int writeMultipleCoils(ModbusMappingTbl *tbl, uint16_t start_addr,
                       uint16_t quantity, uint8_t *bits, uint8_t *response)
{
    if (start_addr + quantity >
        tbl->nb_bits)
    {
        fprintf(stderr, "[Error] requested address out of bound\n");
        return -1;
    }

    // starting addr
    response[0] = (start_addr >> 8) & 0xFF;
    response[1] = start_addr & 0xFF;
    // quantity
    response[2] = (quantity >> 8) & 0xFF;
    response[3] = quantity & 0xFF;

    for (int i = 0; i < quantity; i++)
    {
        int byte_index = (start_addr + i) / BITS_PER_BYTE;
        int bit_index = (start_addr + i) % BITS_PER_BYTE;

        if (bits[i / BITS_PER_BYTE] & (1 << (i % BITS_PER_BYTE)))
        {
            tbl->tab_bits[byte_index] |= (1 << bit_index); // Set the bit
        }
        else
        {
            tbl->tab_bits[byte_index] &= ~(1 << bit_index); // Clear the bit
        }
    }

    return 4;
}