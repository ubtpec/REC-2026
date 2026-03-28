#define PWM_MOTOR 8
#define POS_MOTOR 9
#define NEG_MOTOR 10
#define PWM_ACTUATOR 13
#define POS_ACTUATOR 12
#define NEG_ACTUATOR 11
#define BTN_START 22
#define BTN_POWER 23
#define BTN_ESTOP 24
#define BTN_RESET 25
#define BTN_STOP 26
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

bool estopActive = false;               // initialize state of estop
bool lastEstopButtonState = LOW; 

struct Movement motor = {MTR, PWM_MOTOR,POS_MOTOR,NEG_MOTOR,INITIAL_SPEED};
struct Movement actuator = {ACT, PWM_ACTUATOR,POS_ACTUATOR,NEG_ACTUATOR,INITIAL_SPEED};

void motion(Movement obj, int speed, bool dir = false);

void setup() {
  // put your setup code here, to run once:
  setupPins();
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if(estopActive != true){
    if(digitalRead(BTN_START) == LOW){
      if(digitalRead(BTN_RESET) == HIGH){
        if(digitalRead(BTN_STOP) == HIGH){
          motion(motor,MAX_SPEED);
        }else{
          Serial.println(analogRead(SW_POT));
          if(analogRead(SW_POT) <= 2){
            motion(motor,50);
          }
          else if(analogRead(SW_POT) >= 3 && analogRead(SW_POT) <= 50){
            motion(motor,analogRead(SW_POT)*2+50);
          }else if(analogRead(SW_POT) >= 51){
            motion(motor,analogRead(SW_POT)+100);
          }
        }
      }else{
        motion(actuator,MAX_SPEED,true);
      }
    }
  }else{
    checkEstopButton();
  }
}

void checkEstopButton() {
  bool currentEstopState = digitalRead(BTN_ESTOP);
  
  // Debug output to diagnose button wiring (only when not in E-stop)
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug >= 2000 && !estopActive) {
    lastDebug = millis();
    Serial.print(F("E-stop pin: "));
    Serial.print(currentEstopState == LOW ? "HIGH" : "LOW");
    Serial.println(currentEstopState == LOW ? " - PRESSED!" : " - not pressed");
  }
  
  // Button pressed when reads HIGH
  bool buttonPressed = (currentEstopState == LOW);
  
  // Detect button press
  if (buttonPressed && !estopActive) {
    delay(DEBOUNCE_DELAY);
    currentEstopState = digitalRead(BTN_ESTOP);
    buttonPressed = (currentEstopState == LOW);  
    if (buttonPressed) {
      // E-stop button pressed - activate E-stop
      stop(motor);
      estopActive = true;
      Serial.println(F("============================================"));
      Serial.println(F("           E-STOP ACTIVATED"));
      Serial.println(F("============================================"));
      stop(motor);
      stop(actuator);
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


void motion(Movement obj, int speed, bool dir){
  if(obj.pwm == PWM_ACTUATOR){
    setSpeed(speed,obj);
    setActuatorDirection(dir);
    for(int i = 0; i < 2000; i++){
      checkEstopButton();
      delay(1);
    }
    setSpeed(speed,obj);
    setActuatorDirection(!dir);
    for(int i = 0; i < 2000; i++){
      checkEstopButton();
      delay(1);
    }
    stop(motor);
  }else{
    setSpeed(speed,obj);
    digitalWrite(motor.pos, LOW);
    digitalWrite(motor.neg, HIGH);
    for(int i = 0; i < 20; i++){
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
  pinMode(BTN_POWER, INPUT_PULLUP);
  pinMode(BTN_ESTOP, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);

  // Motor Actuator pins
  pinMode(SW_POT, OUTPUT);
  pinMode(PWM_MOTOR, OUTPUT);
  pinMode(PWM_ACTUATOR, OUTPUT);
  pinMode(POS_MOTOR, OUTPUT);
  pinMode(POS_ACTUATOR, OUTPUT);
  pinMode(NEG_MOTOR, OUTPUT);
  pinMode(NEG_ACTUATOR, OUTPUT);
}
