/*
 * Jiøí Matìjka
 * xmatej52
 * CHANGELOG:
 * 17. 12. - vytvoreni souboru
 */

/* Header file with all the essential definitions for a given type of MCU */
#include "MK60D10.h"
#include <time.h>

// https://stackoverflow.com/questions/11530593/cycle-counter-on-arm-cortex-m4-or-m3
#define start_timer()    *((volatile uint32_t*)0xE0001000) = 0x40000001  // Enable CYCCNT register
#define stop_timer()   *((volatile uint32_t*)0xE0001000) = 0x40000000  // Disable CYCCNT register
#define get_timer()   *((volatile uint32_t*)0xE0001004)               // Get value from CYCCNT register


#define LEN 4

unsigned char data[]   = "1234567 1234567 1234567 1234567 1234567 1234567 1234567 1234567 1234567 1234567 1234567 1234567 1234567 1234567 1234567 1234567";
uint32_t crc_table[256] = {0,};

void Init(void)  {
    MCG_C4 |= ( MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01) );
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
    WDOG_STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK;
    start_timer();

    uint32_t remainder = 0;
	for ( int i = 0; i < 256; i++ ) {
		remainder = i;
		for ( int i = 0; i < 8; i++ ) {
			if ( remainder & 1 ) {
				remainder = ( remainder >> 1 ) ^ 0xEDB88320;
			}
			else {
				remainder = ( remainder >> 1 );
			}
		}
		crc_table[ i ] = remainder;
	}
}

/* A delay function */
void delay(long long bound) {
  long long i;
  for(i=0;i<bound;i++);
}

// zde mi hodne pomohla stranka http://www.hackersdelight.org/hdcodetxt/crc.c.txt, kde jsem nasel optimalizaci reverzniho reseni, ktere bylo pomalejsi
uint32_t crc32_hardware() {
	SIM_SCGC6 |= SIM_SCGC6_CRC_MASK;
	CRC_CTRL = CRC_CTRL_WAS_MASK | CRC_CTRL_TCRC_MASK;
	CRC_CRC = 0xFFFFFFFF;
	CRC_CTRL = CRC_CTRL_TCRC_MASK;
	CRC_GPOLY = 0x04C11DB7;

	uint32_t * ptr = ( uint32_t *) &data[0];

	while( *ptr ) {
		CRC_CRC = *ptr++;
	}

	uint32_t result = CRC_CRC;
	return result;
}

// Toto pocita dobre!
unsigned int crc32_software( char *message ) {
	unsigned int crc, mask;

	crc = 0xFFFFFFFF;
	// dokud nedorazim na konec zpravy
	while ( *message ) {
		crc ^= *message++; // xorujeme s aktualne pocitanym bytem
		// dokud nezpracuji cely byte -> jedu po bitech
		for ( int i = 0; i < 8; i++ ) {
			mask = -( crc & 1 ); // posledni bit
			crc = ( crc >> 1 ) ^ ( 0xEDB88320 & mask ); // vyhodim posledni bit z crc a zxoruji to s nulou nebo jednickou. A to cislo je magicka konstanta
		}
	}
	return ~crc; // jeste nesmime zaomenout znegovat!
}

unsigned long crc32_software_table( char *message ) {
    uint32_t crc = 0xFFFFFFFF;
    while ( *message ) {
    	crc = crc_table[ *message++ ^ ( crc & 0xff ) ] ^ ( crc >> 8 );
    }
    return ~crc;
}

int main(void)
{
	uint32_t start, total;
	Init();

	start = get_timer();
	unsigned int result1 = crc32_software(data); // 0x5B93F1A2
	total = get_timer() - start; // 30 901
	start = get_timer();
	unsigned int result2 = crc32_software_table(data); // 0x5B93F1A2
	total = get_timer() - start; // 3 723
	start = get_timer();
	unsigned int result3 = crc32_hardware(data); // 0x3acaf01a -- ale proc?
	total = get_timer() - start; // 696
    while (1);
    return 0;
}
