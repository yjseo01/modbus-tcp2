#include "packet.h"

uint16_t getPacketLength(const uint8_t *msg)
{
    // printf("len: %u\n", msg[4] << 8 | msg[5]);
    uint16_t len = msg[4] << 8 | msg[5];

    return len;
}

int decodeModbusPacket(const uint8_t *msg, ModbusPkt *pkt)
{
    uint16_t tid = (msg[0] << 8) | msg[1];
    uint16_t pid = (msg[2] << 8) | msg[3];
    uint16_t len = (msg[4] << 8) | msg[5];
    uint8_t uid = msg[6];
    uint8_t fcode = msg[7];

    size_t data_sz = len - sizeof(fcode);

    pkt->t_id = tid;
    pkt->p_id = pid;
    pkt->len = len;
    pkt->u_id = uid;
    pkt->pdu->f_code = fcode;

    memcpy(pkt->pdu->data, msg + 8, data_sz);

    printf("Modbus/TCP\n");
    printf("  Transaction ID: %u\n", pkt->t_id);
    printf("  Protocol ID: %u\n", pkt->p_id);
    printf("  Length: %u\n", pkt->len);
    printf("  Unit ID: %u\n", pkt->u_id);

    return 0;
}

int handleRequest(ModbusMappingTbl *tbl, const ModbusPDU *pdu, uint8_t *response)
{
    uint16_t starting_addr;
    uint16_t quantity;
    uint16_t value;
    uint8_t bytecount;
    uint8_t *data;

    int res_len = 0;

    printf("Modbus\n");
    printf("  Function Code: %u\n", pdu->f_code);
    printf("  Data: \n");

    /* function code 별로 요청 처리 */
    switch (pdu->f_code)
    {
    case RC:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        quantity = (pdu->data[2] << 8) | pdu->data[3];
        res_len = readCoils(tbl, starting_addr, quantity, response);

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    quantity of coils: %u\n", quantity);

        break;
    case RDI:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        quantity = (pdu->data[2] << 8) | pdu->data[3];
        res_len = readDiscreteInputs(tbl, starting_addr, quantity, response);

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    quantity of coils: %u\n", quantity);
        break;
    case RMR:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        quantity = (pdu->data[2] << 8) | pdu->data[3];
        res_len = readHoldingRegisters(tbl, starting_addr, quantity,
                                       response);

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    quantity of registers: %u\n", quantity);
        break;
    case RIR:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        quantity = (pdu->data[2] << 8) | pdu->data[3];
        res_len = readInputRegisters(tbl, starting_addr, quantity,
                                     response);

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    quantity of registers: %u\n", quantity);
        break;
    case WSC:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        value = (pdu->data[2] << 8) | pdu->data[3];
        res_len = writeSingleCoil(tbl, starting_addr, value, response);

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    Value: 0x%04X\n", value);
        break;
    case WSR:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        value = (pdu->data[2] << 8) | pdu->data[3];
        res_len = writeSingleRegister(tbl, starting_addr, value, response);

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    Value: 0x%04X\n", value);
        break;
    case WMR:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        quantity = (pdu->data[2] << 8) | pdu->data[3];
        bytecount = pdu->data[4];
        data = (uint8_t *)malloc(bytecount);
        memcpy(data, pdu->data + 5, bytecount);

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    Quantity of registers: %u\n", quantity);
        printf("    Byte Count: %u\n", bytecount);
        printf("    Data:\n");
        for (size_t i = 0; i < bytecount / 2; i++)
        {
            printf("        Registers %zu: %04X\n", i,
                   (data[i * 2] << 8) | data[i * 2 + 1]);
        }
        printf("\n");

        res_len = writeMultipleRegisters(tbl, starting_addr, quantity, data, response);

        if (data != NULL)
        {
            free(data);
            data = NULL;
        }

        break;
    case WMC:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        quantity = (pdu->data[2] << 8) | pdu->data[3];
        bytecount = pdu->data[4];
        data = (uint8_t *)malloc((bytecount + 1) * sizeof(uint8_t));
        memcpy(data, pdu->data + 5, bytecount * sizeof(uint8_t));

        res_len = writeMultipleCoils(tbl, starting_addr, quantity, data, response);

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    Quantity of coils: %u\n", quantity);
        printf("    Byte Count: %u\n", bytecount);
        printf("    Data: \n");
        for (size_t byte_index = 0; byte_index < bytecount; byte_index++)
        {
            for (int bit_position = 7; bit_position >= 0; bit_position--)
            {
                printf("      Bits %zu: %02X\n",
                       byte_index * 8 + (7 - bit_position),
                       (data[byte_index] >> bit_position) & 1);
            }
        }
        printf("\n");

        if (data != NULL)
        {
            free(data);
            data = NULL;
        }
        break;
    default:
        fprintf(stderr, "[Error] unexpected function code\n");
        break;
    }

    return res_len;
}

int handleResponse(const ModbusPDU *pdu)
{
    uint16_t starting_addr;
    uint16_t quantity;
    uint16_t value;
    uint8_t bytecount;
    uint8_t *data;

    printf("Modbus\n");
    printf("  Function Code: %u\n", pdu->f_code);
    printf("  Data:\n");

    /* function code 별로 요청 결과 출력 */
    switch (pdu->f_code)
    {
    case RC:
    case RDI:
        bytecount = pdu->data[0];
        data = (uint8_t *)malloc((bytecount + 1) * sizeof(uint8_t));

        memcpy(data, pdu->data + 1, bytecount * sizeof(uint8_t));
        printf("    Byte count: %u\n", bytecount);
        printf("    Data: \n");
        for (size_t byte_index = 0; byte_index < bytecount; byte_index++)
        {
            for (int bit_position = 7; bit_position >= 0; bit_position--)
            {
                printf("      Bits %zu: %u\n", byte_index * 8 + (7 - bit_position),
                       (data[byte_index] >> bit_position) & 1);
            }
        }
        printf("\n");

        if (data != NULL)
        {
            free(data);
            data = NULL;
        }
        break;
    case RMR:
    case RIR:
        bytecount = pdu->data[0];
        data = (uint8_t *)malloc(bytecount * sizeof(uint8_t));

        memcpy(data, pdu->data + 1, bytecount * sizeof(uint8_t));
        printf("    Byte count: %u\n", bytecount);
        printf("    Data:\n");
        for (size_t i = 0; i < bytecount / 2; i++)
        {
            uint16_t reg_value = (data[i * 2] << 8) | data[i * 2 + 1];
            printf("      Registers %zu: %u\n", i, reg_value);
        }
        printf("\n");

        if (data != NULL)
        {
            free(data);
            data = NULL;
        }
        break;
    case WSC:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        value = (pdu->data[2] << 8) | pdu->data[3];

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    Value: 0x%04X\n", value);
        break;
    case WSR:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        value = (pdu->data[2] << 8) | pdu->data[3];

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    Value: 0x%04X\n", value);
        break;
    case WMR:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        quantity = (pdu->data[2] << 8) | pdu->data[3];

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    quantity of registers: %u\n", quantity);
        break;
    case WMC:
        starting_addr = (pdu->data[0] << 8) | pdu->data[1];
        quantity = (pdu->data[2] << 8) | pdu->data[3];

        printf("    Starting Address: 0x%04X\n", starting_addr);
        printf("    quantity of coils: %u\n", quantity);
        break;
    default:
        fprintf(stderr, "[Error] unexpected function code\n");
        break;
    }

    return 1;
}

int encodeModbusPacket(const ModbusPkt *pkt, uint8_t *msg)
{
    // Header
    msg[0] = (pkt->t_id >> 8) & 0xFF;
    msg[1] = pkt->t_id & 0xFF;
    msg[2] = (pkt->p_id >> 8) & 0xFF;
    msg[3] = pkt->p_id & 0xFF;
    msg[4] = (pkt->len >> 8) & 0xFF;
    msg[5] = pkt->len & 0xFF;
    msg[6] = pkt->u_id;
    msg[7] = pkt->pdu->f_code;

    memset(msg + 8, 0, pkt->len - 1);
    memcpy(msg + 8, pkt->pdu->data, pkt->len - 1);

    return 1;
}

int createResponsePacket(const ModbusPkt *req, ModbusPkt *res, uint8_t *result,
                         uint8_t res_len, uint8_t *msg)
{
    if (req == NULL || res == NULL || result == NULL || msg == NULL)
    {
        return -1; // Error: Null pointer passed
    }

    res->t_id = req->t_id;
    res->p_id = req->p_id;
    res->len = res_len + 2; // Length of data + 1 for function code
    res->u_id = req->u_id;

    // Copy data to PDU
    memset(res->pdu->data, 0, res_len);
    res->pdu->f_code = req->pdu->f_code;
    memcpy(res->pdu->data, result, res_len);

    msg[0] = (res->t_id >> 8) & 0xFF;
    msg[1] = res->t_id & 0xFF;
    msg[2] = (res->p_id >> 8) & 0xFF;
    msg[3] = res->p_id & 0xFF;
    msg[4] = (res->len >> 8) & 0xFF;
    msg[5] = res->len & 0xFF;
    msg[6] = res->u_id;
    msg[7] = res->pdu->f_code;

    memset(msg + 8, 0, res->len);
    memcpy(msg + 8, result, res_len);

    // if (encodeModbusPacket(res, msg) < 0)
    //{
    //     return -1; // Error: encodeModbusPacket failed
    // }

    return 1;
}

int createRequestPacket(const ModbusPkt *req, uint8_t *msg)
{
    // 파일 읽어와서 req에 할당

    if (encodeModbusPacket(req, msg) != 1)
        return -1;

    return 1;
}
