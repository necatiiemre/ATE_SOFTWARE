#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>

/* typedef struct __attribute__((packed)) {
    uint8_t port1 : 8;
    #ifdef big_endian
        uint8_t port2 : 1; 
        uint8_t port3 : 1; 
        uint8_t port4 : 5;
    #else
        uint8_t port4 : 5;
        uint8_t port3 : 1; 
        uint8_t port2 : 1;
    #endif 
} Status; */

typedef struct __attribute__((packed)) {
    // uint16_t p0;
    uint8_t port1;
    uint8_t port2; 
    uint8_t port3; 
    uint8_t port4;
} Status;

uint8_t bit_extract(const uint8_t *buffer, uint32_t bit_offset, uint32_t bit_count) {
    uint8_t mask = (1 << bit_count) - 1;
    int shift = 7 - (bit_offset % 8) - (bit_count - 1);
    
    return (*buffer >> shift) & mask;
}

// ------Before Bit Extractor------
// Before extraction, struct status, p1: 00000062
// Before extraction, struct status, p2: 00000075
// Before extraction, struct status, p3: 00000072
// AS1: System Initialization Started.
// Before extraction, struct status, p4: 00000061
// [SYS] Starting Process...
// Before extraction, struct status, st: 00000000000000000000000062757261

// ------Before Bit Extractor------
// Before extraction, struct status, p1: 00000061
// Before extraction, struct status, p2: 00000072
// Before extraction, struct status, p3: 00000075
// Before extraction, struct status, p4: 00000062
// Before extraction, struct status, st: 00000000000000000000000062757261

int main() {
    //                    a r u b
    //                    b u r a
    // uint32_t ports = 0x62757261;
    uint8_t ports[4] = {0x62, 0x75, 0x72, 0x61};

    Status st = {0};
    memcpy(&st, &ports, sizeof(uint32_t));
    // st.p0 = 1;
    printf("\n\n------Before Bit Extractor------\n");
    // printf("Before extraction, struct status, p1: %d\n", st.p0);
    printf("Before extraction, struct status, p1: %c\n", st.port1);
    printf("Before extraction, struct status, p2: %c\n", st.port2);
    printf("Before extraction, struct status, p3: %c\n", st.port3);
    printf("Before extraction, struct status, p4: %c\n", st.port4);

    printf("Before extraction, struct status, st: %032X\n", st);
    
// Before extraction, struct status, p1: 000000AB
// Before extraction, struct status, p2: 000000CD
// Before extraction, struct status, p3: 000000EF
// Before extraction, struct status, p4: 00000000
// Before extraction, struct status, st: 000000000000000000000000ABCDEF00

// Before extraction, struct status, p1: 00000000
// Before extraction, struct status, p2: 000000EF
// Before extraction, struct status, p3: 000000CD
// Before extraction, struct status, p4: 000000AB
// Before extraction, struct status, st: 000000000000000000000000ABCDEF00

/*     printf("\n\n------Bit Extractor------\n");
    uint8_t p1 = bit_extract((const uint8_t *)&st, 0, 1);
    uint8_t p2 = bit_extract((const uint8_t *)&st, 1, 1);
    uint8_t p3 = bit_extract((const uint8_t *)&st, 2, 1);
    uint8_t p4 = bit_extract((const uint8_t *)&st, 3, 5);
    printf("After extraction, struct status, p1: %05X\n", p1);
    printf("After extraction, struct status, p2: %05X\n", p2);
    printf("After extraction, struct status, p3: %05X\n", p3);
    printf("After extraction, struct status, p4: %05X\n", p4); */

    return 0;
}