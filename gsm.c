/****************************************************************************
 
 patrz plik >> gsm.h

****************************************************************************/



#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

#include "makra.h"
#include "config.h"
#include "uart.h"
#include "gsm.h"
#include "at.h"


#ifndef NULL
	#define NULL 0
#endif 


//////////////////////////////////////////////////////////////////
// główne funkcje dla GSM

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
	while( gsmSendAtCmdWaitResp("ATE0", "OK", 1, 5) != AT_RESP_OK )
		; // VOID
		
	// czekamy na połączenie z siecią
	while( gsmSendAtCmdWaitResp("AT+CREG?", "+CREG: 0,1", 1, 5) != AT_RESP_OK )
		; // VOID
	
}




//////////////////////////////////////////////////////////////////
// wysyłanie komend AT



enum AT_RESP_ENUM gsmSendAtCmdWaitResp(
			char const *AT_cmd_string,
			char const *response_string,
			uint8_t no_of_attempts,
			uint8_t wait_max_delay) 
{
	
	enum AT_RESP_ENUM ret_val = AT_RESP_ERR_NO_RESP;
	uint8_t wait_max_delay_tmp;
	
	
	while(no_of_attempts){
		
		// czyszczenie bufora
		gsmRxBuff[0]='\0';
		gsmRxBuffIdx=0;
		
		
		// wyślij
		fprintf_P(fUartGsm, PSTR("%s\r\n"), AT_cmd_string);
		
		wait_max_delay_tmp=wait_max_delay+1; 	// dodajemy 1 bo będzie predekrementacja później
		wait_max_delay+=5;						// zwiększamy opóźnienie dla następnych prób


		// czekaj dopóki nie wykryje odbioru końca nadawania lub upłynie max. czas	
		while(!gsmIsRxFinished() && --wait_max_delay_tmp)
			_delay_ms(100);
				
			
		// gdy odebrano jakieś dane
		if(gsmIsRxFinished()){
			// sprawdź czy oczekiwana odpowiedź zawarta jest w otrzymanej
			if( strstr((void *)gsmRxBuff, response_string) != NULL )
				ret_val = AT_RESP_OK;			// odpowiedź jest poprawna
			else
				ret_val = AT_RESP_ERR_DIF_RESP; // niepoprawna
				
			break; // wyjdź z pętli while, przejdź do return
		} 
		
		--no_of_attempts;
	}
	
	return ret_val;

}



void gsmSendAtCmdNoResp(char const *AT_cmd_string, uint8_t wait_delay) 
{
	
	// czyszczenie bufora
	gsmRxBuff[0]='\0';
	gsmRxBuffIdx=0;
	
	// wyślij
	fprintf_P(fUartGsm, PSTR("%s\r\n"), AT_cmd_string);
	
	// czekaj zadanie opóźnienie na wykonanie komendy
	while(wait_delay--) 
		_delay_ms(100);
}


/* czekaj na "znak zachęty" do wprowadzania danych
 */
void gsmWaitForCmdPrompt(void) 
{
	while( strstr((void *)gsmRxBuff, "> ") == NULL ) 
		; // VOID
}



/* zwykle odpowiedź zwracana po komendach AT przez SIM900 ma postać: 
 * 		<CR><LF><response><CR><LF>
 * sprawdzamy wiec 2 ostatnie znaki(\r\n) i czy przed nimi są jeszcze jakieś inne
 */
enum RX_STATE_ENUM gsmIsRxFinished(void)
{
	enum RX_STATE_ENUM ret_val = RX_NOT_FINISHED;
		
	if(	(gsmRxBuffIdx >= 3) && 
		(gsmRxBuff[gsmRxBuffIdx-1] == '\n') && 
		(gsmRxBuff[gsmRxBuffIdx-2] == '\r') )
			ret_val = RX_FINISHED;
	
	return ret_val;
}





//////////////////////////////////////////////////////////////////
// GPRS


enum GPRS_INIT_ENUM gsmGprsInit(char const *apn)
{
	
	
	// przygotuj komendę, np. AT+CSTT="internet"
	strcpy(gsmCmdBuff, "AT+CSTT=\"");
	strcat(gsmCmdBuff, apn);
	strcat(gsmCmdBuff, "\""); // koniec
			
	while( gsmSendAtCmdWaitResp(gsmCmdBuff, "OK", 1, 8) != AT_RESP_OK )
		; // VOID
	while( gsmSendAtCmdWaitResp("AT+CIICR", "OK", 1, 15) != AT_RESP_OK )
		; // VOID
		
	// pobierz adres IP
	gsmSendAtCmdNoResp("AT+CIFSR", 4);
	
	// skopiuj adres IP do specjalnej zmiennej tablicowej
	strcpy((void *)gsm.ipAddress, (void *)gsmRxBuff);

	
	return GPRS_INIT_OK;
}


enum GPRS_SOCKET_ENUM gsmGprsOpenSocket(
			char const *socket_type,
			char const *remote_addr,
			char const *remote_port)
{
	
	// przygotuj komendę, np. AT+CIPSTART="TCP","chmurli.dyndns.info","9999"
	strcpy(gsmCmdBuff, "AT+CIPSTART=\"");
	strcat(gsmCmdBuff, socket_type);				// dodaj typ gniazda (TCP/UDP)
	strcat(gsmCmdBuff, "\",\"");
	strcat(gsmCmdBuff, remote_addr);				// dodaj zdalny adres IP
	strcat(gsmCmdBuff, "\",\"");
	strcat(gsmCmdBuff, remote_port);				// dodaj numer zdalnego portu
	strcat(gsmCmdBuff, "\""); 						// koniec


	/* wyślij i czekaj dłuższy czas
	 * UWAGA!
	 * po tej komendzie otrzymamy niemal od razu odpowiedź "OK", a chwilę potem komunikat "CONNECTED OK"
	 * nie możemy wieć pomylić komunikatów;
	 * musimy to wykryć lub czekać dłuższy czas na dalsze instrukcje
	 */
	gsmSendAtCmdNoResp(gsmCmdBuff, 25);


	// sprawdź czy połączenie zostało otwarte pomyślnie i zwróć wartość enum
	if( strstr((void *)gsmRxBuff, "CONNECTED OK") != NULL )
		return GPRS_SOCKET_OPEN;
	else
		return GPRS_SOCKET_NOT_OPEN;

	//return gsmSendAtCmdWaitResp(gsmCmdBuff, "CONNECTED OK", 1, 15);

}



uint8_t gsmGprsSendData(char const *str_data)
{
	// czekaj 200ms, powinien być już znak '>'
	gsmSendAtCmdNoResp("AT+CIPSEND", 2);
	//gsmWaitForCmdPrompt();
			
	// wyślij stringa kończąc go Ctrl+Z (0x1a)	
	fprintf_P(fUartGsm, PSTR("%s" CTRL_Z), str_data);
	
	return 0;
}




//////////////////////////////////////////////////////////////////
// proste funkcje inline


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
// przerwanie do odbioru danych


ISR(USART0_RXC_vect)
{
	
	// pobieramy znak z rejestru od przerwania
	char znak;
	znak=UDR0;

	gsmRxBuff[gsmRxBuffIdx++]=znak;
	gsmRxBuff[gsmRxBuffIdx]='\0';

	
	
}













