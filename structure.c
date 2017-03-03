#include "structure.h"

dSet *queue = NULL;

uint8_t add(unsigned int n, uint8_t ar, uint8_t ps, uint8_t cH, uint8_t cL, uint8_t c) {
    dSet *temp = (dSet*) malloc(sizeof(dSet));
    dSet *end = queue;
    
    if (temp != NULL && isNumOk(n) && isADCRateOk(ar) && isPrescaleOk(ps) && isNumOk(c)) {
        //Number of Samples
        temp->num = n;
        //ADC rate
        temp->adcRate = ar;
        //Prescale
        temp->prescale = ps;
        //Compare High
        temp->compareH = cH;
        //Compare Low
        temp->compareL = cL;
        //Channels
        //TODO: make this a 16bit to account for the other two channels
        temp->channels = c;
        //Pointer to next
        temp->next = NULL;
        
        if (queue == NULL) queue = temp;
        else {
            while(end->next != NULL) end = end->next;
            end->next = temp;
        }
        return 1;
    }
    else return NULL;
}

uint8_t isNumOk(unsigned int n) {
    return (n > 0) ? 1 : NULL;
}

//TODO: Replace this function with error checking on the sample rate
uint8_t isADCRateOk(uint8_t ar) {
    return ((ar & 0xF0) == 0) ? 1 : NULL;
}

//TODO: Replace this function with error checking on the sample rate
uint8_t isPrescaleOk(uint8_t ps) {
    return (ps <= 8) ? 1 : NULL;
}

uint8_t rm() {
    dSet *temp = queue;
    queue = queue->next;
    free(temp);
    return (queue == NULL) ? NULL : 1;
}

uint8_t dec() {
    if (queue == NULL) return NULL;
    else if (--(queue->num) == 0) return rm() ? 2 : NULL;
    else return 1;
}

dSet* findSet(uint8_t n) {
    uint8_t i;
    dSet *temp = queue;
    
    for(i = 0; i < n && temp != NULL; i++) temp = temp->next;
    
    return temp;
}

dSet* qryDSet(unsigned int *n, uint8_t *a, uint8_t *ps, uint8_t *ch, uint8_t *cl, uint8_t *c) {
    dSet *temp = findSet(*a);
    
    if (temp != NULL) {
        *n = temp->num;
        *a = temp->adcRate;
        *ps = temp->prescale;
        *ch = temp->compareH;
        *cl = temp->compareL;
        *c = temp->channels;
    }
    return temp;
}

uint8_t determinePrescale(float rate) {
    if (rate > (float) CLK_PS_1) return PS_1;
    else if (rate > (float) CLK_PS_8) return PS_8;
    else if (rate > (float) CLK_PS_64) return PS_64;
    else if (rate > (float) CLK_PS_256) return PS_256;
    else return PS_1024;
}

unsigned int determineCounter(uint8_t prescale, float rate) {
    unsigned int ps = 1;
    
    switch (prescale) {
        case PS_8:
            ps = 8;
            break;
        case PS_64:
            ps = 64;
            break;
        case PS_256:
            ps = 256;
            break;
        case PS_1024:
            ps = 1024;
            break;
        default:
            break;
    }
    
    return ((unsigned int) ((float) F_CPU/((float) ps*rate)));
}
