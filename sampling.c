#include "sampling.h"

uint8_t cur_channel, read, buf1_end = 0, temp_test = 0;
startS ss = STOP;
dSamp *dataQ = NULL;
uint8_t buf1[255];

void nextChannel() {
    uint8_t n, temp;
    
    switch (ss) {
        case START:
            n = findNext((cur_channel = HIGHEST_CHANNEL));
            setRate(queue->adcRate, queue->prescale, queue->compareH, queue->compareL);
            read = 0;
            break;
        case GO:
            if ((n = findNext(cur_channel)) <= cur_channel) {
                if ((temp = dec()) == NULL) interruptEnableDisable(0);
                else if (temp == 2) setRate(queue->adcRate, queue->prescale, queue->compareH, queue->compareL);
            }
            break;
        case YIELD:
            if ((n = findNext(HIGHEST_CHANNEL)) <= cur_channel) {
                temp = dec();
                interruptEnableDisable(0);
            }
            break;
        default:
            break;
    }
    
    //Set channel
    if (cur_channel != n) changeChannel(cur_channel = n);
}

uint8_t findNext(uint8_t cur) {
    uint8_t n;
    
    for (n = cur+1; (((uint8_t) (1 << n) & (uint8_t) queue->channels) == 0) && (n <= HIGHEST_CHANNEL); n++);
    if (n == HIGHEST_CHANNEL + 1) {
        for (n = 0; (((uint8_t) (1 << n) & (uint8_t) queue->channels) == 0) && (n <= HIGHEST_CHANNEL); n++);
    }
    return n;
}

//TODO: Think about removing this function
uint8_t addData() {
    dSamp *temp = (dSamp*) malloc(sizeof(dSamp));
    dSamp *end = dataQ;
    
    if (temp != NULL) {
        temp->end = 0;
        temp->lock = 0;
        temp->next = NULL;
        
        if (dataQ == NULL) dataQ = temp;
        else {
            while(end->next != NULL) end = end->next;
            end->next = temp;
        }
        return 1;
    }
    else return NULL;
}

startS start() {
    if (ss == STOP & queue != NULL) {
        ss = START;
        nextChannel();
        SPI_txrx(START_ADC1);
        return START;
    }
    return ss;
}

startS stop() {
    return (ss == STOP) ? STOP : (ss = YIELD);
}

void setRate(uint8_t adcRate, uint8_t prescale, uint8_t compareH, uint8_t compareL) {
    changeSampleRate(adcRate);
    
    //External pin toggle
    //DDRB |= (1 << DDB5);
    //TCCR1A |= (1 << COM1A0);
    
    TCCR1B |= (1 << WGM12) + prescale;
    
    // set compare value for interrupt
    OCR1AH = compareH;
    OCR1AL = compareL;
    TCNT1 = 0;
    
    if (ss != GO) interruptEnableDisable(1);
}

void interruptEnableDisable(uint8_t ed) {
    if(ed) {
        //Pin interrupt
        PCICR |= (1 << PCIE0);
        PCMSK0 |= (1 << PCINT7);
        //Timer interrupt
        TIMSK1 |= (1 << OCIE1A);
        ss = GO;
    }
    else {
        ss = STOP;
        PCICR &= (0 << PCIE0);
        PCMSK0 &= (0 << PCINT7);
        TIMSK1 &= (0 << OCIE1A);
    }
}

//TODO: make sure this isnt going before the interrupt pin
ISR(TIMER1_COMPA_vect, ISR_BLOCK) {
    read |= 1;
}

ISR(PCINT0_vect, ISR_BLOCK) {
    read |= 2;
    SPI_txrx(STOP_ADC1);
}

unsigned int readData() {
    uint8_t *value;
    
   if (queue != NULL && ss != STOP) {
        if (read == 3) {
            cli();
            value = &(buf1[buf1_end]);
            if (buf1_end < 248) buf1_end += 4;
            readADC1(value);
            nextChannel();
            read = 0;
            sei();
            if (ss == GO) SPI_txrx(START_ADC1);
        }
        return (queue != NULL) ? queue->num : NULL;
    }
    else return NULL;
}
