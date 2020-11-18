#include "chu_init.h"
#include "gpio_cores.h"
#include "xadc_core.h"
#include "sseg_core.h"
#include "spi_core.h"
#include "i2c_core.h"
#include "ps2_core.h"
#include "ddfs_core.h"
#include "adsr_core.h"
#include "math.h" // used for the mouse xmov (we use abs() to find the highest value recorded)

//Chasing LED Function
void chasingLED(GpoCore *led_p, int period) {
	//LED_Reg->write(bit_value, bit_pos);
	static uint32_t LED_OUT = 1; // represents what LED to write to
	// is static so that for each iteration of the main while loop, it will increment/decrement
	static bool left = true; // checks to see if the chasing LED is at the left most LED
	if (left) {
		led_p->write(LED_OUT);
		if (LED_OUT == pow(2, 15)) { // if we are at the LEFT most LED, we start dividing by 2 (shift right)
			LED_OUT = LED_OUT / 2;
			left = false;
		}
		else {
			LED_OUT = LED_OUT * 2; // we left shift by multiplying by 2
		}
	}
	else {
		led_p->write(LED_OUT);
		if (LED_OUT == 1) {
			LED_OUT = LED_OUT * 2; // if we are at the RIGHT most LED, we start multiplying by 2 (shift left)
			left = true;
		}
		else {
			LED_OUT = LED_OUT / 2; // we shift right by dividing by 2
		}
	}
	sleep_ms(period); // period is an input that is controlled by the right click of the mouse
}

int convertPeriod(int x) { // x goes from -127 to +127
	return (x + 128) * 350 / 255; // goes from 1 to 350 ms for simplicity
}

void mouse_check(Ps2Core *ps2_p, GpoCore *led_p) {
   int lbtn, rbtn, xmov, ymov;
   static bool left_prev = false; // holds the previous left click value
   static bool changePeriod = false; // this is used when we right click. 
									 //It will tell the program to take the mouse input as a delay (period)
   static int period = 50; // holds the period (delay) of the chasing LEDs (we start at a 50 ms delay)
   static bool pause = false; // used to tell the system if it should pause the chasing LEDs
   static int highestX = 0; // store the highest xmov value (takes the absolute value of xmov 
							// so more negative values are considered

   if (changePeriod) { // used when we want to change the period (or delay) of the chasing LEDs
    	do {
    		if (ps2_p->get_mouse_activity(&lbtn, &rbtn, &xmov, &ymov)) {
    			uart.disp("Current Delay: ");
    			uart.disp(convertPeriod(xmov));
    			uart.disp(" \r\n");
    			if(abs(highestX) < abs(xmov)) // used absolute value so that we can take the 
    				highestX = xmov;		  // largest negative/positive number
    			}
    	} while (!rbtn); // do this while we wait for another right click

		period = convertPeriod(highestX); // takes the larget xmov and converts into a delay
		highestX = 0; // resets for the next iteration of changePeriod
		changePeriod = false; // means we no longer want to change the period of the LEDs
		pause = false; // unpauses the chasing LEDs
		uart.disp("Selected Period = ");
		uart.disp(period);
		uart.disp(" ms \r\n");
		uart.disp("Resume \r\n");
		sleep_ms(500); // sleep so it doesn't observe another right click
    	  }
   else {
        if (ps2_p->get_mouse_activity(&lbtn, &rbtn, &xmov, &ymov)) {
        	if (rbtn) {
        		changePeriod = true; //if rbtn is pressed, we want to change the period of the LEDs
        		pause = true; // pause the LEDs
        		uart.disp("Pause \r\n");
        		sleep_ms(500); // sleep so it doesn't observe another right click
        	}
        	if (lbtn) {
        		if (!left_prev) { // use left_prev so that the system only recognizes one left click.
								  // if left_prev is true (means we are still holding lbtn), the following...
        			pause = !pause; // will be ignored
        			if (pause)
        				uart.disp("Pause \r\n");
        			else
        				uart.disp("Resume \r\n");
        		}
        		left_prev = true; // left_prev is true while we are holding lbtn
        	}
        }
        else
            left_prev = false; // if there is no mouse input, left_prev is false
							   // this means that we are no longer holding left click
   }  // end get_mouse_activitiy()
   if (!pause) // if the system is not paused, we will do the chasing LEDs
	  chasingLED(led_p, period);
}

void ps2_check(Ps2Core *ps2_p) { // only used for testing
   int id;
   int lbtn, rbtn, xmov, ymov;
   char ch;
   unsigned long last;

   uart.disp("\n\rPS2 device (1-keyboard / 2-mouse): ");
   id = ps2_p->init();
   uart.disp(id);
   uart.disp("\n\r");
   do {
      if (id == 2) {  // mouse
         if (ps2_p->get_mouse_activity(&lbtn, &rbtn, &xmov, &ymov)) {
            uart.disp("[");
            uart.disp(lbtn);
            uart.disp(", ");
            uart.disp(rbtn);
            uart.disp(", ");
            uart.disp(xmov);
            uart.disp(", ");
            uart.disp(ymov);
            uart.disp("] \r\n");
            last = now_ms();

         }   // end get_mouse_activitiy()
      } else {
         if (ps2_p->get_kb_ch(&ch)) {
            uart.disp(ch);
            uart.disp(" ");
            last = now_ms();
         } // end get_kb_ch()
      }  // end id==2
   } while (now_ms() - last < 5000);
   uart.disp("\n\rExit PS2 test \n\r");

}

GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));
PwmCore pwm(get_slot_addr(BRIDGE_BASE, S6_PWM));
DebounceCore btn(get_slot_addr(BRIDGE_BASE, S7_BTN));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
SpiCore spi(get_slot_addr(BRIDGE_BASE, S9_SPI));
I2cCore adt7420(get_slot_addr(BRIDGE_BASE, S10_I2C));
Ps2Core ps2(get_slot_addr(BRIDGE_BASE, S11_PS2));
DdfsCore ddfs(get_slot_addr(BRIDGE_BASE, S12_DDFS));
AdsrCore adsr(get_slot_addr(BRIDGE_BASE, S13_ADSR), &ddfs);


int main() {
	//*********************//
	//*** CLEARS 7 SEGS ***//
	//*********************//
	sseg.set_dp(0x00); //decimal point
	for (int i = 0; i < 8; i++) {
		sseg.write_1ptn(0xff, i); // turn all sseg's off
	}

	//**********************************************************//
	//*** DETERMINES WHETER WE ARE USING A MOUSE OR KEYBOARD ***//
	//**********************************************************//
	int id;
	uart.disp("\n\rPS2 device (1-keyboard / 2-mouse): ");
	id = ps2.init();
	uart.disp(id);
	uart.disp("\n\r");

	while(1) {
		//ps2_check(&ps2); // only used for testing
		if (id == 2)
			mouse_check(&ps2, &led);
	}
	//while
} //main

