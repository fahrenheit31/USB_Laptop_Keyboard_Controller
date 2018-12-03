/* Copyright 2018 Frank Adams
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
// This software interfaces the Teensy 3.2 with a PS/2 touchpad found in an HP DV9000 laptop.
// The ps/2 code uses the USB PJRC Mouse functions at www.pjrc.com/teensy/td_mouse.html 
// The ps/2 code has a watchdog timer so the code can't hang if a clock edge is missed.
// In the Arduino IDE, select Tools, Teensy LC. Also under Tools, select Keyboard+Mouse+Joystick
//
// Revision History
// Rev 1.0 - Nov 23, 2018 - Original Release
// Rev 1.1 - Dec 2, 2018 - Replaced ps/2 trackpoint code from playground arduino with my own code 
//
// This code has been tested on the touchpad from an HP Pavilion DV9000
// Touchpad part number 920-000702-04 Rev A
// The test points on the touchpad were wired to a Teensy 3.2 as follows:
// T22 = 5V wired to the Teensy Vin pin
// T23 = Gnd wired to the Teensy Ground pin   It's hard to solder to the T23 ground plane so I soldered to a bypass cap gnd pad.
// T10 = Clock wired to the Teensy I/O 30 pin
// T11 = Data wired to the Teensy I/O 27 pin
//
// Clock and Data measure open to the 5 volt pin, indicating no pull up resistors but,
// Clock and Data both measure 5 volts when the touchpad is powered, indicating active pullups are in 
// the touchpad blob top chip.
// The ps/2 signals are at 5 volts from the touchpad to the Teensy which is 5 volt tolerant.
// The ps/2 signals are at 3.3 volts from the Teensy to the touchpad which is enough to be a logic high.
// In the Arduino IDE, select Tools, Teensy 3.2. Also under Tools, select Keyboard+Mouse+Joystick
//
// The touchpad ps/2 data and clock lines are connected to the following Teensy I/O pins
#define TP_DATA 27
#define TP_CLK 30
//
// Declare variable that will be used by functions
boolean touchpad_error = LOW; // sent high when touch pad routine times out
//
// Function to float a pin and let the pull-up or Touchpad determine the logic level
void go_z(int pin)  
{
  pinMode(pin, INPUT); // make the pin an input so it floats
  digitalWrite(pin, HIGH);
}
// function to drive a pin to a logic low
void go_0(int pin)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}
//
// *****************Functions for Touchpad***************************
//
// Function to send the touchpad a byte of data (command)
//
void tp_write(char send_data)  
{
  unsigned int timeout = 200; // breakout of loop if over this value in msec
  elapsedMillis watchdog; // zero the watchdog timer clock
  char odd_parity = 0; // clear parity bit count
// Enable the bus by floating the clock and data
  go_z(TP_CLK); //
  go_z(TP_DATA); //
  delayMicroseconds(250); // wait before requesting the bus
  go_0(TP_CLK); //   Send the Clock line low to request to transmit data
  delayMicroseconds(100); // wait for 100 microseconds per bus spec
  go_0(TP_DATA); //  Send the Data line low (the start bit)
  delayMicroseconds(1); //
  go_z(TP_CLK); //   Release the Clock line so it is pulled high
  delayMicroseconds(1); // give some time to let the clock line go high
  while (digitalRead(TP_CLK) == HIGH) { // loop until the clock goes low
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
// send the 8 bits of send_data 
  for (int j=0; j<8; j++) {
    if (send_data & 1) {  //check if lsb is set
      go_z(TP_DATA); // send a 1 to TP
      odd_parity = odd_parity + 1; // keep running total of 1's sent
    }
    else {
      go_0(TP_DATA); // send a 0 to TP
    }
    delayMicroseconds(1); // delay to let the clock settle out
    while (digitalRead(TP_CLK) == LOW) { // loop until the clock goes high
      if (watchdog >= timeout) { //check for infinite loop
        touchpad_error = HIGH; // set error flag       
        break; // break out of infinite loop
      }
    }
    delayMicroseconds(1); // delay to let the clock settle out
    while (digitalRead(TP_CLK) == HIGH) { // loop until the clock goes low
      if (watchdog >= timeout) { //check for infinite loop
        touchpad_error = HIGH; // set error flag       
        break; // break out of infinite loop
      }
    }  
    send_data = send_data >> 1; // shift data right by 1 to prepare for next loop
  }
// send the parity bit
  if (odd_parity & 1) {  //check if lsb of parity is set
    go_0(TP_DATA); // already odd so send a 0 to TP
  }
  else {
    go_z(TP_DATA); // send a 1 to TP to make parity odd
  }   
  delayMicroseconds(1); // delay to let the clock settle out
  while (digitalRead(TP_CLK) == LOW) { // loop until the clock goes high
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
  delayMicroseconds(1); // delay to let the clock settle out
  while (digitalRead(TP_CLK) == HIGH) { // loop until the clock goes low
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
  go_z(TP_DATA); //  Release the Data line so it goes high as the stop bit
  delayMicroseconds(80); // testing shows delay at least 40us 
  while (digitalRead(TP_CLK) == HIGH) { // loop until the clock goes low
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
  delayMicroseconds(1); // wait to let the data settle
  if (digitalRead(TP_DATA)) { // Ack bit s/b low if good transfer
    touchpad_error = HIGH; //bad ack bit so set the error flag
  }
  while ((digitalRead(TP_CLK) == LOW) || (digitalRead(TP_DATA) == LOW)) { // loop if clock or data are low
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
// Inhibit the bus so the tp only talks when we're listening
  go_0(TP_CLK);
}
//
// Function to get a byte of data from the touchpad
//
char tp_read(void)
{
  unsigned int timeout = 200; // breakout of loop if over this value in msec
  elapsedMillis watchdog; // zero the watchdog timer clock
  char rcv_data = 0; // initialize to zero
  char mask = 1; // shift a 1 across the 8 bits to select where to load the data
  char rcv_parity = 0; // count the ones received
  go_z(TP_CLK); // release the clock
  go_z(TP_DATA); // release the data
  delayMicroseconds(5); // delay to let clock go high
  while (digitalRead(TP_CLK) == HIGH) { // loop until the clock goes low
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
  if (digitalRead(TP_DATA)) { // Start bit s/b low from tp
    touchpad_error = HIGH; // No start bit so set the error flag
  }  
  delayMicroseconds(1); // delay to let the clock settle out
  while (digitalRead(TP_CLK) == LOW) { // loop until the clock goes high
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
  for (int k=0; k<8; k++) {  
    delayMicroseconds(1); // delay to let the clock settle out
    while (digitalRead(TP_CLK) == HIGH) { // loop until the clock goes low
      if (watchdog >= timeout) { //check for infinite loop
        touchpad_error = HIGH; // set error flag       
        break; // break out of infinite loop
      }
    }
    if (digitalRead(TP_DATA)) { // check if data is high
      rcv_data = rcv_data | mask; // set the appropriate bit in the rcv data
      rcv_parity++; // increment the parity bit counter
    }
    mask = mask << 1;
    delayMicroseconds(1); // delay to let the clock settle out
    while (digitalRead(TP_CLK) == LOW) { // loop until the clock goes high
      if (watchdog >= timeout) { //check for infinite loop
        touchpad_error = HIGH; // set error flag       
        break; // break out of infinite loop
      }
    }
  }
// receive parity
  delayMicroseconds(1); // delay to let the clock settle out
  while (digitalRead(TP_CLK) == HIGH) { // loop until the clock goes low
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
  if (digitalRead(TP_DATA))  { // check if received parity is high
    rcv_parity++; // increment the parity bit counter
  }
  rcv_parity = rcv_parity & 1; // mask off all bits except the lsb
  if (rcv_parity == 0) { // check for bad (even) parity
    touchpad_error = HIGH; //bad parity so set the error flag
  } 
  delayMicroseconds(1); // delay to let the clock settle out
  while (digitalRead(TP_CLK) == LOW) { // loop until the clock goes high
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
// stop bit
  delayMicroseconds(1); // delay to let the clock settle out
  while (digitalRead(TP_CLK) == HIGH) { // loop until the clock goes low
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
  if (digitalRead(TP_DATA) == LOW) { // check if stop bit is bad (low)
    touchpad_error = HIGH; //bad stop bit so set the error flag
  }
  delayMicroseconds(1); // delay to let the clock settle out
  while (digitalRead(TP_CLK) == LOW) { // loop until the clock goes high
    if (watchdog >= timeout) { //check for infinite loop
      touchpad_error = HIGH; // set error flag       
      break; // break out of infinite loop
    }
  }
// Inhibit the bus so the tp only talks when we're listening
  go_0(TP_CLK);
  return rcv_data; // pass the received data back
}
//
void touchpad_init()
{
  touchpad_error = LOW; // start with no error
  go_z(TP_CLK); // float the clock and data to touchpad
  go_z(TP_DATA);
  //  Sending reset command to touchpad
  tp_write(0xff);
  if (tp_read() != 0xfa) { // verify correct ack byte
    touchpad_error = HIGH;
  }
  delayMicroseconds(100); // give the tp time to run its self diagnostic
  //  verify proper response from tp
  if (tp_read() != 0xaa) { // verify basic assurance test passed
    touchpad_error = HIGH;
  } 
  if (tp_read() != 0x00) { // verify correct device id
    touchpad_error = HIGH;
  }
  // increase resolution from 4 counts/mm to 8 counts/mm
  tp_write(0xe8); //  Sending resolution command
  if (tp_read() != 0xfa) { // verify correct ack byte
    touchpad_error = HIGH;
  } 
  tp_write(0x03); // value of 03 = 8 counts/mm resolution (default is 4 counts/mm)
  if (tp_read() != 0xfa) { // verify correct ack byte
    touchpad_error = HIGH;
  }
  //  Sending remote mode code so the touchpad will send data only when polled
  tp_write(0xf0);  // remote mode 
  if (tp_read() != 0xfa) { // verify correct ack byte
    touchpad_error = HIGH;
  } 
  if (touchpad_error == HIGH) { // check for any errors from tp
    delayMicroseconds(300); // wait before trying to initialize tp one last time
    tp_write(0xff); // send tp reset code
    tp_read();  // read but don't look at response from tp
    tp_read();  // read but don't look at response from tp 
    tp_read();  // read but don't look at response from tp 
    tp_write(0xe8); // Send resolution command
    tp_read();  // read but don't look at response from tp
    tp_write(0x03); // value of 03 gives 8 counts/mm resolution
    tp_read();  // read but don't look at response from tp
    tp_write(0xf0);  // remote mode 
    tp_read();  // read but don't look at response from tp 
    delayMicroseconds(100);
  }
}
// ************************************Begin Routine*********************************************************
void setup()
{
  touchpad_init(); // reset touchpad, then set it's resolution and put it in remote mode 
  if (touchpad_error) {
    touchpad_init(); // try one more time to initialize the touchpad
  }
}

// declare and initialize variables  
  char mstat; // touchpad status reg = Y overflow, X overflow, Y sign bit, X sign bit, Always 1, Middle Btn, Right Btn, Left Btn
  char mx; // touchpad x movement = 8 data bits. The sign bit is in the status register to 
           // make a 9 bit 2's complement value. Left to right on the touchpad gives a positive value. 
  char my; // touchpad y movement also 8 bits plus sign. Touchpad movement away from the user gives a positive value.
  boolean over_flow; // set if x or y movement values are bad due to overflow
  boolean left_button = 0; // on/off variable for left button = bit 0 of mstat
  boolean right_button = 0; // on/off variable for right button = bit 1 of mstat
  boolean old_left_button = 0; // on/off variable for left button status the previous polling cycle
  boolean old_right_button = 0; // on/off variable for right button status the previous polling cycle
  boolean button_change = 0; // Active high, shows when a touchpad left or right button has changed since last polling cycle
  
// ************************************Main Loop***************************************************************
void loop() {
// poll the touchpad for new movement data
  over_flow = 0; // assume no overflow until status is received 
  touchpad_error = LOW; // start with no error
  tp_write(0xeb);  // request data
  if (tp_read() != 0xfa) { // verify correct ack byte
    touchpad_error = HIGH;
  }
  mstat = tp_read(); // save into status variable
  mx = tp_read(); // save into x variable
  my = tp_read(); // save into y variable
  if (((0x80 & mstat) == 0x80) || ((0x40 & mstat) == 0x40))  {   // x or y overflow bits set?
    over_flow = 1; // set the overflow flag
  }   
// change the x data from 9 bit to 8 bit 2's complement
  mx = mx >> 1; // convert to 7 bits of data by dividing by 2
  mx = mx & 0x7f; // don't allow sign extension
  if ((0x10 & mstat) == 0x10) {   // move the sign into 
    mx = 0x80 | mx;              // the 8th bit position
  } 
// change the y data from 9 bit to 8 bit 2's complement and then take the 2's complement 
// because y movement on ps/2 format is opposite of touchpad.move function
  my = my >> 1; // convert to 7 bits of data by dividing by 2
  my = my & 0x7f; // don't allow sign extension
  if ((0x20 & mstat) == 0x20) {   // move the sign into 
    my = 0x80 | my;              // the 8th bit position
  } 
  my = (~my + 0x01); // change the sign of y data by taking the 2's complement (invert and add 1)
// zero out mx and my if over_flow or touchpad_error is set
  if ((over_flow) || (touchpad_error)) { 
    mx = 0x00;       // data is garbage so zero it out
    my = 0x00;
  } 
// send the x and y data back via usb if either one is non-zero
  if ((mx != 0x00) || (my != 0x00)) {
    Mouse.move(mx,my);
  }
//
// send the touchpad left and right button status over usb if no error
  if (!touchpad_error) {
    if ((0x01 & mstat) == 0x01) {   // if left button set 
      left_button = 1;   
    }
    else {   // clear left button
      left_button = 0;   
    }
    if ((0x02 & mstat) == 0x02) {   // if right button set 
      right_button = 1;   
    } 
    else {   // clear right button
      right_button = 0;  
    }
// Determine if the left or right touch pad buttons have changed since last polling cycle
    button_change = (left_button ^ old_left_button) | (right_button ^ old_right_button);
// Don't send button status if there's no change since last time. 
    if (button_change){
      Mouse.set_buttons(left_button, 0, right_button); // send button status
    }
    old_left_button = left_button; // remember new button status for next polling cycle
    old_right_button = right_button;
  }
//
// **************************************End of touchpad routine***********************************
// 
  delay(30);  // wait 30ms before repeating next polling cycle
}
