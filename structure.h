#ifndef structure_h
#define structure_h

#include <stdio.h>
#include <stdint.h>

//Atmega32u4 CPU frequency
#ifndef F_CPU
#define F_CPU 16000000
#endif

//Atmega32u4 timer rates with prescale
#define CLK_PS_1 F_CPU/65536
#define CLK_PS_8 F_CPU/524288
#define CLK_PS_64 F_CPU/4194304
#define CLK_PS_256 F_CPU/16777216

//Atmega32u4 prescale rate register values
#define PS_1 1
#define PS_8 2
#define PS_64 3
#define PS_256 4
#define PS_1024 5

typedef struct dataSet {
    unsigned int num;
    uint8_t adcRate, prescale, compareH, compareL, channels;
    struct dataSet *next;
} dSet;

extern dSet *queue;

uint8_t add(unsigned int n, uint8_t ar, uint8_t ps, uint8_t cH, uint8_t cL, uint8_t c);
uint8_t isNumOk(unsigned int n);
uint8_t isADCRateOk(uint8_t ar);
uint8_t isPrescaleOk(uint8_t ps);
uint8_t rm();
uint8_t dec();
dSet* findSet(uint8_t n);
dSet* qryDSet(unsigned int *n, uint8_t *a, uint8_t *ps, uint8_t *ch, uint8_t *cl, uint8_t *c);

#endif /* structure_h */
