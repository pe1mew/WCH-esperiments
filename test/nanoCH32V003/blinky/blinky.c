#include "ch32fun.h"

// On the MuseLab nanoCH32V003 board the user LED (D1, via R1) is wired to PD6.
#define PIN_LED PD6

int main()
{
	SystemInit();

	funGpioInitAll();                                          // Enable all GPIO ports
	funPinMode( PIN_LED, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP ); // LED pin as push-pull output

	while( 1 )
	{
		funDigitalWrite( PIN_LED, FUN_HIGH );
		Delay_Ms( 750 );
		funDigitalWrite( PIN_LED, FUN_LOW );
		Delay_Ms( 250 );
	}
}
