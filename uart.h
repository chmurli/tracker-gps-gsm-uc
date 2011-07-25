/****************************************************************************
 
 uart.h - obsługa portu szeregowego do obsługi GPS i GSM

****************************************************************************/


#ifndef UART_H_INCLUDED
#define UART_H_INCLUDED

#include <stdio.h>


#define UART_MAKE_UBRR(baud) (F_CPU/(baud*16L)-1)



// GSM
void uartGpsInit(void);
int uartGsmPut(char znak);
int uartGsmGet(void);
extern FILE* fUartGsm;



// GPS
void uartGpsInit(void);
int uartGpsPut(char znak);
int uartGpsGet(void);
extern FILE* fUartGps;




#endif // UART_H_INCLUDED
