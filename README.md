# modbus-tcp2
modbus 라이브러리 없이 epoll 기반 modbus tcp server, client 구현

## 사용법
### client
input: [modbus packet]

예시) 

00 04 00 00 00 57 01 10 00 30 00 28 50 00 01 00 02 00 03 00 04 00 05 00 06 00 07 00 08 00 09 00 0A 00 0B 00 0C 00 0D 00 0E 00 0F 00 10 00 11 00 12 00 13 00 14 00 15 00 16 00 17 00 18 00 19 00 1A 00 1B 00 1C 00 1D 00 1E 00 1F 00 20 00 21 00 22 00 23 00 24 00 25 00 26 00 27 00 28

## 결과
### client
```
-------------- Response --------------
00 04 00 00 00 06 01 10 00 30
00 28

Modbus/TCP
  Transaction ID: 4
  Protocol ID: 0
  Length: 6
  Unit ID: 1
Modbus
  Function Code: 16
  Data:
    Starting Address: 0x0030
    quantity of registers: 40
```

### server
```
-------------- Request 1 --------------
Request from client fd 5

00 04 00 00 00 57 01 10 00 30
00 28 50 00 01 00 02 00 03 00
04 00 05 00 06 00 07 00 08 00
09 00 0A 00 0B 00 0C 00 0D 00
0E 00 0F 00 10 00 11 00 12 00
13 00 14 00 15 00 16 00 17 00
18 00 19 00 1A 00 1B 00 1C 00
1D 00 1E 00 1F 00 20 00 21 00
22 00 23 00 24 00 25 00 26 00
27 00 28
Modbus/TCP
  Transaction ID: 4
  Protocol ID: 0
  Length: 87
  Unit ID: 1
Modbus
  Function Code: 16
  Data:
    Starting Address: 0x0030
    Quantity of registers: 40
    Byte Count: 80
    Data:
        Registers 0: 0001
        Registers 1: 0002
        Registers 2: 0003
        Registers 3: 0004
        Registers 4: 0005
        Registers 5: 0006
        Registers 6: 0007
        Registers 7: 0008
        Registers 8: 0009
        Registers 9: 000A
        Registers 10: 000B
        Registers 11: 000C
        Registers 12: 000D
        Registers 13: 000E
        Registers 14: 000F
        Registers 15: 0010
        Registers 16: 0011
        Registers 17: 0012
        Registers 18: 0013
        Registers 19: 0014
        Registers 20: 0015
        Registers 21: 0016
        Registers 22: 0017
        Registers 23: 0018
        Registers 24: 0019
        Registers 25: 001A
        Registers 26: 001B
        Registers 27: 001C
        Registers 28: 001D
        Registers 29: 001E
        Registers 30: 001F
        Registers 31: 0020
        Registers 32: 0021
        Registers 33: 0022
        Registers 34: 0023
        Registers 35: 0024
        Registers 36: 0025
        Registers 37: 0026
        Registers 38: 0027
        Registers 39: 0028


Response
00 04 00 00 00 06 01 10 00 30
00 28
```
