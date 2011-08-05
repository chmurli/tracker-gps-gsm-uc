/****************************************************************************

 File			:	gps.h
 Version		:	1.2 (2011.08.04)
 
 Author			:	Bartosz Chmura
 Copyright		:	(c) 2011 by Bartosz Chmura
 License		:	GNU GPL v3

-----------------------------------------------------------------------------
 
 Description	:	biblioteka do obsługi GPS
					obsługiwane układy:
						* FGPMMOPA4; chipseat MTK MT3318
					
					możliwości:
						* obsługiwane ramki
							* GGA
							* RMC
							* GSA
						* sprawdza sumę kontrolną
						* przelicza prędkość z węzły/h na km/h lub m/s
							* zwraca wynik jako liczbę float lub string  
						 
 
-----------------------------------------------------------------------------
 
 Changelog		:	* 1.0 	- wersja stabilna
					* 1.1 	- dodano przeliczniki prędkości na km/h i m/s	
					* 1.2 	- dodano opcje konfiguracyjne; włączenie/wyłączenie:
								- operacji na liczbach zmiennoprzecinkowych
								- użycia funkcji floatToString
								- sprawdzania sumy kontrolnej
							- drobne poprawki

 
-----------------------------------------------------------------------------
 
 ToDo			:	* 
					
 
****************************************************************************/


#ifndef __GPS_H_INCLUDED
#define __GPS_H_INCLUDED

#include "config.h"

#define GPS_LIB_VERSION 012	// library version X.YY (e.g. 1.00)


/* jeżeli zdefiniujemy - będzie można użyć funkcji do operacji na liczbach zmiennoprzecinkowych
 * np. zamiana prędkości na liczbę float i zmiana jednostki prędkości
 * potrzebne dla funkcji:
 		* gpsSpeedInKnotsPH
		* gpsSpeedInKmPH
		* gpsSpeedInMPS
 * UWAGA!!!
 * może powodować znaczne zwiększenie objętości programu!
 */
#define GPS_USE_FLOAT



/* jeżeli zdefiniujemy - możemy używać funkcji floatToString
 * przeważnie musimy też zdefiniować GPS_USE_FLOAT gdy chcemu używać tej funkcji
 * UWAGA!!!
 * może powodować znaczne zwiększenie objętości programu!
 */
#define GPS_USE_FLOAT_TO_STRING



/* jeżeli zdefiniujemy - będzie sprawdzana suma kontrolna każdej ramki
 * wyłączenie sprawdzania zmniejszy nieznacznie objętość programu i zwiększy jego szybkość
 */
#define GPS_VERIFY_CHECKSUM



// zabezpieczenie
#ifdef GPS_USE_FLOAT_TO_STRING
	#ifndef GPS_USE_FLOAT
		#warning "Zdefiniowana GPS_USE_FLOAT_TO_STRING, a nie zdefiniowano GPS_USE_FLOAT. Czy jesteś pewien?"
	#endif // GPS_USE_FLOAT
#endif // GPS_USE_FLOAT_TO_STRING



// maksymalna długość znaków w ramce NMEA
#define MAX_LEN_NMEA 82



// inicjacja odbiornika GPS; ustawienie pinu reset
extern inline void gpsInit(void);


/* zatrzymuje odbieranie danych z GPS
 * GPS dalej działa i wysyła dane, ale przerwanie które je odbiera jest wyłączone
 */
extern inline void gpsDisable(void);


/* wznawia odbieranie danych z GPS po użyciu gpsDisable()
 */
extern inline void gpsEnable(void);


// reset odbiornika GPS
extern inline void gpsReset(void);


/* sprawdza czy wszystkie potrzebne dane z GPS są gotowe
 * 1 - dane gotowe
 * 0 - dane niegotowe
 */
extern inline uint8_t gpsDataRdy(void);


/* gdy pobierzemy dane z struktury GPS i wyślemy np. przez GSM musimy wyzerować gotowość do nadania danych
 */
extern inline void gpsClearDataRdy(void);



#ifdef GPS_USE_FLOAT

/* mała ściągawka
 * 
 * węzły/h		km/h		m/s
 * ---------------------------------
 * 1			1.852		0.5
 * 2			3.704		1
 * 10			18.52		5
 * 20			37.04		10
 * 30			55.56		15
 * 40			74.08		20
 * 50			92.6		25
 * 60			111.12		30
 */


	/* zwraca prędkość w węzły/h jako liczbę float
	 */
	float gpsSpeedInKnotsPH();
	
	
	/* zwraca prędkość w km/h jako liczbę float
	 */
	float gpsSpeedInKmPH();
	
	
	/* zwraca prędkość w metry/s jako liczbę float
	 */
	float gpsSpeedInMPS();

#endif // GPS_USE_FLOAT



#ifdef GPS_USE_FLOAT_TO_STRING

	/* zamiana float na string
	 * tolerance - określa dokładność np. 0.01f
	 * zamiana powoduje przekłamanie liczby o wartość tolerancji (+/- telerancja)
	 * używane z funkcjami gpsSpeedInKnotsPH, gpsSpeedInKmPH i gpsSpeedInMPS
	 */
	uint8_t* floatToString(float num, float tolerance);

#endif // GPS_USE_FLOAT_TO_STRING





/***************************************************************************** 

-----------------------------------------------------------------------------
GGA — Global Positioning System Fixed Data. Time, Position and fix related data for a GPS receiver
-----------------------------------------------------------------------------


postać ramki GGA:
$GPGGA,035238.000,2307.1219,N,12016.4423,E,1,9,0.89,23.6,M,17.8,M,,*69
 
>>> GGA Data Format <<<

Name  				Example 		Units 	Description 
________________________________________________________________
Message ID  		$GPGGA    				GGA protocol header 
UTC Time  			035238.000    			hhmmss.sss 
Latitude 			2307.1219  				ddmm.mmmm 
N/S Indicator  		N    					N=north or S=south 
Longitude   		12016.4424    			dddmm.mmmm 
E/W Indicator  		E    					E=east or W=west
Fix Quality 		1						UWAGA! brak opisu tego pola w dokumentacji tego modelu odbiornika
											Data is from a GPS fix
											0 = Invalid
											1 = GPS fix
											2 = DGPS fix
Satellites Used  	9    					Range 0 to 14 
HDOP   				0.89 					Horizontal Dilution of Precision; Relative accuracy of horizontal position
MSL Altitude  		17.3  			meters 	Antenna Altitude above/below mean-sae-level 
Units  				M  				meters 	Units of antenna altitude 
Geoidal Separation 	17.8  			meters
Units  				M  				meters 	Units of geoidal separation 
Age of Diff. Corr. 					second	Null fields when DGPS is not used 
Checksum 			*69    
<CR> <LF>      								End of message termination 


-----------------------------------------------------------------------------
RMC — Recommended Minimum Navigation Information 
-----------------------------------------------------------------------------

postać ramki RMC:
$GPRMC,035242.000,A,2307.1220,N,12016.4420,E,0.06,0.00,140508,3.05,W,A*63


>>> RMC Data Format <<<

Name  				Example 		Units 	Description 
________________________________________________________________
Message ID  		$GPRMC    				RMC protocol header 
UTC Time  			035242.000    			hhmmss.sss 
Status 				A 						A=data valid or V=data no valid 
Latitude 			2307. 1220  			ddmm.mmmm 
N/S Indicator  		N    					N=north or S=south 
Longitude   		12016.4420    			dddmm.mmmm 
E/W Indicator  		E    					E=east or W=west 
Speed Over Ground 	0.06 			knots 
Course Over Ground 	0.00 			degrees True 
Date 				140508  				ddmmyy 
Magnetic Variation  3.05, W  		degrees	E=east or W=west (Need GlobalTop customization service) 
Mode 				A  						A= Autonomous mode 
											D= Differential mode (DGPS)
											E= Estimated mode 
Checksum 			*63    
<CR> <LF>     								End of message termination 


-----------------------------------------------------------------------------
GSA — GNSS DOP and Active Satellites
-----------------------------------------------------------------------------

postać ramki GSA:
$GPGSA,A,3,29,21,09,18,10,26,12,24,15,,,,1.20,0.89,0.80*04 


>>> GSA Data Format <<<

Name  				Example 		Units 	Description 
________________________________________________________________
Message ID  		$GPGSA    				GSA protocol header 
Mode 1  			A    					M  Manual—forced to operate in 2D or 3D mode 
											A  2D Automatic—allowed to automatically switch 2D/3D
Mode 2  			3    					1  Fix not available   
											2  2D ( < 4 SVs used) 
											3  3D ( >=4 SVs used)
Satellite Used    	29    					SV on Channel 1 
Satellite Used  	21    					SV on Channel 2 
....					
Satellite Used  	    					SV on Channel 12 
PDOP  				1.20    				Position Dilution of Precision 
HDOP   				0.89   					Horizontal Dilution of Precision
VDOP  				0.80   					Vertical Dilution of Precision 
Checksum 			*04   
<CR> <LF>      								End of message termination 



*****************************************************************************/




/* deklaracja struktury w której będą przechowywane wszelkie dane z GPS z różnych ramek NMEA
 * tablice muszą być powiększone o znak NULL
 */
struct GPS_DATA {
	
	uint8_t time[6+1];				// czas UTC 					- 185724
	uint8_t date[6+1];				// data; ddmmyy					- 140508
	
	
	uint8_t latitude[9+1];			// szerokość geograficzna		- 5013.1744
	uint8_t latitudeInd[1+1];		// wskaźnik kierunku N/S		- N	
	uint8_t longitude[10+1];		// długość geograficzna			- 01903.8160
	uint8_t longitudeInd[1+1];		// wskaźnik kierunku E/W 		- E
	uint8_t altitude[7+1];			// wysokość w m. n.p.m.			- 17.3
	uint8_t speed[7+1];				// prędkość	w węzłach			- 0.06 
	
	
	uint8_t satellites[2+1];		// ilość używanych satelit		- 9
	uint8_t status[1+1];			// status; A lub V				- A
	uint8_t mode[1+1];				// tryb; czy pomiar z DGPS		- A
	
	
	uint8_t hdop[4+1];				// precyzja szer. geo.			- 0.89
	uint8_t vdop[4+1];				// precyzja dł. geo.			- 0.80
	uint8_t pdop[4+1];				// precyzja dla trzech wpsółrz.	- 1.20
	

	uint8_t checksumRcv[2+1];		// pole na sumę kontrolną odebraną z GPS
	uint8_t checksum;				// pole na sumę kontrolną wyliczoną
	
};



/* pole bitowe
 * flagi informujące o stanie odbieranych ramek 
 */
struct GPS_FLAGS {
	
	// jakiego rodzaju jest aktualnie odczytywana ramka
	uint8_t isGGA : 1;
	uint8_t isRMC : 1;
	uint8_t isGSA : 1;
			
	// ramki zostały odebrane poprawnie, a dane zapisane w odpowiedniej strukturze
	uint8_t rdyGGA : 1;
	uint8_t rdyRMC : 1;
	uint8_t rdyGSA : 1;
	
	// informuje czy teraz będzie odbierana suma kontrolna
	uint8_t isChecksum : 1;
	
};


// definicja struktury na dane z GPS
volatile struct GPS_DATA gps;

// definicja struktury na flagi do obsługi GPS
volatile struct GPS_FLAGS gpsFlags;










#endif // __GPS_H_INCLUDED
