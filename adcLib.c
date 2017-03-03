#include "adcLib.h"

uint8_t mode2, inmux;

/*
 * Function: changeChannel
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: channel
 * Output: None
 *
 * Description: This function changes the channel on the ADC to the input channel. */
void changeChannel (uint8_t channel) {
    inmux &= 0x0F;
    inmux += channel*16;
    writeReg(INMUX_REG, inmux);
}

/*
 * Function: changeSampleRate
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: rate
 * Output: None
 *
 * Description: This function sets the ADC to the sample rate given by the input rate. */
void changeSampleRate (uint8_t rate) {
    mode2 &= 0xF0;
    mode2 += 15;
    writeReg(MODE2_REG, mode2);
}

/*
 * Function: initReg
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: rate, channel
 * Output: None
 *
 * Description: This function initializes the ADC with the given rate and channel. */
void initReg(uint8_t rate, uint8_t channel) {
    inmux = INMUX_REG_INIT;
    mode2 = MODE2_REG_INIT;
    
    writeReg(INTERFACE_REG, INTERFACE_REG_INIT);
    //writeReg(MODE0_REG, MODE0_REG_INIT);
    writeReg(MODE1_REG, MODE1_REG_INIT);
    
    changeSampleRate(rate);
    changeChannel(channel);
}

/*
 * Function: readADC1
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: None
 * Output: uint8_t
 *
 * Description: This function reads the ADC value and sends it to the function addVal (edisonCtrl) returning what addVal returns. */
void readADC1(uint8_t *value) {
    uint8_t temp = 1, i = 0;
    
    while (i++ < 2 && temp != 0) {
        SPI_txrx(READ_ADC1);
    
        value[0] = SPI_txrx(0);
        value[1] = SPI_txrx(0);
        value[2] = SPI_txrx(0);
        value[3] = SPI_txrx(0);
        value[4] = SPI_txrx(0);
        
        temp = crc8(value,5);
   }
}

/*
 * Function: initGPIO
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: None
 * Output: None
 *
 * Description: This function initializes the GPIO pins for powerdown/reset, start, and data-ready. */
void initGPIO() {
    //DDRD |= (1 << PWDN_PIN) | (1 << START_PIN);
    DDRB &= (0 << DRDY_PIN);
    //PORTD |= (1 << PWDN_PIN);
    //PORTD &= (0 << START_PIN);
}

/*
 * Function: turnOnOff
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: val
 * Output: None
 *
 * Description: This function toggles the powerdown/reset pin on the ADC dependent on the input variable val. */
void turnOnOff(uint8_t val) {
    (val) ? (PORTB |= _BV(PWDN_PIN)) : (PORTB &= ~_BV(PWDN_PIN));
}

/*
 * Function: startStop
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: val
 * Output: None
 *
 * Description: This function toggles the start pin on the ADC dependent on the input variable val. */
void startStop(uint8_t val) {
    (val) ? (PORTB |= _BV(START_PIN)) : (PORTB &= ~_BV(START_PIN));
}

/*
 * Function: writeReg
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: reg, value
 * Output: None
 *
 * Description: This function writes the ADC register (reg) with the data from the variable (value). */
void writeReg(uint8_t reg, uint8_t value) {
    while (value != readReg(reg)) {
        SPI_txrx(WRITE_REG + reg);
        SPI_txrx(0);
        SPI_txrx(value);
    }
}

/*
 * Function: readReg
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: reg
 * Output: uint8_t
 *
 * Description: This function reads from the ADC register (reg) and returns the value. */
uint8_t readReg(uint8_t reg) {
    SPI_txrx(READ_REG + reg);
    SPI_txrx(0);
    return SPI_txrx(0);
}

/*
 * Function: initADC
 * Date Modified: 06/01/2016
 * Date Created: 06/01/2016
 * Input: None
 * Output: None
 *
 * Description: This function initializes the ADC. */
void initADC() {
    initGPIO();
    turnOnOff(1);
    startStop(0);
    SPI_init();
    _delay_ms(10);
    initReg(15,0);
}

uint8_t determineADCRate(float rate) {
    if (rate < (float) RATE_100) {
        if (rate < (float) RATE_16_6) {
            if (rate < (float) RATE_5) return (rate < (float) RATE_2_5) ? DATA_RATE_2_5 : DATA_RATE_5;
            else return (rate < (float) RATE_10) ? DATA_RATE_10 : DATA_RATE_16_6;
        }
        else {
            if (rate < (float) RATE_50) return (rate < (float) RATE_20) ? DATA_RATE_20 : DATA_RATE_50;
            else return (rate < (float) RATE_60) ? DATA_RATE_60 : DATA_RATE_100;
        }
    }
    else {
        if (rate < (float) RATE_4800) {
            if (rate < (float) RATE_1200) return (rate < (float) RATE_400) ? DATA_RATE_400 : DATA_RATE_1200;
            else return (rate < (float) RATE_2400) ? DATA_RATE_2400 : DATA_RATE_4800;
        }
        else {
            if (rate < (float) RATE_14400) return (rate < (float) RATE_7200) ? DATA_RATE_7200 : DATA_RATE_14400;
            else return (rate < (float) RATE_19200) ? DATA_RATE_19200 : DATA_RATE_38400;
        }
    }
}
