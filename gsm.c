/****************************************************************************
 
 patrz plik >> gsm.h

****************************************************************************/



#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <util/delay.h>
#include <stdio.h>
#include "makra.h"
#include "config.h"
#include "uart.h"
#include "at.h"
#include "gsm.h"


#ifndef NULL
	#define NULL 0
#endif 



void gsmInit(void) 
{
	
	//DDR(GSM_RTS_PORT) |= 1<<GSM_RTS;			// ustawienie pinu do RTS GSM jako wyjście z uC
	//gsmRtsOff();							// stan 0
	
	/* CTS i RI jako wejścia
	 * NIE podciągnięte do zasilania!
	 * domyślnie tak już są ustawione
	 */

	
	DDR(GSM_PWRKEY_PORT) |= 1<<GSM_PWRKEY;		// PWRKEY GSM jako wyjście z uC
	PORT(GSM_PWRKEY_PORT) &= ~(1<<GSM_PWRKEY);	// stan 0, włączenie modułu
	
}



void gsmConnectToBts(void) 
{
	
	// nawiązanie poprawnnej komunikacji z GSM (autobounding) i wyłączenie echa 
	while( gsmSendAtCmdWaitResp("ATE0", "OK", 1, 3) != AT_RESP_OK )
		; // VOID
		
	// czekamy na połączenie z siecią
	while( gsmSendAtCmdWaitResp("AT+CREG?", "+CREG: 0,1", 1, 3) != AT_RESP_OK )
		; // VOID
	
}


//////////////////////////////////////////////////////////////////
// proste funkcje (inline głównie)


inline void gsmTurnOn(void) 
{
	//if(!gsmFlags.status) {
	//	PORT(GSM_PWRKEY_PORT) &= ~(1<<GSM_PWRKEY);
		
	//}
}

inline void gsmTurnOff(void) 
{
	PORT(GSM_RTS_PORT) &= ~(1<<GSM_RTS);
}



// RTS
inline void gsmRtsOn(void) 		{	PORT(GSM_RTS_PORT) |= 1<<GSM_RTS;		}
inline void gsmRtsOff(void) 	{	PORT(GSM_RTS_PORT) &= ~(1<<GSM_RTS);	}

// DTR
inline void gsmDtrOn(void) 		{	PORT(GSM_DTR_PORT) |= 1<<GSM_DTR;		}
inline void gsmDtrOff(void) 	{	PORT(GSM_DTR_PORT) &= ~(1<<GSM_DTR);	}

// PWRKEY
inline void gsmPwrkeyOn(void) 	{	PORT(GSM_PWRKEY_PORT) |= 1<<GSM_PWRKEY;		}
inline void gsmPwrkeyOff(void) 	{	PORT(GSM_PWRKEY_PORT) &= ~(1<<GSM_PWRKEY);	}

// RESET
inline void gsmResetOn(void) 	{	PORT(GSM_RESET_PORT) |= 1<<GSM_RESET;		}
inline void gsmResetOff(void) 	{	PORT(GSM_RESET_PORT) &= ~(1<<GSM_RESET);	}



inline uint8_t gsmCtsCheck(void) 
{
	return (PIN(GSM_CTS_PORT) & 1<<GSM_CTS) ? 1:0;
}



inline void gsmRxcieEnable(void) 	{	UCSR0B |= 1<<RXCIE0; 		}
inline void gsmRxcieDisable(void)	{	UCSR0B &= ~(1<<RXCIE0); 	}





//////////////////////////////////////////////////////////////////
// wysyłanie komend AT



uint8_t gsmSendAtCmdWaitResp(
			uint8_t const *AT_cmd_string,
			uint8_t const *response_string,
			uint8_t no_of_attempts,
			uint8_t wait_delay) 
{
	
	enum AT_RESP_ENUM ret_val = AT_RESP_ERR_NO_RESP;
	uint8_t wait_delay_tmp;
	
	// czyszczenie bufora
	gsmBuff[0]='\0';
	gsmBuffIndex=0;
	
	
	while(no_of_attempts){
		
		// wyślij
		fprintf_P(fUartGsm, PSTR("%s\r\n"), AT_cmd_string);
		
		wait_delay_tmp=wait_delay;
		wait_delay+=5;
		while(wait_delay_tmp--) 
			_delay_ms(100);
		
		// czy coś otrzymano?
		if(gsmBuff[0] != '\0') {
			// sprawdź czy oczekiwana odpowiedź zawarta jest w otrzymanej
			if( strstr(gsmBuff, response_string) != NULL ) {
				ret_val = AT_RESP_OK;
				break;  // odpowiedź jest poprawna => koniec
			} else {
				ret_val = AT_RESP_ERR_DIF_RESP;
				break;  // niepoprawna => też koniec
			}
		} 
		
		--no_of_attempts;
	}
	
	return ret_val;

}



void gsmSendAtCmdNoResp(uint8_t const *AT_cmd_string, uint8_t wait_delay) 
{
	
	// czyszczenie bufora
	gsmBuff[0]='\0';
	gsmBuffIndex=0;
	
	// wyślij
	fprintf_P(fUartGsm, PSTR("%s\r\n"), AT_cmd_string);
	while(wait_delay--) 
		_delay_ms(100);
}



inline void gsmWaitForCmdPrompt(void) 
{
	while( strstr(gsmBuff, "> ") == NULL ) 
		; // VOID
}




//////////////////////////////////////////////////////////////////
// GPRS


uint8_t gsmGprsInit(uint8_t const *apn)
{
	
	
	// przygotuj komendę:  AT+CSTT="internet"
	strcpy(gsmCmdBuff, "AT+CSTT=\"");
	strcat(gsmCmdBuff, apn);
	strcat(gsmCmdBuff, "\""); // koniec
			
	while( gsmSendAtCmdWaitResp(gsmCmdBuff, "OK", 1, 5) != AT_RESP_OK )
		; // VOID
	while( gsmSendAtCmdWaitResp("AT+CIICR", "OK", 1, 15) != AT_RESP_OK )
		; // VOID
		
	// pobierz adres IP
	gsmSendAtCmdNoResp("AT+CIFSR", 3);
	
}


uint8_t gsmGprsOpenSocket(
			uint8_t const *socket_type,
			uint8_t const *remote_addr,
			uint8_t const *remote_port)
{
	
	// przygotuj komendę:  AT+CIPSTART="TCP","chmurli.dyndns.info","9999"
	strcpy(gsmCmdBuff, "AT+CIPSTART=\"");
	strcat(gsmCmdBuff, socket_type);				// dodaj typ gniazda (TCP/UDP)
	strcat(gsmCmdBuff, "\",\"");
	strcat(gsmCmdBuff, remote_addr);				// dodaj zdalny adres IP
	strcat(gsmCmdBuff, "\",\"");
	strcat(gsmCmdBuff, remote_port);				// dodaj numer zdalnego portu
	strcat(gsmCmdBuff, "\""); 						// koniec

	return gsmSendAtCmdWaitResp(gsmCmdBuff, "CONNECTED OK", 1, 15);
	

	
}



uint8_t gsmGprsSendData(uint8_t const *str_data)
{
	// czekaj 200ms, powinien być już znak '>'
	gsmSendAtCmdNoResp("AT+CIPSEND", 2);
	//gsmWaitForCmdPrompt();
			
	// wyślij stringa kończąc go Ctrl+Z (0x1a)	
	fprintf_P(fUartGsm, PSTR("%s" CTRL_Z), str_data);
	
	return 0;
}




//////////////////////////////////////////////////////////////////
// przerwanie do odbioru danych


SIGNAL(USART0_RXC_vect)
{
	
	// pobieramy znak z rejestru od przerwania
	uint8_t znak;
	znak=UDR0;


	gsmBuff[gsmBuffIndex++]=znak;
	gsmBuff[gsmBuffIndex]='\0';

	
}













