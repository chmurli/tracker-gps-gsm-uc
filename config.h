/****************************************************************************
 
 config.h - plik konfiguracyjny; definicje sprzętu etc.

****************************************************************************/


#ifndef __CONFIG_H_INCLUDED
#define __CONFIG_H_INCLUDED


#ifndef F_CPU
  #define F_CPU 8000000UL
#endif 



// konfiguracja połączenia do serwera aplikacji
#define INTERNET_PROTOCOL	"TCP"
#define INTERNET_HOST		"chmurli.dyndns.info"
#define INTERNET_PORT		"9999"

// konfiguracja połączenia do sieci GPRS
#define GPRS_APN			"internet"


/* po przebyciu jakiego dystansu w metrach mamy wysyłać dane na serwer
 * jeżeli prędkość w m/s będzie większa niż zadana wartość dane będą wysyłane co sekundę
 */
#define DISTANCE_SEND_DATA		15

/* czas w sekundach co jaki wymusić wysłanie danch z pozycją do serwera niezależnie od prędkości
 * jeżeli urządzenie mobilne się nie porusza dane będą wysyłane rzadko np. co 5 minut
 */
#define FIX_HARD_SEND_DATA		60

/* czas w sekundach co jaki wysyłać informację o niepoprawnym ustaleniu pozycji
 */
#define NO_FIX_HARD_SEND_DATA	30





/* GPS - definicje wyprowadzeń
 * używany USART1 więc RXD i TXD nie trzeba konfigurować
 * 
 * 	RXD 	- PB2 / RXD1
 * 	TXD 	- PB3 / TXD1
 * 	RESET 	- PB1			reset odbiornika
 */
#define GPS_PORT B
#define GPS_RXD 2
#define GPS_TXR 3
#define GPS_RESET_PORT B
#define GPS_RESET 1


/* diody sygnalizacyjne o statusie GPS
 * 		PB0 - zielona  		- pozycja wyliczona
 * 		PB1 - czerwona  	- pozycja niewyliczona/błędna
 */
#define DIODE_GREEN_PORT B
#define DIODE_GREEN 0
#define DIODE_RED_PORT B
#define DIODE_RED 1




/* GSM - definicje wyprowadzeń
 * używany USART0 więc RXD i TXD nie trzeba konfigurować
 * 
 * 	RXD 	- PD0 / RXD0
 * 	TXD 	- PD1 / TXD0
 * 	RTS 	- PD 			(RTS	- request to send 			WY)
 * 	CTS 	- PD 			(CTS	- clear to send 			WE)
 * 	DSR 	- PD 			(DSR	- data set ready 			WE)
 * 	DCD 	- PD 			(DCD 	- data carrier detected 	WE)
 * 	DTR 	- PD 			(DTR 	- data terminal ready		WY)
 * 							(wybudzanie modułu z trybu uśpienia)
 * 	RI 		- PD2 			(RI  	- ring indicator 			WE)
 * 							(detekcja przychodzących rozmów lub SMS'ów)
 * 	PWRKEY	- PD5			załączenie/wyłącznie modułu GSM (1s)
 * 	RESET	- PD5			reset modułu GSM (200ms)
 * 	STATUS	- PD5			1 - włączony
 * 							0 - logged off from base station in a power mode procedure
 */
#define GSM_PORT D
#define GSM_RXD 0
#define GSM_TXD 1

#define GSM_RTS_PORT D
#define GSM_RTS 2
#define GSM_CTS_PORT D
#define GSM_CTS 3
#define GSM_DSR_PORT D
#define GSM_DSR 3
#define GSM_DCD_PORT D
#define GSM_DCD 3
#define GSM_DTR_PORT D
#define GSM_DTR 3
#define GSM_RI_PORT D
#define GSM_RI 4

#define GSM_PWRKEY_PORT D
#define GSM_PWRKEY 5
#define GSM_RESET_PORT D
#define GSM_RESET 5
#define GSM_STATUS_PORT D
#define GSM_STATUS 5








/* 
 * 9600
 * 19200
 * 38400
 * 57600
 * 115200
 */ 


// prędkość portu szeregowego do GPS
#define DEF_BAUD_GPS 9600

// prędkość portu szeregowego do GSM
#define DEF_BAUD_GSM 19200









#endif // __CONFIG_H_INCLUDED
