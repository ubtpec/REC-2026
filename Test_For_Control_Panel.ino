#define PWM_MOTOR 8
#define POS_MOTOR 9
#define NEG_MOTOR 10
#define PWM_ACTUATOR 13
#define POS_ACTUATOR 12
#define NEG_ACTUATOR 11
#define BTN_START 22
#define BTN_POWER_ON 23
#define BTN_ESTOP 24
#define BTN_RESET 25
#define BTN_STOP 26
#define BTN_MNT_LEFT 27
#define BTN_MNT_RIGHT 28
#define BTN_CONSTRAINTS 29
#define BTN_UP 30
#define BTN_DOWN 31
#define SW_POT A0


#define INITIAL_SPEED 0
#define MAX_SPEED 255

#define MTR true
#define ACT false
#define DEBOUNCE_DELAY 20
#define IDLE 0


struct Movement {
  bool typ;
  int pwm;
  int pos;
  int neg;
  int speed;
};

bool estopActive = false; 
bool lastEstopButtonState = LOW;

struct Movement motor = { MTR, PWM_MOTOR, POS_MOTOR, NEG_MOTOR, INITIAL_SPEED };
struct Movement actuator = { ACT, PWM_ACTUATOR, POS_ACTUATOR, NEG_ACTUATOR, INITIAL_SPEED };

void motion(Movement obj, int speed, bool dir = false);

void setup() {
  setupPins();
  Serial.begin(9600);
  Serial.println("Reset");
}
//1 = Constraints
//2 = Maintenance Left
//3 = 5v
//4 = Maintenance Right
//5 = LIFT UP
//6 = LIFT DOWN
//7 = start
//8 = gnd
//9 = estop
//10 = POWER
//11 = stop
//12 = reset
//13 = pot
//15 =
//16 =
//17 =
//18 =
//19 =
//20 =
//21 =
//22 =
//23 =

void loop() {
  if (estopActive != true) {
    Serial.println(digitalRead(BTN_MNT_LEFT));
    if (digitalRead(BTN_MNT_LEFT) == LOW) {
      if (digitalRead(BTN_START) == LOW) {
        int sensorValue = analogRead(SW_POT);
        int outputValue = map(sensorValue, 0, 1023, 0, 255);
        setSpeed(outputValue, motor);
        digitalWrite(motor.pos, LOW);
        digitalWrite(motor.neg, HIGH);
      } else if (digitalRead(BTN_UP) == LOW) {
        digitalWrite(actuator.pwm, 155);
        digitalWrite(actuator.pos, HIGH);
        digitalWrite(actuator.neg, LOW);
      } else if (digitalRead(BTN_DOWN) == LOW) {
        digitalWrite(actuator.pwm, 155);
        digitalWrite(actuator.pos, LOW);
        digitalWrite(actuator.neg, HIGH);
      } else if (digitalRead(BTN_STOP) == LOW) {
        digitalWrite(actuator.pwm, 0);
        digitalWrite(motor.pwm, 0);
      }
    } else if (digitalRead(BTN_MNT_RIGHT) == LOW) {
      if (digitalRead(BTN_START) == LOW) {
        checkEstopButton();
        rideCycle();
      }
    }
  }
  if (digitalRead(BTN_ESTOP) == LOW) {
    checkEstopButton();
  }
}

void checkEstopButton() {
  bool currentEstopState = digitalRead(BTN_ESTOP);
  // // Debug output to diagnose button wiring (only when not in E-stop)
  // static unsigned long lastDebug = 0;
  // if (millis() - lastDebug >= 2000 && !estopActive) {
  //   lastDebug = millis();
  //   //Serial.print(F("E-stop pin: "));
  //   //Serial.print(currentEstopState == LOW ? "HIGH" : "LOW");
  //   // Serial.println(currentEstopState == LOW ? " - PRESSED!" : " - not pressed");
  // }

  // Button pressed when reads HIGH
  bool buttonPressed = (currentEstopState == LOW);

  // Detect button press
  if (buttonPressed && !estopActive) {
    delay(DEBOUNCE_DELAY);
    currentEstopState = digitalRead(BTN_ESTOP);
    buttonPressed = (currentEstopState == LOW);
    if (buttonPressed) {
      // E-stop button pressed - activate E-stop
      digitalWrite(actuator.pos, HIGH);
      digitalWrite(actuator.neg, LOW);
      digitalWrite(actuator.pwm, 255);
      delay(5000);
      digitalWrite(motor.pos, LOW);
      digitalWrite(motor.neg, LOW);
      digitalWrite(motor.pwm, 0);
      delay(5000);
      estopActive = true;
      // Serial.println(F("============================================"));
      // Serial.println(F("           E-STOP ACTIVATED"));
      // Serial.println(F("============================================"));
    }
  }
  // Detect button release to clear E-stop
  if (!buttonPressed && estopActive) {
    delay(DEBOUNCE_DELAY);
    currentEstopState = digitalRead(BTN_ESTOP);
    buttonPressed = (currentEstopState == LOW);

    if (!buttonPressed) {
      // E-stop button released - deactivate E-stop
      estopActive = false;
      Serial.println(F("============================================"));
      Serial.println(F("E-STOP CLEARED - Returning to login"));
      Serial.println(F("============================================"));
    }
  }
  lastEstopButtonState = currentEstopState;
}

void rideCycle() {
  setSpeed(200, motor);
  digitalWrite(motor.pos, LOW);
  digitalWrite(motor.neg, HIGH);
  for (int i = 0; i < 50; i++) {
    setSpeed(200 + i * 2, motor);
    checkEstopButton();
    delay(100);
  }
  setSpeed(150, actuator);
  setActuatorDirection(true);
  delay(2000);
  digitalWrite(actuator.pwm, INITIAL_SPEED);
  setSpeed(MAX_SPEED, motor);
  for (int i = 0; i < 350; i++) {
    checkEstopButton();
    delay(100);
  }
  stop(actuator);
  for (int i = 255; i > 160; i -= 4) {
    checkEstopButton();
    setSpeed(i, motor);
    delay(80);
  }
  stop(motor);
  delay(16000);
  rideCycle();
}

void motion(Movement obj, int speed, bool dir) {
  if (obj.pwm == PWM_ACTUATOR) {
    setSpeed(speed, obj);
    setActuatorDirection(dir);
    for (int i = 0; i < 2000; i++) {
      checkEstopButton();
      delay(1);
    }
    setSpeed(speed, obj);
    setActuatorDirection(!dir);
    for (int i = 0; i < 2000; i++) {
      checkEstopButton();
      delay(1);
    }
    stop(motor);
  } else {
    setSpeed(speed, obj);
    digitalWrite(motor.pos, LOW);
    digitalWrite(motor.neg, HIGH);
    for (int i = 0; i < 20; i++) {
      checkEstopButton();
      delay(1);
    }
    stop(motor);
  }
}


void stop(Movement obj) {
  digitalWrite(obj.pwm, INITIAL_SPEED);
  digitalWrite(obj.neg, HIGH);
  digitalWrite(obj.pos, HIGH);
  delay(10);
  digitalWrite(actuator.pos, HIGH);
  digitalWrite(actuator.neg, LOW);
  digitalWrite(actuator.pwm, 255);
  delay(5000);
  digitalWrite(actuator.pwm, INITIAL_SPEED);
  return;
}

void setActuatorDirection(bool direction) {
  if (direction) {
    // IN
    digitalWrite(actuator.pos, LOW);
    digitalWrite(actuator.neg, HIGH);
  } else {
    // OUT
    digitalWrite(actuator.pos, HIGH);
    digitalWrite(actuator.neg, LOW);
  }
}

void setSpeed(int speed, Movement obj) {
  speed = constrain(speed, INITIAL_SPEED, MAX_SPEED);
  analogWrite(obj.pwm, speed);
}

void setupPins() {
  // Button pins - most use INPUT_PULLUP (active LOW)
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_POWER_ON, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_ESTOP, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
  pinMode(BTN_MNT_LEFT, INPUT_PULLUP);
  pinMode(BTN_MNT_RIGHT, INPUT_PULLUP);
  pinMode(BTN_CONSTRAINTS, INPUT_PULLUP);

  // Motor Actuator pins

  pinMode(PWM_MOTOR, OUTPUT);
  pinMode(PWM_ACTUATOR, OUTPUT);
  pinMode(POS_MOTOR, OUTPUT);
  pinMode(POS_ACTUATOR, OUTPUT);
  pinMode(NEG_MOTOR, OUTPUT);
  pinMode(NEG_ACTUATOR, OUTPUT);
}


void buttonSerialTest() {
  if (digitalRead(BTN_START) == LOW) {
    Serial.println("START");
    //rideCycle();
  }
  if (digitalRead(BTN_STOP) == LOW) {
    Serial.println("STOP");
    //rideCycle();
  }
  if (digitalRead(BTN_POWER_ON) == LOW) {
    Serial.println("POWER ON");
    //rideCycle();
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    Serial.println("DOWN");
    //rideCycle();
  }
  if (digitalRead(BTN_UP) == LOW) {
    Serial.println("UP");
    //rideCycle();
  }
  if (digitalRead(BTN_RESET) == LOW) {
    Serial.println("RESET");
    //rideCycle();
  }
  if (digitalRead(BTN_ESTOP) == LOW) {
    Serial.println("ESTOP");
    //rideCycle();
  }
  if (digitalRead(BTN_MNT_LEFT) == LOW) {
    Serial.println("MNAINTENANCE");
    //rideCycle();
  }
  if (digitalRead(BTN_MNT_RIGHT) == LOW) {
    Serial.println("AUTO");
    //rideCycle();
  }
  if (digitalRead(BTN_CONSTRAINTS) == LOW) {
    Serial.println("CONSTRAINTS");
    //rideCycle();
  }
  if (analogRead(SW_POT) == 0) {
    Serial.println("POT SW");
    //rideCycle();
  }
  delay(10);
}
