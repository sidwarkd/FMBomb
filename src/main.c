#include <htc.h>

// Setup the configuration bits.  We can find what they
// should be by referencing Section 11.1 of the PIC datasheet.
// I've included the information from the datasheet below
// for reference.  Each bit is explaned.  I've marked our
// choices below.
/*
bit 15-12 Reserved: Reserved bits. Do Not Use.

bit 11 FCMEN: Fail-Safe Clock Monitor Enabled bit
1 = Fail-Safe Clock Monitor is enabled
0 = Fail-Safe Clock Monitor is disabled  <------------- WE'LL KEEP THIS DISABLED FOR SIMPLICITY

bit 10 IESO: Internal External Switchover bit
1 = Internal External Switchover mode is enabled
0 = Internal External Switchover mode is disabled <---- DON'T NEED THE SWITCHOVER ENABLED

bit 9-8 BOREN<1:0>: Brown-out Reset Selection bits(1)
11 = BOR enabled
10 = BOR enabled during operation and disabled in Sleep
01 = BOR controlled by SBOREN bit of the PCON register
00 = BOR disabled <------------------------------------ WE'RE NOT WORRIED ABOUT BROWN OUT

bit 7 CPD: Data Code Protection bit(2)
1 = Data memory code protection is disabled <---------- IMPORTANT: KEEP DISABLED
0 = Data memory code protection is enabled

bit 6 CP: Code Protection bit(2)
1 = Program memory code protection is disabled <------- IMPORTANT: KEEP DISABLED
0 = Program memory code protection is enabled

bit 5 MCLRE: MCLR Pin Function Select bit(3)
1 = MCLR pin function is MCLR <------------------------ WE NEED THIS AS MCLR FOR PROGRAMMING
0 = MCLR pin function is digital input, MCLR internally tied to VDD

bit 4 PWRTE: Power-up Timer Enable bit
1 = PWRT disabled <------------------------------------ WE'RE NOT GOING TO USE THIS
0 = PWRT enabled

bit 3 WDTE: Watchdog Timer Enable bit
1 = WDT enabled
0 = WDT disabled <------------------------------------- DON'T NEED THIS FOR OUR PROJECT

bit 2-0 FOSC<2:0>: Oscillator Selection bits
111 = EXTRC oscillator: External RC on RA5/OSC1/CLKIN, CLKOUT function on RA4/OSC2/CLKOUT pin
110 = EXTRCIO oscillator: External RC on RA5/OSC1/CLKIN, I/O function on RA4/OSC2/CLKOUT pin
101 = INTOSC oscillator: CLKOUT function on RA4/OSC2/CLKOUT pin, I/O function on
RA5/OSC1/CLKIN
100 = INTOSCIO oscillator: I/O function on RA4/OSC2/CLKOUT pin, I/O function on <-------------- WE'LL USE THE INTERNAL OSCILLATOR
RA5/OSC1/CLKIN
011 = EC: I/O function on RA4/OSC2/CLKOUT pin, CLKIN on RA5/OSC1/CLKIN
010 = HS oscillator: High-speed crystal/resonator on RA4/OSC2/CLKOUT and RA5/OSC1/CLKIN
001 = XT oscillator: Crystal/resonator on RA4/OSC2/CLKOUT and RA5/OSC1/CLKIN
000 = LP oscillator: Low-power crystal on RA4/OSC2/CLKOUT and RA5/OSC1/CLKIN
*/

// The defines used below in the __CONFIG macro can be looked up in
// pic16F688.h in the HI-TECH include folder that was laid down 
// when you installed the compiler.  This sets up the basic operation
// of our PIC at startup.
__CONFIG(FCMDIS & IESODIS & BORDIS & UNPROTECT & MCLREN & PWRTDIS & WDTDIS & INTIO);

// Let's setup some defines to make our code more readable
// Our LED is connected to RC3 on the PIC
// This is the same way RC3 is defined in the pic16F688 
// header file.
volatile       bit	LED		@ ((unsigned)&PORTC*8)+3;
#define ON 1;
#define OFF 0;
#define _XTAL_FREQ 4000000	// We're using the internal oscillator running at 4MHz

// Setup the pin defines for communicating with the FM transmitter
#define CK  RC2
#define DA  RC1
#define LA	RC0
#define IIC RA2

// Prototype for our send function
void SendCommand(char address, char data);
void init(void);



void main(void)
{
	init();

	// Wait for the power supply to settle.  This is always a good idea to do
	// at startup.
	__delay_ms(10);

	// From page 8 of the transmitter datasheet we know that to use the 
	// 3 wire SPI mode we need to hold IIC low so let's set it here
	IIC = 0;
    
	// The datasheet tells us that the first thing we need to do is
	// send a reset command before we do anything else
    SendCommand(0x0E, 0b00000101);

	// The datasheet then recommends that we load our desired
	// values into the different registers.
    //Load register values with initial default values
    SendCommand(0x01, 0b10110100); //Register 1 defaults
    SendCommand(0x02, 0b00000101); //Register 2 defaults

	// Register address 4 contains bits 13 to 8 of the N
	// value while register 3 contains bits 7 to 0;
	// To calculate N for a given FM frequency use the following
	// equation:
	// N = (f_desired_in_MHz + .304) / .008192
	// For example for 98.5 the equation would be
	// (98.5 + .304) / .008192 = 12061.035 rounded down = 12061
	// Convert that to binary and we get 0b10111100011101 and we
	// need to pad that out to 16 bits so we just left pad it with
	// zeroes to get 0010111100011101.  Put the top eight bits into
	// register 4 and the bottom eight bits into register 3.
    SendCommand(0x03, 0b00011101);
    SendCommand(0x04, 0b00101111);

    SendCommand(0x08, 0b00011010); //Register 8 defaults

	// The last thing we'll do is flip the "power switch"
	// on to the module
    SendCommand(0x00, 0b00000001);

    while (1)
	{
		LED = ON;
		__delay_ms(1000);
		LED = OFF;
		__delay_ms(1000);
	}
}

void init(void)
{
	// This is where we'll do all of our hardware initialization
	// For now all we have to do is set up our LED and FM
	// transmitter pins.

	// From the datasheet we know that if we want certain pins to be
	// digital inputs instead of analog inputs that we need to set the
	// ANSEL and CMCON0 registers.  The following settings make all 
	// pins digital.
	ANSEL = 0;
	CMCON0 = 0x07;

	// The internal oscillator we're using defaults to 4MHz on startup
	// You can confirm this on page 22 of the datasheet.  If we wanted
	// we could change the frequency here to something else.  For our
	// purposes we'll just leave it.

	// Setup pin 7 (RC3) to be an output so we can drive the LED with it.  
	// Remember, setting the TRIS bit to a 1 makes a pin an input and a 0 sets
	// it as an output.  Think 1 is like I so Input. 0 is like an O so Output.
	TRISC3 = 0; // This tells the pin to be an output
	RC3 = 0;	// This sets the initial state to 0V.
	//RC3 = 1;	// This would set the initial state to whatever your power
				// supply voltage is.  In our case, 3 volts.

	// Setup the pins that are connected to the FM transmitter
	TRISC2 = 0;
	TRISC1 = 0;
	TRISC0 = 0;
	TRISA2 = 0;	

	// It's always a good idea to set pins you aren't using to
	// be outputs and force them to 0 to save power.
	TRISC5 = 0;
	TRISC4 = 0;
	TRISA5 = 0;
	TRISA4 = 0;
	TRISA1 = 0;
	TRISA0 = 0;


	// Another way to set the TRIS bits is by doing them all at once with
	// a binary expression.
	// TRISC = 0b00000000
	// To make RC1 an input we would do TRISC = 0b00000010
}

// The FM transmitter we'll be using has two different options
// for communication; simple SPI-ish interface or I2C.  Since
// the PIC we are using doesn't have a built in I2C peripheral
// we'll iimplement the SPI version.  This might seem a little
// tricky but it's pretty straight forward if you just look
// at the timing diagrams in the transmitter datasheet and
// right code to match what they've specified.

// The address parameter is the address of the register that
// you wish to write to and data is the data you want to 
// assign.  All of the possible registers and their potential
// values can be found in the transmitter datasheet. We learn
// from the datasheet that to send a command we need to send
// 4 bits of an address and then 8 bits of data.
void SendCommand(char address, char data)
{
    char x;

	// We'll reference the diagram in section 7.1 on page 8
	// of the transmitter datasheet
	
	// The clocking diagram shows that the LA pin needs to be
	// low until the data has all been sent so lets set it to
	// zero here.
    LA = 0;
    
	// The first thing we need to send is the 4 address bits
    for(x = 0 ; x < 4 ; x++)
    {
		// When transmitting data the diagram shows us that the clock starts
		// low.
        CK = 0;

		// After the clock is low we set the outgoing data bit
		// LSB first.
        DA = address & 0x01; // This is a trick for getting the value of the LSB
        
		// Rotate 1 bit to the right.  This rolls the LSB to the right
		// and makes the second bit the LSB
		address >>= 1; 
		
		// We see from the diagram that after the data line has been
		// properly set to 1 or 0 we need to change the clock state
		// to high.
        CK = 1;
    }

	// Now that the address bits have been sent let's send the data
    for(x = 0 ; x < 8 ; x++)
    {
		// We'll follow the exact same pattern as seen with the 
		// address bits
        CK = 0;
        DA = data & 0x01;	
        data >>= 1;
        CK = 1;
    }
    
	// From the diagram we see that once all bits have been
	// sent we need to latch the transfer by setting LA
	// high.  From the table on page 9 we see that we need
	// to hold that line high for a minimum of 250 ns so
	// we'll use a 1 ms delay for good measure before setting
	// LA back to zero.
    LA = 1;
    __delay_ms(1);
    LA = 0;
}
