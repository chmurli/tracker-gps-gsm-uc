/****************************************************************************
 
 patrz plik >> gps.h

****************************************************************************/


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <util/delay.h>
#include <ctype.h>

#include "makra.h"
#include "config.h"
#include "gps.h"

#ifdef GPS_USE_FLOAT_TO_STRING
	#include <math.h>
#endif // GPS_USE_FLOAT_TO_STRING



inline void gpsInit(void) {
	DDR(GPS_RESET_PORT) |= 1<<GPS_RESET;		// ustawienie pinu do resetu GPS jako wyjście z uC
	PORT(GPS_RESET_PORT) &= ~(1<<GPS_RESET);	// stan 0, wyłączony
}


inline void gpsDisable(void) 	{	UCSR1B &= ~(1<<RXCIE1); 	}
inline void gpsEnable(void)		{	UCSR1B |= 1<<RXCIE1; 		}


inline void gpsReset(void) {
	PORT(GPS_RESET_PORT) |= 1<<GPS_RESET;		// stan 1
	_delay_ms(100);
	PORT(GPS_RESET_PORT) &= ~(1<<GPS_RESET);	// stan 0
}


inline uint8_t gpsDataRdy(void) {	
	return (gpsFlags.rdyGGA && gpsFlags.rdyRMC && gpsFlags.rdyGSA) ? 1:0;
}


inline void gpsClearDataRdy(void) {
	gpsFlags.rdyGGA=gpsFlags.rdyRMC=gpsFlags.rdyGSA=0;
}



#ifdef GPS_USE_FLOAT_TO_STRING
	
	/* tablica pomocnicza do zapisu stringa z przekonwertowaną prędkościna na km/h lub m/s
	 */
	char speed_tmp[6];
	
    
	char* floatToString(float num, float const tolerance) {
		char *fstr=speed_tmp;			// wskaźnik do tablicy, który będziemy przesuwać
		int8_t m = log10f(num);			// musi być ze znakiem!!!
		uint8_t digit;
		float weight;
	
		if(!(num > 0.0f + tolerance)) 	// jeżeli jest mniejsze niż tolerancja to zapisujemy "0.0"
		{
			*(fstr++) = '0';
			*(fstr++) = '.';
			*(fstr++) = '0';
		} else {
			
			while(num > 0.0f + tolerance)
			{
			    weight = powf(10.0f, (float)m);
			    digit = floorf(num / weight);
			    num -= (digit*weight);
			    *(fstr++) = '0' + digit;
			    if (m == 0)
			        *(fstr++) = '.';
			    --m;
			}	
		}
		
		*(fstr) = '\0';		// znak NULL
		return speed_tmp;
	}

#endif // GPS_USE_FLOAT_TO_STRING


#ifdef GPS_USE_FLOAT

	/* bardzo uproszczone przekształcenie stringa na liczbę float
	 * tylko liczby dodatnie (bez znaków '+' lub '-')
	 * brak obsługi zapisu naukowego
	 */
	 
	float myAtof(char s[]) {
		float val, power;
	    uint8_t i;

		// zamiana stringa na float
	    for(i=0; isspace(s[i]); i++) ; 		/* skip leading space */
	    for(val = 0.0; isdigit(s[i]); i++)  /* convert integer portion */
	        val = val * 10.0 + (s[i] - '0');
	    if(s[i] == '.') i++;				/* convert fraction portion */
	    for(power = 1.0; isdigit(s[i]); i++, power *= 10.0)
	        val = 10.0 * val + (s[i] - '0');
	
	    return val / power;
	}
	
	
	float gpsSpeedInKnotsPH(void) {
		return myAtof((void *)gps.speed);
	}
	
	
	
	float gpsSpeedInKmPH(void) {
		return myAtof((void *)gps.speed) * 1.852;
	}
	
	
	float gpsSpeedInMPS(void) {
		return myAtof((void *)gps.speed) * (1852/3600);
	}


#endif // GPS_USE_FLOAT


#ifdef GPS_VERIFY_CHECKSUM
	
	/* prosta zamienia stringa z liczbą hex w postaci A9 na liczbę dziesiętną
	 * tylko duże litery hex!
	 */
	static uint8_t gpsHexToDec(char const *checksum) {
		
		uint8_t result;
		if(checksum[0] >= 'A')
			result = 16 * (checksum[0] - 'A' + 10);
		else
			result = 16 * (checksum[0] - '0');
			
		if(checksum[1] >= 'A')
			result += checksum[1] - 'A' + 10;
		else
			result += checksum[1] - '0';
		
		return result;
	} 
	
#endif // GPS_VERIFY_CHECKSUM


/* sprawdź czy odebrana suma kontrolna jest równa tej wyliczonej
 * jeżeli sprawdzanie sumy kontrolnej jest wyłączone, to zawsze zwraca 1 (true)
 */
static inline uint8_t gpsVerifyChecksum(void){
	
	#ifdef GPS_VERIFY_CHECKSUM
		return (gps.checksum == gpsHexToDec((void *)gps.checksumRcv)) ? 1:0;
	#else
		return 1;
	#endif // GPS_VERIFY_CHECKSUM
}




/* tablice header'ów ramek służące do rozpoznawanie ramek; 
 * zapisane w pamięci programu
 */
prog_uint8_t frame_GGA[] = "GPGGA";
prog_uint8_t frame_RMC[] = "GPRMC";
prog_uint8_t frame_GSA[] = "GPGSA";



/* obsługa odbioru znaków z GPS następuje w przerwaniu
 * przerwanie następuje gdy zostanie odebrany jeden znak z GPS
 * w przerwaniu nastąpi od razu zdekodowanie danych i zapisanie ich do odpowiednich struktur bez buforowania danych;
 * takie rozwiązanie pozwala lepiej uśrednić czas obsługi przerwania 
 * stosuję makro 'reti()' by szybciej zakończyć przerwanie jeżeli wiem że reszta operacji mnie nie obchodzi;
 * pozwala to wcześniej zakończyć przerwanie, może to być ważne przy większych prędkościach i niższym taktowaniu uC
 * 
 * Trzeba pamiętać że dane napływają w sposób ciągły i ich obsługa powinna być jak najkrótsza
 */
ISR(USART1_RXC_vect)
{
	
	// pobieramy znak z rejestru od przerwania
	char znak;
	znak=UDR1;
	
	
	/* każda wartość w ramce NMEA jest oddzielona znakiem ',' lub '*' na końcu ramki
	 * zmienna krok informuje w którym 'polu' ramki jesteśmy
	 * jest zerowana na początku nowej ramki 
	 */
	static uint8_t krok;
	
	/* zmienna 'i' służy do numerowanie znaków w każdym polu ramki NMEA (miejsce między znakami ',' lub znakiem '*' na końcu ramki)
	 * służy do prawidłowego zapisywania danych do tablic w strukturze z danymi GPS
	 * jest zerowana na początku nowego pola w ramce
	 */
	static uint8_t i;
	
	
	/* każda ramka kończy się sumą kontrolną, służącą do potwierdznia prawidłowości transmisji
	 * są to 2 liczby hex po znaku '*', po sumie kontrolnej następuje znak nowego wiersza <CR><LF>
	 * sumę kontrolną tworzy się poprzez sumę XOR wszystkich znaków pomiędzy '$' i '*'
	 * tworzymy sumę kontrolną na bierząco i pod koniec porównujemy z odebraną
	 */
	if(znak == '*'){
		i=0;
		gpsFlags.isChecksum=1;	// nastepne dane to suma kontrolna
		reti();
	}
	
	// koniec sumy kontrolnej
	if(znak == '\r'){
		gpsFlags.isChecksum=0;
		reti();
	}
	
	// zapisujemy sumę kontrolną
	if(gpsFlags.isChecksum){
		gps.checksumRcv[i++]=znak;
		gps.checksumRcv[i]='\0';
		reti();
	}
	
	
	
	/* nowa ramka NMEA
	 * zawsze rozpoczyna się znakiem '$'
	 * zerujemy wszelkie dane
	 */
	if(znak == '$') {	
		i=0;
		krok=0;
		gpsFlags.isGGA=1;
		gpsFlags.isRMC=1;
		gpsFlags.isGSA=1;
		gps.checksum=0;
		reti();
	}


	// suma kontrolna, XOR wszystkich danych pomiędzy '$' i '*'
	if(znak != '\n')	// pomijamy '\n'
		gps.checksum ^= znak;
	

	// jeżeli ',' to zwiększamy krok (niezależnie jako to jest ramka)
	if(znak == ',') {
		++krok;
		i=0;
		reti();
	}

	
	
	/* musimy ustalić jakiego rodzaju mamy ramkę
	 * domyślnie wszystkie ramki są w stanie 'prawdziwe'
	 * w miarę odbioru znaków eliminujemy kolejne ramki, aż zostanie jedna lub żadna
	 */  
	if(!krok) {
	
		// GGA
		if(znak != pgm_read_byte(&frame_GGA[i])) 
			gpsFlags.isGGA=0;
		
		// RMC
		if(znak != pgm_read_byte(&frame_RMC[i])) 
			gpsFlags.isRMC=0;
		
		// GSA
		if(znak != pgm_read_byte(&frame_GSA[i])) 
			gpsFlags.isGSA=0;
		
		
		++i;
		reti();
			
	}


	/* do tego miejsca program dojdzie tylko w przypadku gdy krok>=1
	 * czyli w przypadku gdy już znamy rodzaj ramki i możemy ją ewentualnie dekodować
	 */
	
		
	// ramka GGA, dekodujemy ją
	if(gpsFlags.isGGA) {
	
		// koniec ramki
		if(znak == '\n' && gpsVerifyChecksum()) {
			gpsFlags.rdyGGA=1;
			reti();
		}
		
		
		switch(krok) {
	
			case 2 :
				gps.latitude[i++]=znak;
				gps.latitude[i]='\0';
				break;
			case 3 :
				gps.latitudeInd[i++]=znak;
				gps.latitudeInd[i]='\0';
				break;
			case 4 :
				gps.longitude[i++]=znak;
				gps.longitude[i]='\0';
				break;
			case 5 :
				gps.longitudeInd[i++]=znak;
				gps.longitudeInd[i]='\0';
				break;
			case 7 :
				gps.satellites[i++]=znak;
				gps.satellites[i]='\0';
				break;
			/*case 8 :
				gps.hdop[i++]=znak;
				gps.hdop[i]='\0';
				break; */
			case 9 :
				gps.altitude[i++]=znak;
				gps.altitude[i]='\0';
				break;
			default :
				break;
		}
		
	
	}
	
	
	// ramka RMC, dekodujemy ją
	if(gpsFlags.isRMC) {
	
		// koniec ramki
		if(znak == '\n' && gpsVerifyChecksum()) {
			gpsFlags.rdyRMC=1;
			reti();
		}
	
		
		switch(krok) {
		
			case 1 :
				if(i<=5) {
					gps.time[i++]=znak;
					gps.time[i]='\0';
				}
				break;
			case 2 :
				gps.status[i++]=znak;
				gps.status[i]='\0';
				break;
			case 7 :
				gps.speed[i++]=znak;
				gps.speed[i]='\0';
				break;
			case 9 :
				gps.date[i++]=znak;
				gps.date[i]='\0';
				break;
			case 12 :
				gps.mode[i++]=znak;
				gps.mode[i]='\0';
				break;
			default :
				break;
		}
		
	}
	
	// ramka GSA, dekodujemy ją
	if(gpsFlags.isGSA) {
	
		// koniec ramki
		if(znak == '\n' && gpsVerifyChecksum()) {
			gpsFlags.rdyGSA=1;
			reti();
		}
	
		
		switch(krok) {

			case 15 :
				gps.pdop[i++]=znak;
				gps.pdop[i]='\0';
				break;
			case 16 :
				gps.hdop[i++]=znak;
				gps.hdop[i]='\0';
				break;
			case 17 :
				gps.vdop[i++]=znak;
				gps.vdop[i]='\0';
				break;
			default :
				break;
		}
		
	}
	
	
	
}






