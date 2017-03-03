#ifndef SPI_CON_H
#define SPI_CON_H

#include <avr/io.h>

#define MOSI PB2
#define MISO PB3
#define SCK PB1
#define SS PB0

#define DD_MOSI DDB2
#define DD_MISO DDB3
#define DD_SCK DDB1
#define DD_SS DDB0

void SPI_init(void);
uint8_t SPI_txrx(uint8_t cData);

#endif
