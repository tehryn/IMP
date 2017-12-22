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

unsigned char data[] = "12345678";
uint32_t crc_table32[256] = {0,};
uint16_t crc_table16[256] = {0,};

void Init(void)  {
    MCG_C4 |= ( MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01) );
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
    WDOG_STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK;
    SIM_SCGC6 |= SIM_SCGC6_CRC_MASK;
    start_timer();

    uint32_t remainder32 = 0;
    uint16_t remainder16 = 0;
	for ( int i = 0; i < 256; i++ ) {
		remainder32 = i;
		remainder16 = i << 8;
		for ( int i = 0; i < 8; i++ ) {
			if ( remainder32 & 1 ) {
				remainder32 = ( remainder32 >> 1 ) ^ 0xEDB88320;;
			}
			else {
				remainder32 >>= 1;
			}

			if ( remainder16 & 0x8000 ) {
				remainder16 = ( remainder16 << 1 ) ^ 0x1021;
			}
			else {
				remainder16 <<= 1;
			}
		}
		crc_table32[ i ] = remainder32;
		crc_table16[ i ] = remainder16;
	}
}

/* A delay function */
void delay(long long bound) {
  long long i;
  for(i=0;i<bound;i++);
}

// zde mi hodne pomohla stranka http://www.hackersdelight.org/hdcodetxt/crc.c.txt, kde jsem nasel optimalizaci reverzniho reseni, ktere bylo pomalejsi
uint32_t crc32_hardware( char * message ) {
	CRC_CTRL = CRC_CTRL_TCRC_MASK; // Set CTRL[TCRC] to enable 32-bit CRC mode
	//CRC_GPOLY = 0x04C11DB7;        // Write a 32-bit polynomial to GPOLY[HIGH:LOW].
	CRC_GPOLY = 0x04C11DB7;
	CRC_CTRL |= CRC_CTRL_WAS_MASK; // Set CTRL[WAS] to program the seed value.
	CRC_CRC = 0xFFFFFFFF;          // Write a 32-bit seed to CRC[HU:HL:LU:LL].
	CRC_CTRL = CRC_CTRL_TCRC_MASK; // Clear CTRL[WAS] to start writing data values

	while( *message ) {
		//uint32_t val = (uint32_t)*(message+3) | (uint32_t)(*(message+2) << 8) | (uint32_t)(*(message+1) << 16) | (uint32_t)(*(message) << 24);
		CRC_CRC = (uint32_t)*(message+3) | (uint32_t)(*(message+2) << 8) | (uint32_t)(*(message+1) << 16) | (uint32_t)(*(message) << 24) ; // Write data values into CRC[HU:HL:LU:LL]. A CRC is computed on every data value write, and the intermediate CRC result is stored back into CRC[LU:LL].
		//CRC_CRC = (uint32_t)(*(message+1)) | (uint32_t)(*(message) << 8) ;
		message += 4;
	}

	uint32_t result = CRC_CRC; // When all values have been written, read the final CRC result from CRC[HU:HL:LU:LL]. The CRC is calculated bytewise, and two clocks are required to complete one CRC calculation.
	return result;
}

uint16_t crc16_hardware( char * message ) {
	CRC_CTRL = 0x0;                    // Clear CTRL[TCRC] to enable 16-bit CRC mode.
	CRC_GPOLY = 0x1021;                // Write a 16-bit polynomial to the GPOLY[LOW] field.
	CRC_CTRL = CRC_CTRL_WAS_MASK;      // Set CTRL[WAS] to program the seed value
	CRC_CRC = 0xFFFF;                  // Write a 16-bit seed to CRC[LU:LL].
	CRC_CTRL = 0x0;                    // Clear CTRL[WAS] to start writing data values.

	while( *message ) {
		//uint32_t val = (uint32_t)*(message+3) | (uint32_t)(*(message+2) << 8) | (uint32_t)(*(message+1) << 16) | (uint32_t)(*(message) << 24);
		CRC_CRC = (uint32_t)*(message+3) | (uint32_t)(*(message+2) << 8) | (uint32_t)(*(message+1) << 16) | (uint32_t)(*(message) << 24) ; // Write data values into CRC[HU:HL:LU:LL]. A CRC is computed on every data value write, and the intermediate CRC result is stored back into CRC[LU:LL].
		//CRC_CRC = (uint32_t)(*(message+1)) | (uint32_t)(*(message) << 8) ;
		message += 4;
	}

	uint16_t result = CRC_CRC; // When all values have been written, read the final CRC result from CRC[LU:LL].
	return result;
}

// Toto pocita dobre!
uint32_t crc32_software( char *message ) {
	uint32_t crc, mask;

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

uint32_t crc32_software_table( char *message ) {
    uint32_t crc = 0xFFFFFFFF;
    while ( *message ) {
    	crc = crc_table32[ *message++ ^ ( crc & 0xff ) ] ^ ( crc >> 8 );
    }
    return ~crc;
}

uint16_t crc16_software_table( char *message ) {
    uint16_t crc = 0xFFFF;
    while ( *message ) {
    	 crc = (crc << 8) ^ crc_table16[((crc >> 8) ^ *message++)];
    }
    return crc;
}

uint16_t crc16_software( char *message ) {
	uint64_t crc = 0xFFFF;
	while ( *message ) {
		crc ^= *message++ << 8;
		for ( int i = 0; i < 8; i++ ) {
			if ( crc & 0x8000 ) {
				crc = ( crc << 1 ) ^ 0x1021;
			}
			else {
				crc <<= 1;
			}
		}
	}
	return crc;
}

int main(void)
{
	uint32_t start, total;
	Init();

	start = get_timer();
	uint32_t result1 = crc32_software(data); // 0x662cd4bb - CRC-32
	total = get_timer() - start; // 1 985
	start = get_timer();
	uint32_t result2 = crc32_software_table(data); // 0x9ae0daaf - CRC-32
	total = get_timer() - start; // 271
	start = get_timer();
	uint32_t result3 = crc32_hardware(data); // 0x49e3c2fb - CRC-32/MPEG-2
	total = get_timer() - start; // 179
	start = get_timer();
	uint16_t result4 = crc16_software(data); // 0xa12b - CRC-16/CCITT-FALSE
	total = get_timer() - start; // 2 431
	start = get_timer();
	uint16_t result5 = crc16_software_table(data); // 0xa12b - CRC-16/CCITT-FALSE
	total = get_timer() - start; // 312
	start = get_timer();
	uint16_t result6 = crc16_hardware(data); // 0xa12b - CRC-16/CCITT-FALSE
	total = get_timer() - start; // 174

	data[0] = 0b00110000; // jedna chyba
	uint32_t chyba1_1 = crc32_software_table(data); // 0x564ada31
	uint16_t chyba1_2 = crc16_software_table(data); // 0xe6f8
	data[2] = 0b01110011; // +1 chyba, celkem 2
	uint32_t chyba2_1 = crc32_software_table(data); // 0x5912d05d
	uint16_t chyba2_2 = crc16_software_table(data); // 0x8ce8
	data[5] = 0b00111111; // +2 chyby, celkem 4
	uint32_t chyba3_1 = crc32_software_table(data); // 0x56c3ebd2
	uint16_t chyba3_2 = crc16_software_table(data); // 0x1279
    while (1);
    return 0;
}
