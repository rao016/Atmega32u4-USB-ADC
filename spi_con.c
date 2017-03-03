#include "spi_con.h"

/*
 * Function: SPI_init
 * Date Modified: 06/01/2016
 * Date Created: 12/23/2016
 * Input: None
 * Output: None
 *
 * Description: This function initializes the SPI. */
void SPI_init(void) {
    /* Set MOSI and SCK output, all others input */
    DDRB = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<DD_SS);
    PORTB |= SS;
    /* Enable SPI, Master, set clock rate fck/2 */
    SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPHA);
    SPSR = (1<<SPI2X);
}

/*
 * Function: SPI_txrx
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: cData
 * Output: uint8_t
 *
 * Description: This function reads and writes to the SPI bus, returning any data received. */
uint8_t SPI_txrx(uint8_t cData) {
    PORTB &= ~_BV(SS);
    SPDR = cData;
    while(!(SPSR & (1<<SPIF)));
    PORTB |= _BV(SS);
    return SPDR;
}
