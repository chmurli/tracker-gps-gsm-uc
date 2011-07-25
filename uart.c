/****************************************************************************
 
 patrz plik >> uart.h

****************************************************************************/


#include <avr/io.h>
#include "uart.h"
#include "config.h"



////////////////////////////////////////////////
//	GSM
//	USART0
////////////////////////////////////////////////


/*  Inicjalizacja portu szeregowego do GSM */
void uartGsmInit(void)
{
	
	/* prędkość transmisji */
	UBRR0H = (uint8_t)(UART_MAKE_UBRR(DEF_BAUD_GSM)>>8);
    UBRR0L = (uint8_t)(UART_MAKE_UBRR(DEF_BAUD_GSM));
    	
	/* Format ramki: asynchroniczny, 8 bitów danych, 1 bit stopu, brak bitu parzystości */
	UCSR0C = 1<<URSEL0 | 1<<UCSZ01 | 1<<UCSZ00; 
	
	/* Włączenie TXD, RXD i przerwanie do RXD */
	UCSR0B = 1<<RXEN0 | 1<<TXEN0 | 1<<RXCIE0; 
}



int uartGsmPut(char znak)
{
	// Oczekiwanie aż bufor nadajnika jest pusty
	while(!(1<<UDRE0 & UCSR0A)) {}
	UDR0 = znak; 
	return 0;
}

int uartGsmGet(void)
{
	char znak; 
	// Oczekiwanie na pojawienie się danej
	while(!(1<<RXC0 & UCSR0A)) {}
	znak = UDR0; 
	return znak;
}


// tworzymy ręcznie strukturę FILE (zamiast alokować dynamicznie) 
FILE fileUartGsm = FDEV_SETUP_STREAM(uartGsmPut, uartGsmGet, _FDEV_SETUP_RW);
// wskaźnik do stworzonej struktury
FILE* fUartGsm = (void*)&fileUartGsm;




////////////////////////////////////////////////
//	GPS
//	USART1
////////////////////////////////////////////////


/*  Inicjalizacja portu szeregowego do GSM */
void uartGpsInit(void)
{
	
	/* prędkość transmisji */
	UBRR1H = (uint8_t)(UART_MAKE_UBRR(DEF_BAUD_GPS)>>8);
    UBRR1L = (uint8_t)(UART_MAKE_UBRR(DEF_BAUD_GPS));
    	
	/* Format ramki: asynchroniczny, 8 bitów danych, 1 bit stopu, brak bitu parzystości */
	UCSR1C = 1<<URSEL1 | 1<<UCSZ11 | 1<<UCSZ10; 
	
	/* Włączenie RXD i przerwanie do RXD */
	//UCSR1B = 1<<RXEN1 | 1<<RXCIE1;
	
	// DO TESTOW 
	/* Włączenie TXD, RXD i przerwanie do RXD */
	UCSR1B = 1<<RXEN1 | 1<<TXEN1 | 1<<RXCIE1; 
}

	
int uartGpsPut(char znak)
{
	// Oczekiwanie aż bufor nadajnika jest pusty
	while(!(1<<UDRE1 & UCSR1A)) {}
	UDR1 = znak; 
	return 0;
}


int uartGpsGet(void)
{
	char znak; 
	// Oczekiwanie na pojawienie się danej
	while(!(1<<RXC1 & UCSR1A)) {}
	znak = UDR1; 
	return znak;
}



// NIEWYKORZSTYWANE OBECNIE W DOCELOWEJ APLIKACJI
// OBECNIE TYLKO DO TESTÓW
// tworzymy ręcznie strukturę FILE (zamiast alokować dynamicznie) 
FILE fileUartGps = FDEV_SETUP_STREAM(uartGpsPut, uartGpsGet, _FDEV_SETUP_RW);
// wskaźnik do stworzonej struktury
FILE* fUartGps = (void*)&fileUartGps;








