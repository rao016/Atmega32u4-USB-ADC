#include "command.h"

uint8_t mystrcmp(uint8_t *a, uint8_t *b) {
    uint8_t n;
    
    for(n = 0; a[n] == b[n] && a[n] != NULL && n < 255; n++);
    
    if ((NULL == b[n]) && (a[n] == NULL || a[n] == '\n')) return NULL;
    else return 1;
}

unsigned int myatoi(uint8_t *a, uint8_t *valid) {
    uint8_t n;
    unsigned int i = 0;
    
    if (!isNumeric(a,0)) {
        *valid = NUM_INVALID;
        return 0;
    }
    
    if (a[0] < '0' || a[0] > '9') {
        *valid = 3;
        return i;
    }
    
    for(n = 0; a[n] >= '0' && a[n] <= '9'; n++) i = i*10 + a[n] - 48;
    
    if (i > 255) *valid = NUM_INT;
    else *valid = NUM_VALID;
    
    return i;
}

cmd findCommand(uint8_t* command) {
    toUpper(command);
    
    if (!mystrcmp((uint8_t*)command, (uint8_t*)RREG_CMD)) return CMD_RREG;
    else if (!mystrcmp((uint8_t*)command, (uint8_t*)ADD_CMD)) return CMD_ADD;
    else if (!mystrcmp((uint8_t*)command, (uint8_t*)RM_CMD)) return CMD_RM;
    else if (!mystrcmp((uint8_t*)command, (uint8_t*)STOP_CMD)) return CMD_STOP;
    else if (!mystrcmp((uint8_t*)command, (uint8_t*)START_CMD)) return CMD_START;
    else if (!mystrcmp((uint8_t*)command, (uint8_t*)QRY_CMD)) return CMD_QRY;
    else if (!mystrcmp((uint8_t*)command, (uint8_t*)READ_CMD)) return CMD_READ;
}

uint8_t crc8(uint8_t *p, uint8_t len) {
    uint16_t i, crc = 0x0;
    
    while (len--) {
        i = (crc ^ *p++) & 0xFF;
        crc = (crc_table[i] ^ (crc << 8)) & 0xFF;
    }
    
    return crc & 0xFF;
}

uint8_t lookForward(uint8_t *data, const uint8_t *charAllowed) {
    uint8_t i,j,allow;
    
    for(i = 0; data[i] != NULL; i++) {
        allow = 0;
        for(j = 0; charAllowed[j] != NULL && allow == 0; j++) {
            if (data[i] == charAllowed) allow = 1;
        }
        if (allow == 0) return 0;
    }
    return 1;
}

uint8_t isWhiteSpace(uint8_t c) {
    if (c == ' ' || c == '\n' || c == '\t') return 1;
    else return 0;
}

uint8_t isNumeric(uint8_t *d, uint8_t isFloat) {
    uint8_t f,i;
    
    if (d == NULL) return NULL;
    
    for(i = 0, f = 0; ((d[i] >= '0' && d[i] <= '9') || (d[i] == '.' && isFloat)) && f < 2; i++) {
        if (d[i] == '.') f++;
    }
    
    if (d[i] == NULL && i > 0) return 1;
    else return 0;
}

void toUpper(uint8_t *str) {
    uint8_t i;
    
    for (i = 0; str[i] != NULL; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') str[i] -= 'a' - 'A';
    }
}
