#include <Bounce2.h>

const int switchPin = 34; 
Bounce debouncer = Bounce(); 

bool isSwitchOn = false;

void setup() {
  pinMode(switchPin, INPUT_PULLUP); 
  debouncer.attach(switchPin);
  debouncer.interval(50); 
  Serial.begin(115200);
}

void loop() {
  debouncer.update();
  
  if (debouncer.fell()) {

    isSwitchOn = !isSwitchOn;

    if (isSwitchOn) {
      Serial.println("Switch is ON");

    } else {
      Serial.println("Switch is OFF");

    }
  }
}
