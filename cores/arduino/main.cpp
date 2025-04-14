/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#define ARDUINO_MAIN
#include "Arduino.h"

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

// Initialize C library
extern "C" void __libc_init_array(void);

void setupBrownout(){
  /* Disable the brown-out detector during configuration,
   otherwise it might misbehave and reset the
   microcontroller. */
SYSCTRL->BOD33.bit.ENABLE = 0;
while (!SYSCTRL->PCLKSR.bit.B33SRDY) {};
/* Configure the brown-out detector so that the
   program can use it to watch the power supply
   voltage */
SYSCTRL->BOD33.reg = (
    /* This sets the minimum voltage level to 3.0v - 3.2v.
       See datasheet table 37-21. */
    SYSCTRL_BOD33_LEVEL(48) |
    /* Since the program is waiting for the voltage to rise,
       don't reset the microcontroller if the voltage is too
       low. */
    SYSCTRL_BOD33_ACTION_NONE |
    /* Enable hysteresis to better deal with noisy power
       supplies and voltage transients. */
    SYSCTRL_BOD33_HYST);

/* Enable the brown-out detector and then wait for
   the voltage level to settle. */
SYSCTRL->BOD33.bit.ENABLE = 1;
while (!SYSCTRL->PCLKSR.bit.BOD33RDY) {}

/* BOD33DET is set when the voltage is *too low*,
   so wait for it to be cleared. */
while (SYSCTRL->PCLKSR.bit.BOD33DET) {}
}

/*
 * \brief Main entry point of Arduino application
 */
int main( void )
{
  setupBrownout();
  init();

  __libc_init_array();

  initVariant();
 
  delay(1);

#if defined(USE_TINYUSB)
  TinyUSB_Device_Init(0);
#elif defined(USBCON)
  USBDevice.init();
  USBDevice.attach();
#endif

  /* Let the brown-out detector automatically reset the microcontroller
  if the voltage drops too low. */
  SYSCTRL->BOD33.bit.ENABLE = 0;
  while (!SYSCTRL->PCLKSR.bit.B33SRDY) {};
  SYSCTRL->BOD33.reg |= SYSCTRL_BOD33_ACTION_RESET;
  SYSCTRL->BOD33.bit.ENABLE = 1;

  // setupWDT();
  setup();

  for (;;)
  {
    loop();
    yield(); // yield run usb background task
    // feedWatchdog();
    if (serialEventRun) serialEventRun();
  }

  return 0;
}
