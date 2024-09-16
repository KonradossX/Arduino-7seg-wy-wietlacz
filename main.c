#include <avr\io.h>
#include <avr\iom32.h>
#include <util\delay.h>
#include <avr\interrupt.h>

#define F_CPU 1600000UL
#define ZMIERZONO 1
#define OCZEKIWANIE 0

//volatile unsigned short ledy, przyciski;
// volatile unsigned short swiec_all = 0x00, swiec_nic = 0xFF, lin = 0b01111111;
//volatile unsigned short jeden, dwa, trzy, cztery, piec, szesc, siedem, osiem, dziewiec;
volatile float wynik;
volatile unsigned int pomiar;
volatile unsigned char flaga;

//Zapisanie cyfr 
#define cyf0 0b00111111
#define cyf1 0b00000110
#define cyf2 0b01011011
#define cyf3 0b01001111
#define cyf4 0b01100110
#define cyf5 0b01101101
#define cyf6 0b01111101
#define cyf7 0b00000111
#define cyf8 0b01111111
#define cyf9 0b01101111

volatile unsigned char numerWyswietlacza = 0;
#define LICZBA_WYSWIETLACZY 4

void inicjacjaKonwertera(void) {
	//Aktywancja przetwornika, preskaler 128, wlaczenie przerwania
	ADCSRA = (1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2)|(1<<ADIE); 
	ADMUX = (1<<REFS0);	//Napiecie odniesiena zewnatrzne - dzialao lepiej
}

void inicjacjaTimera(void) { //Wlaczenie timera
	TCCR0 = 0b00000011;
	OCR0  = 0x80;		//Rejestr komparatora
	TIMSK = (0<<OCIE0)|(1<<TOIE0); //W³acznie przerwania przepelnienie
	TCNT0 = 0;			//Reset licznika
}

//Funkcja ta wyswietla 1 cyfre wyniku
void wyswietl_wynik(unsigned char wyswietlacz) { 
	float liczba = wynik; //Pobieramy ze zmiennej globalnej
	volatile unsigned short	tablica_cyfr[11] = {
		cyf0,
		cyf1,
		cyf2,
		cyf3,
		cyf4,
		cyf5,
		cyf6,
		cyf7,
		cyf8,
		cyf9,
		0x00 //Czyszczenie - pusty wyswietlacz
	};
	
	int kropka = 100;
	int cyfry[4];
	
	if (liczba < 10) { //1.234
		cyfry[0] = ((int)(liczba/1   ))%10; 
		cyfry[1] = ((int)(liczba*10  ))%10;  
		cyfry[2] = ((int)(liczba*100 ))%10;  
		cyfry[3] = ((int)(liczba*1000))%10;
		kropka = 0;  
	} else if ((liczba >= 10) && (liczba < 100)) { //12.34
		cyfry[0] = ((int)(liczba/10  ))%10;  
		cyfry[1] = ((int)(liczba/1   ))%10;  
		cyfry[2] = ((int)(liczba*10  ))%10;  
		cyfry[3] = ((int)(liczba*100 ))%10;  
		kropka = 1;
	} else if ((liczba >= 100) && (liczba < 1000)) { //123.4
		cyfry[0] = ((int)(liczba/100 ))%10;
		cyfry[1] = ((int)(liczba/10  ))%10;
		cyfry[2] = ((int)(liczba/1   ))%10;
		cyfry[3] = ((int)(liczba*10  ))%10;
		kropka = 2;
	} else if (liczba >= 1000) { //1234.
		cyfry[0] = ((int)(liczba/1000))%10;
		cyfry[1] = ((int)(liczba/100 ))%10;
		cyfry[2] = ((int)(liczba/10  ))%10;
		cyfry[3] = ((int)(liczba/1   ))%10;
		kropka = 3;
	}
	
	//Wywolujac funckje wybieramy ktory z wyswietlaczy ma byc teraz wlaczony
	if (wyswietlacz == kropka) {
		PORTB = tablica_cyfr[cyfry[wyswietlacz]] | 0b10000000;
	} else {
		PORTB = tablica_cyfr[cyfry[wyswietlacz]];
	}
		
	//starszy kod do testow jakby cos sie zepsulo
// 	PORTD =~0b1000;
// 	for (int i = 0; i < 4; i++) {
// 		if (i == kropka) {
// 			PORTB = tablica_cyfr[cyfry[i]] | 0b10000000; 
			//zapalanie kropki (poprawi³am & na | i dzia³a :D )
// 		} else {
// 			PORTB = tablica_cyfr[cyfry[i]];
// 		}
// 		_delay_ms(510);
// 		PORTD = PORTD >> 1;
// 	}
}

ISR(ADC_vect) { //Przerwanie po zakonczonej konwersji AC
	pomiar = (ADCH<<8) | (ADCL); //odczyt 10 bit
	flaga = ZMIERZONO;
}

ISR(TIMER0_OVF_vect) { //Przerwanie wywolane przez timer
	//TCNT0 = 0; //Zerowanie timera 
	wyswietl_wynik(numerWyswietlacza); //Wyswietlanie 1 cyfry wyniku
	
	numerWyswietlacza++; //Zmiana wyswietlacza
	
	 //Ustawienie Portu D w zaleznosci od wybranego wyswietlacza
	PORTD = ~(1<<(LICZBA_WYSWIETLACZY - numerWyswietlacza));
	
	if (numerWyswietlacza >= LICZBA_WYSWIETLACZY) {
		numerWyswietlacza = 0;
	}
}

int main(void) {	
	inicjacjaKonwertera(); 
	inicjacjaTimera();
	sei(); //wlaczenie przerwan
		
	DDRD  = 0xFF; //tryb wyjsciowy portu D
	PORTD = 0x00; //stan wysoki na wyjsciu

	DDRB  = 0xFF; //tryb wyjciowy portu B //zapis szestnastkowy
	PORTB = 0x00; //stan wysoki na wyjsciu
	
	PORTA = 0x00; //tryb wejsciowy portu A
	
	//PORTB = cyf9; //test
	//PORTD = 0x00;

	PORTD =~0b1000; //Pocz¹tkowe ustawieie wyœwietalczy na pierwszy
	
	while(1) {	
		ADCSRA |= (1<<ADSC); //W³¹czenie pomiaru
		//while(ADCSRA & (1<<ADSC));
		
		if (flaga == ZMIERZONO) {
			wynik = (float)pomiar * 5.0 / 1024.0; //Konwersja na wolty
			//wynik = 20.05;
			//wyswietl_wynik();
			flaga = OCZEKIWANIE;
		}
		
		/*
		* tutaj
		* mog¹
		* byæ
		* inne
		* rzeczy
		*/
		
		_
	}
}
