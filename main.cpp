/* Copyright (c) 2025 justaLoli
 * Licensed under the BSD License(see license.txt).
 */

/* Includes */
#include "mbed.h"
#include "XNucleoIKS01A2.h"
#include "stdio.h"
#include <string>
#include "AS7341.h"

Serial pc1(USBTX, USBRX);

AS7341 as7341;
I2C i2c(I2C_SDA,I2C_SCL);

bool e(const char* msg){
  pc1.printf(msg);
	mbed_die();
  return true;
}

/* Simple main function */
int main() {
  pc1.baud(115200);
  //setup
  as7341.begin(i2c) || e("as: error in begin\r\n");
  as7341.setATIME(100) || e("as: error in setATIME\r\n");
  as7341.setASTEP(999) || e("as: error in setASTEP\r\n");
  as7341.setGain(AS7341_GAIN_256X) || e("as: error in setGain\r\n");
  //loop
  while(1){
    as7341.readAllChannels() || e("as: error in readAllChannels\r\n");
    
    // Print out the stored values for each channel
    pc1.printf("F1 415nm : %d\r\n",as7341.getChannel(AS7341_CHANNEL_415nm_F1));
    pc1.printf("F2 445nm : %d\r\n",as7341.getChannel(AS7341_CHANNEL_445nm_F2));
    pc1.printf("F3 480nm : %d\r\n",as7341.getChannel(AS7341_CHANNEL_480nm_F3));
    pc1.printf("F4 515nm : %d\r\n",as7341.getChannel(AS7341_CHANNEL_515nm_F4));
    pc1.printf("F5 555nm : %d\r\n",as7341.getChannel(AS7341_CHANNEL_555nm_F5));
    pc1.printf("F6 590nm : %d\r\n",as7341.getChannel(AS7341_CHANNEL_590nm_F6));
    pc1.printf("F7 630nm : %d\r\n",as7341.getChannel(AS7341_CHANNEL_630nm_F7));
    pc1.printf("F8 680nm : %d\r\n",as7341.getChannel(AS7341_CHANNEL_680nm_F8));

    pc1.printf("Clear    : %d\r\n",as7341.getChannel(AS7341_CHANNEL_CLEAR));

    pc1.printf("Near IR  : %d\r\n",as7341.getChannel(AS7341_CHANNEL_NIR));

    pc1.printf("\r\n");
  }
}