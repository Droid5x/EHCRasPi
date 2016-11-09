int dirPin = 8;
int stepperPin = 11;
int n_enable = 7; // NOTE: enable pin is active low!
bool direct = true;
#define DELAY 1500
void setup() {
 pinMode(dirPin, OUTPUT);
 pinMode(stepperPin, OUTPUT);
 pinMode(n_enable, OUTPUT);
 digitalWrite(n_enable, true);
 setPwmFrequency(stepperPin, 128);
}
void stepSome(int steps){
  for (int i = 0; i < steps; i ++){
    digitalWrite(stepperPin, HIGH);
    delayMicroseconds(DELAY);
    digitalWrite(stepperPin, LOW);
    if (i != steps)
      delayMicroseconds(DELAY);
      else delayMicroseconds(DELAY-100);
  }
}
void loop(){
  int value = analogRead(0);
  if (value > 700) {
    direct = false;
    digitalWrite(n_enable, false);
    digitalWrite(dirPin,direct);
    stepSome(20);
    //analogWrite(stepperPin, 128);
  }
  else if (value < 300) {
    direct = true;
    digitalWrite(n_enable, false);
    digitalWrite(dirPin,direct);
    stepSome(20);
    //analogWrite(stepperPin, 128);
  }
  else digitalWrite(n_enable, true);

}

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
