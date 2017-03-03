#ifndef ADCLIB_H
#define ADCLIB_H

#include "spi_con.h"
#include <util/delay.h>
#include <avr/io.h>
#include "command.h"

/*
 * ADC COMMANDS
 */
#define READ_ADC1 0x12
#define READ_REG 0x20
#define WRITE_REG 0x40
#define START_ADC1 0x08
#define STOP_ADC1 0x0A
#define START_ADC2 0x0C
#define STOP_ADC2 0x0E
#define READ_ADC2 0x14
#define ADC1_SYSTEM_CAL 0x16
#define ADC1_FULL_CAL 0x17
#define ADC1_OFFSET_CAL 0x19
#define ADC2_SYSTEM_CAL 0x1B
#define ADC2_FULL_CAL 0x1C
#define ADC2_OFFSET_CAL 0x1E

/*
 * ADC REGISTERS
 */
// 9.6.1: Device Identification (Read Only)
#define DIR_REG 0
// 9.6.2: Power Register
#define POWER_REG 1
// 9.6.3: Interface Register
#define INTERFACE_REG 2
// 9.6.4: Mode0 Register
#define MODE0_REG 3
// 9.6.5: Mode1 Register
#define MODE1_REG 4
// 9.6.6: Mode2 Register
#define MODE2_REG 5
// 9.6.7: Input Multiplexer Register
#define INMUX_REG 6
// 9.6.8: Offset Calibration Registers
#define OFFSET_REG1 7
#define OFFSET_REG2 8
#define OFFSET_REG3 9
// 9.6.9: Full-Scale Calibration Registers
#define FULLCAL_REG1 10
#define FULLCAL_REG2 11
#define FULLCAL_REG3 12
// 9.6.10: IDAC Multiplexer Register
#define IDACMUX_REG 13
// 9.6.11: IDAC Magnitude Register
#define IDACMAG_REG 14
// 9.6.12: Reference Multiplexer Register
#define REF_REG 15
// 9.6.13: TDACP Output Register
#define TDACP_REG 16
// 9.6.14: TDACN Output Register
#define TDACN_REG 17
// 9.6.15: GPIO Connection Register
#define GPIO_REG 18
// 9.6.16: GPIO Direction Register
#define GPIO_DIR_REG 19
// 9.6.17: GPIO Data Register
#define GPIO_DATA_REG 20
// 9.6.18: ADC2 Configuration Register
#define ADC2_CONFIG_REG 21
// 9.6.19: ADC2 Input Mux Register
#define ADC2_INMUX_REG 22
// 9.6.20: ADC2 Offset Calibration Register
#define ADC2_OFFCAL_REG1 23
#define ADC2_OFFCAL_REG2 24
// 9.6.21: ADC2 Full Calibration Register
#define ADC2_FULLCAL_REG1 25
#define ADC2_FULLCAL_REG2 26

#define DATA_RATE_38400 15

/*
 * ADC 1 DATA RATES FOR COMPARISON
 */
#define RATE_2_5 2.5
#define RATE_5 5
#define RATE_10 10
#define RATE_16_6 16.666
#define RATE_20 20
#define RATE_50 50
#define RATE_60 60
#define RATE_100 100
#define RATE_400 400
#define RATE_1200 1200
#define RATE_2400 2400
#define RATE_4800 4800
#define RATE_7200 7200
#define RATE_14400 14400
#define RATE_19200 19200
#define RATE_38400 38400
#define FF 0.9

/*
 * ADC 1 DATA RATES
 */
#define DATA_RATE_2_5 0
#define DATA_RATE_5 1
#define DATA_RATE_10 2
#define DATA_RATE_16_6 3
#define DATA_RATE_20 4
#define DATA_RATE_50 5
#define DATA_RATE_60 6
#define DATA_RATE_100 7
#define DATA_RATE_400 8
#define DATA_RATE_1200 9
#define DATA_RATE_2400 10
#define DATA_RATE_4800 11
#define DATA_RATE_7200 12
#define DATA_RATE_14400 13
#define DATA_RATE_19200 14
#define DATA_RATE_38400 15

/*
 * ADC INITIALIZATION
 */
#define INTERFACE_REG_INIT 0b00001000
#define MODE0_REG_INIT 0b01000000
//#define MODE0_REG_INIT 0
#define MODE1_REG_INIT 0b10010000
#define MODE2_REG_INIT 0b10001111
#define INMUX_REG_INIT 10
//#define INMUX_REG_INIT 0b00001010
//Gain = 0, Data Rate = 100SPS
#define CAL_MODE2_REG 0b00000111

#define PWDN_PIN PB5
#define START_PIN PB6
#define DRDY_PIN PB7

extern uint8_t mode2, inmux;

uint8_t changeRate(int rate);
void changeChannel(uint8_t channel);
void changeSampleRate(uint8_t rate);
void initReg(uint8_t rate, uint8_t channel);
void readADC1(uint8_t *value);
void initGPIO();
void turnOnOff(uint8_t val);
void startStop(uint8_t val);
void writeReg(uint8_t reg, uint8_t value);
uint8_t readReg(uint8_t reg);
void initADC();
uint8_t determineADCRate(float rate);

#endif
