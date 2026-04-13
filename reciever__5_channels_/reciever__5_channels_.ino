#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

int ch_width_1 = 0;
int ch_width_2 = 0;
int ch_width_3 = 0;
int ch_width_4 = 0;

Servo ch1;
Servo ch2;
Servo ch3;
Servo ch4;

struct Signal {
  byte throttle; 
  byte yaw;     
  byte pitch;
  byte roll1;
  byte roll2;
  bool switchState; // switch state LED
};

Signal data;

const uint64_t pipeIn = 0xE9E8F0F0E1LL;
RF24 radio(7, 8); 

const int ledPin = 6; // LED connected to pin 6

void ResetData() {
  data.throttle = 0; // For ESC motor startup before initializing the control and failsafe
  data.yaw = 127;
  data.pitch = 127;  
  data.roll1 = 127;   
  data.roll2 = 127;   
  data.switchState = false; // Initialize switch state
}

void setup() {
  // Pins for each channel servos and ESC
  ch1.attach(2);
  ch2.attach(3);
  ch3.attach(4);
  ch4.attach(5);
  
  // Configure the NRF24 module
  ResetData();
  radio.begin();
  radio.openReadingPipe(1, pipeIn);
  radio.setChannel(100);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);  
  radio.setPALevel(RF24_PA_MAX);        
  radio.startListening(); // Initial communication for receiver
  
  pinMode(ledPin, OUTPUT); // Set LED pin as output
  digitalWrite(ledPin, LOW); // Turn off LED initially
}

unsigned long lastRecvTime = 0;
unsigned long lastBlinkTime = 0;
bool ledOn = false;
bool switchStatePrev = false; // Variable to track previous switch state
int blinkState = 0; // To track the current blink state
int blinkCount = 0; // To count the number of blinks

void recvData() {
  while (radio.available()) {
    radio.read(&data, sizeof(Signal));
    lastRecvTime = millis(); // Update the last receive time
  }
}

void loop() {
  recvData();
  unsigned long now = millis();
  
  // Check if switch state changes LED behavior
  if (data.switchState != switchStatePrev) {
    switchStatePrev = data.switchState;
    if (!data.switchState) {
      // Switch turned off, turn off the LED
      digitalWrite(ledPin, LOW); // Turn off LED
      ledOn = false;
      blinkState = 0; // Reset blink state
      blinkCount = 0; // Reset blink count
    }
  }

  // Implementing the fail-safe mechanism
  if (now - lastRecvTime > 1000) { // No signal received for 1 second
    ResetData(); // Signal lost, reset data
    digitalWrite(ledPin, LOW); // Turn off LED
  } else {
    if (data.switchState) {
   
      switch (blinkState) {
        case 0: // First blink on
          if (now - lastBlinkTime >= 100) { // 100ms blink interval
            lastBlinkTime = now;
            ledOn = true;
            digitalWrite(ledPin, HIGH);
            blinkState = 1;
          }
          break;
        case 1: // First blink off
          if (now - lastBlinkTime >= 100) {
            lastBlinkTime = now;
            ledOn = false;
            digitalWrite(ledPin, LOW);
            blinkCount++;
            if (blinkCount < 2) {
              blinkState = 0; // Start next blink
            } else {
              blinkState = 2; // Move to delay state
            }
          }
          break;
        case 2: // Delay state
          if (now - lastBlinkTime >= 1000) { // 1 second delay
            lastBlinkTime = now;
            blinkCount = 0;
            blinkState = 0; // Restart blink pattern
          }
          break;
      }
    } else {
      digitalWrite(ledPin, LOW); 
    }
  }

  ch_width_1 = map(data.throttle, 0, 255, 1000, 2000); // pin D2 (PWM signal)
  ch_width_2 = map(data.yaw, 0, 255, 1000, 2000); // pin D3 (PWM signal)
  ch_width_3 = map(data.roll1 + data.pitch, 0, 255, 1000, 2000); // pin D4 (PWM signal)
  ch_width_4 = map(data.roll2 + data.pitch, 0, 255, 1000, 2000); // pin D5 (PWM signal)

  // Write the PWM signal 
  ch1.writeMicroseconds(ch_width_1);
  ch2.writeMicroseconds(ch_width_2);
  ch3.writeMicroseconds(ch_width_3);
  ch4.writeMicroseconds(ch_width_4);
}
