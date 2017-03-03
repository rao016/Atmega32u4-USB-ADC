#ifndef sampling_h
#define sampling_h

#include <inttypes.h>
#include <avr/interrupt.h>
#include "adcLib.h"
#include "structure.h"

#define HIGHEST_CHANNEL 8
#define BUFFER1 1
#define BUFFER2 2

typedef enum startStop {
    START,
    STOP,
    YIELD,
    GO
}startS;

typedef struct data_sampled {
    //Enough for 63 samples
    uint8_t data[255];
    uint8_t end, lock;
    struct data_sampled *next;
} dSamp;

extern uint8_t cur_channel, read, startSamp, buf1_end, temp_test;
extern dSamp *dataQ;
extern uint8_t buf1[255];
extern startS ss;

void nextChannel();
uint8_t findNext(uint8_t cur);
uint8_t addData();
startS start();
startS stop();
void setRate(uint8_t adcRate, uint8_t prescale, uint8_t compareH, uint8_t compareL);
void interruptEnableDisable(uint8_t ed);
unsigned int readData();
uint8_t switchBuffer();

#endif
