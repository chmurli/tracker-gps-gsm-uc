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
 * 		tylko czerwona 	- urządzenie działa, ale brak ustalonej pozycji z GPS
 * 		tylko zielona	- pozycja z AGPS
 * 		obie zapalone	- pozycja z DGPS
 */ 
inline void diodeGreenOn(void) 		{	PORT(DIODE_GREEN_PORT) &= ~(1<<DIODE_GREEN);	}
inline void diodeGreenOff(void) 	{	PORT(DIODE_GREEN_PORT) |= 1<<DIODE_GREEN;		}
inline void diodeGreenToggle(void) 	{	PORT(DIODE_GREEN_PORT) ^= 1<<DIODE_GREEN;		}
inline void diodeRedOn(void) 		{	PORT(DIODE_RED_PORT)   &= ~(1<<DIODE_RED);		}
inline void diodeRedOff(void) 		{	PORT(DIODE_RED_PORT)   |= 1<<DIODE_RED;			}
inline void diodeRedToggle(void) 	{	PORT(DIODE_RED_PORT)   ^= 1<<DIODE_RED;			}



// instrukcja wysyłana gdy GPS nie ustalił pozycji i minał zadany czaw (NO_FIX_HARD_SEND_DATA)
uint8_t *INSTRUCTION_NOFIX="!nofix\r\n";


/* zmianna sumuje przebyty dystans
 * dodajemy prędkość w m/s
 */
float g_distance=0.0f;

// licznik "wymuszonego" wysyłania pozycji np. gdy urządzenie się nie porusza
uint8_t g_fix_cnt=0;

// licznik do wysyłania informacji o braku ustalenia pozycji z GPS
uint8_t g_no_fix_cnt=0;




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
	
		
		/* gdy dane z GPS są już odczytane i gotowe do wysłania dalej;
		 * domyślnie powinny być gotowe co sekundę jeżeli nie było błędów w transmisji
		 */ 
		if(gpsDataRdy()) {
			gpsDisable();


			// gdy pozycja z GPS została ustalona poprawnie
			if(gps.status[0]=='A') {
				
				// zapal odpowiednie diody sygnalizacyjne
				diodeGreenOn();
				if(gps.mode[0]=='D')
					diodeRedOn();
				else
					diodeRedOff();
				
				
				/* oblicz przebytą drogę
				 * funkcja powinna się wykonywać co 1 sekundę więc sumujemy aktualną prędkość w m/s
				 * z grubsza odpowiada to przebytej drodze w metrach
				 */
				g_distance+=gpsSpeedInMPS();
				
				// zwiększ licznik (domyślnie co 1s)
				++g_fix_cnt;
				
				// licznik do braku ustalenia pozycji możemy wyzerować
				g_no_fix_cnt=0;
				
				
				/* wysyłaj dane o pozycji na serwer jeśli:
				 * 		- od ostatniej wysłanej pozycji przebyliśmy zadaną drogę (w metrach)
				 * 		- upłynął max. czas od wysłania ostatniej pozycji
				 */
				if( (g_distance >= DISTANCE_SEND_DATA) || (g_fix_cnt >= FIX_HARD_SEND_DATA) ){
					
					// zeruj drogę i licznik
					g_distance=0.0f;
					g_fix_cnt=0;
					
				
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
					
				}
				
			
				
				
				
			
			// gdy pozycja z GPS nie została ustalona poprawnie
			} else {
				diodeRedOn();
				diodeGreenOff();
				
				g_distance=0.0f;
				g_fix_cnt=0;
				
				/* zwiększ licznik i jeżeli upłynął zadany czas wyślij informację do serwera o braku
				 * wyznaczenia pozycji z GPS
				 */
				if( ++g_no_fix_cnt >= NO_FIX_HARD_SEND_DATA ){
					
					g_no_fix_cnt=0;
					
					// otwórz połączenie
					gsmGprsOpenSocket(INTERNET_PROTOCOL, INTERNET_HOST, INTERNET_PORT);
					
					// wyślij wiadomość o błędnej pozycji
					gsmGprsSendData(INSTRUCTION_NOFIX);
				
				}
			}
			
			
			
			
			
			gpsClearDataRdy();
			gpsEnable();
		}
		
		
		
		// czekaj
		//uint8_t i;
		//for(i=0; i<100; ++i) {
		//	_delay_ms(100);
			//diodeGreenToggle();
		//}
		//diodeGreenOn();



  
	}
}









