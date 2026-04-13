//Transmitter RC plane with light indicator developed by Chad

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

const uint64_t pipeOut = 0xE9E8F0F0E1LL; // Same value as receiver 
RF24 radio(7, 8);

struct Signal {
  byte throttle;
  byte yaw;
  byte pitch;
  byte roll1;
  byte roll2;
  bool switchState; // switch state LED
};

Signal data;

void ResetData() {
  data.throttle = 0; // Motor Stop for ESC failsafe
  data.yaw = 127;
  data.pitch = 127;
  data.roll1 = 127; 
  data.roll2 = 127; 
  data.switchState = false; // Initialize switch state
}

void setup() {
  pinMode(3, INPUT_PULLUP); // Set pin 3 as input for the switch

  radio.begin();
  radio.openWritingPipe(pipeOut);
  radio.setChannel(100);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS); 
  radio.setPALevel(RF24_PA_MAX);   
  radio.stopListening(); // Start the radio communication for Transmitter 
  ResetData();
}

int mapJoystickValues(int val, int lower, int middle, int upper, bool reverse) {
  val = constrain(val, lower, upper);
  if (val < middle)
    val = map(val, lower, middle, 0, 128);
  else
    val = map(val, middle, upper, 128, 255);
  return (reverse ? 255 - val : val);
}

void loop() {
  // Control Stick Calibration 
  data.pitch =    mapJoystickValues(analogRead(A1), 12, 524, 1020, true); // (Y) axis direction on the right joystick ... pitch
  data.roll1 =    mapJoystickValues(analogRead(A0), 12, 524, 1020, false); // (X) axis direction on the right joystick ... Aileron
  data.roll2 =    mapJoystickValues(analogRead(A0), 12, 524, 1020, true);
  data.throttle = mapJoystickValues(analogRead(A2), 0, 510, 1023, false); 
  data.yaw =      mapJoystickValues(analogRead(A3), 12, 524, 1020, true); // (X) axis direction on the left joystick    

  // Read switch state from pin 3
  data.switchState = digitalRead(3) == LOW; // Switch is active LOW

  // Send data
  radio.write(&data, sizeof(Signal));
}
