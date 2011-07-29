/****************************************************************************
 
 File			:	main.c
 Version		:	0.01 (2011.04.11)
 Project		:	tracker GPS + GSM
 Author			:	Bartosz Chmura
 
-----------------------------------------------------------------------------
 
 Copyright		:	(c) 2011 by Bartosz Chmura
 License		:	GNU GPL v3
 
-----------------------------------------------------------------------------
 
 Hardware		:	uc:		ATMega162
					GPS:	FGPMMOPA4
					GSM:	SIM900
 
-----------------------------------------------------------------------------
 
 Description	:	N/A
 
-----------------------------------------------------------------------------
 
 Changelog		:	0.2 - initial version
 
-----------------------------------------------------------------------------
 
 ToDo			:	* none
					
 
****************************************************************************/


/**** PLIKI NAGŁÓWKOWE ****/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <stdio.h>

#include "config.h"
#include "makra.h"
#include "uart.h"
#include "gps.h"
#include "gsm.h"
#include "at.h"

#include <util/delay.h>


/* diody sygnalizacyjne
 * świeci sie:
 * 		obie zgaszone	- wyłączone
 * 		tylko czerwona 	- urządzenie działa, ale nie brak ustalonej pozycji z GPS
 * 		tylko zielona	- pozycja z AGPS
 * 		obie zapalone	- pozycja z DGPS
 * 
 */ 

inline void diodeGreenOn(void) 		{	PORT(DIODE_GREEN_PORT) &= ~(1<<DIODE_GREEN);	}
inline void diodeGreenOff(void) 	{	PORT(DIODE_GREEN_PORT) |= 1<<DIODE_GREEN;		}
inline void diodeGreenToggle(void) 	{	PORT(DIODE_GREEN_PORT) ^= 1<<DIODE_GREEN;		}
inline void diodeRedOn(void) 		{	PORT(DIODE_RED_PORT)   &= ~(1<<DIODE_RED);		}
inline void diodeRedOff(void) 		{	PORT(DIODE_RED_PORT)   |= 1<<DIODE_RED;			}
inline void diodeRedToggle(void) 	{	PORT(DIODE_RED_PORT)   ^= 1<<DIODE_RED;			}



uint8_t *INSTRUCTION_NOFIX="!nofix\r\n";





int main(void)
{
	
	/* Inicjalizacja */
	uartGpsInit();
	uartGsmInit();
	//gpsInit();
	//gsmInit();
	
	// wyjścia z uc
	DDR(DIODE_GREEN_PORT) |= 1<<DIODE_GREEN;
	DDR(DIODE_RED_PORT) |= 1<<DIODE_RED;
	

	diodeRedOn();
	diodeGreenOff();
	
	
	// włączenie przerwań
	sei();
	
	
	// połącz się z BTS i siecią GPRS
	gsmConnectToBts();
	gsmGprsInit(GPRS_APN);
	
	

	while(1) {
	
		
		// gdy dane z GPS są już odczytane i gotowe do wysłania dalej
		// domyślnie powinny być gotowe co sekundę jeżeli nie było błędów w transmisji
		if(gpsDataRdy()) {
			gpsDisable();
			
			
	
			// gdy pozycja z GPS została ustalona poprawnie
			if(gps.status[0]=='A') {
				
				diodeGreenOn();
				if(gps.mode[0]=='D')
					diodeRedOn();
				else
					diodeRedOff();
			
			
				// otwórz połączenie
				gsmGprsOpenSocket(INTERNET_PROTOCOL, INTERNET_HOST, INTERNET_PORT);
				
				
				/* przygotuj dane do wysłania:
				 * 		latitude,latitudeInd,longitude,longitudeInd,altitude,speed,satellites,pdop,mode
				 * 		5013.2225,N,01903.7918,E,172.3,0.16,4,1.21,D\r\n
				 */  
				strcpy(gsmCmdBuff, gps.latitude);		// strcpy - nastąpi "wyczyszczenie" poprzednich danych
				strcat(gsmCmdBuff, ",");				// dalej strcat
				strcat(gsmCmdBuff, gps.latitudeInd);
				strcat(gsmCmdBuff, ",");
				strcat(gsmCmdBuff, gps.longitude);
				strcat(gsmCmdBuff, ",");
				strcat(gsmCmdBuff, gps.longitudeInd);
				strcat(gsmCmdBuff, ",");
				strcat(gsmCmdBuff, gps.altitude);
				strcat(gsmCmdBuff, ",");
				strcat(gsmCmdBuff, gps.speed);
				strcat(gsmCmdBuff, ",");
				strcat(gsmCmdBuff, gps.satellites);
				strcat(gsmCmdBuff, ",");
				strcat(gsmCmdBuff, gps.pdop);
				strcat(gsmCmdBuff, ",");
				strcat(gsmCmdBuff, gps.mode);
				strcat(gsmCmdBuff, "\r\n"); 			// koniec
				
				
				// wyślij dane
				gsmGprsSendData(gsmCmdBuff);
				
				
			
			// gdy pozycja z GPS nie została ustalona poprawnie
			} else {
				diodeRedOn();
				diodeGreenOff();
				
				// otwórz połączenie
				gsmGprsOpenSocket(INTERNET_PROTOCOL, INTERNET_HOST, INTERNET_PORT);
					
				// wyślij wiadomość o błędnej pozycji
				gsmGprsSendData(INSTRUCTION_NOFIX);
			}
	
			
			
			
			gpsClearDataRdy();
			gpsEnable();
		}
		
		
		
		// czekaj
		uint8_t i;
		for(i=0; i<100; ++i) {
			_delay_ms(100);
			//diodeGreenToggle();
		}
		//diodeGreenOn();



  
	}
}









