/* 
 *  ********************************************************************************************
 *  Project:  FYP 16-4: Design and Fabrication of an Automated Pill Dispenser
 *  Authors:  Fredrick Mungai   EN292-0679/2015
 *            Agnes Maina       EN292-4109/2015
 *  Board:    Arduino Mega 2560          
 *  Date:     22/11/2020
 *  ********************************************************************************************
 */

#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>

// ************ ARDUINO PIN ASSIGNMENTS ************
// Counting Mechanism Stepper Motor Pins
const byte counterA_IN1 = 0;
const byte counterA_IN2 = 1;
const byte counterA_IN3 = 2;
const byte counterA_IN4 = 3;

const byte counterB_IN1 = 4;
const byte counterB_IN2 = 5;
const byte counterB_IN3 = 6;
const byte counterB_IN4 = 7;

const byte counterC_IN1 = 8;
const byte counterC_IN2 = 9;
const byte counterC_IN3 = 10;
const byte counterC_IN4 = 11;

// Conveyor Motor Pin
const byte conveyorMotor = 12;

// IR Sensor Pins
const byte counterA_Sensor = 13;
const byte counterB_Sensor = 14;
const byte counterC_Sensor = 15;

const byte conveyorCounterA_Sensor = 16;
const byte conveyorCounterB_Sensor = 17;
const byte conveyorCounterC_Sensor = 18;
const byte collectionSlot_Sensor = 19;

// LCD I2C Pins (Automatically initialized in LiquidCrystal_I2C library)
// const byte serialData = 20;
// const byte serialClock = 21;

// LED Indicator Pins
const byte collectionSlotLED = 22;
const byte powerLED = 24;

// Solenoid Pins
const byte solenoid1 = 23;
const byte solenoid2 = 25;

// 4x4 Keypad Pins
const byte keypadRow1 = 26;
const byte keypadRow2 = 28;
const byte keypadRow3 = 30;
const byte keypadRow4 = 32;

const byte keypadCol1 = 34;
const byte keypadCol2 = 36;
const byte keypadCol3 = 38;
const byte keypadCol4 = 40;

// Keypad Constants
const byte rows = 4;  // four rows
const byte cols = 4;  // four columns

char keys[rows][cols] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

byte rowPins[rows] = {keypadRow1, keypadRow2, keypadRow3, keypadRow4};
byte colPins[cols] = {keypadCol1, keypadCol2, keypadCol3, keypadCol4};

// Keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

// Wiring: SDA pin is connected to pin 20, SCL pin is connected to pin 21 of Arduino Mega
// Connect LCD via I2C, address is 0x27
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);  // 20x4 LCD

// Stepper Motor constant
const byte stepSequence = 4;  // Full step mode

// Stepper Motor objects
// Pins entered in sequence IN1-IN3-IN2-IN4 for proper stepping rotation
AccelStepper counterA_Motor(stepSequence, counterA_IN1, counterA_IN3, counterA_IN2, counterA_IN4);
AccelStepper counterB_Motor(stepSequence, counterB_IN1, counterB_IN3, counterB_IN2, counterB_IN4);
AccelStepper counterC_Motor(stepSequence, counterC_IN1, counterC_IN3, counterC_IN2, counterC_IN4);

// Variables
int stepperMotorSpeed = 400;  // Steps per second
int stepperMotorMaxSpeed = 1000;
byte count = 0;
byte blinks = 3;
int blinkDelay = 500;
int conveyorOnDelay = 250;
int solenoidOnTime = 2000;

char key;  // Stores the value of current key pressed
String input = "";  // Stores the user's input
char pillType;  // Stores the type of pill to dispense
String pillName;
int pillAmount;

byte currentConveyorSensor;
byte currentCounterSensor;
AccelStepper currentCounterMotor;

bool alreadyCounted = false;  // Boolean state for preventing counting object twice
bool validPillType = false;
bool validPillAmount = false;

void setup(){
    pinMode(conveyorMotor, OUTPUT);
    pinMode(counterA_Sensor, INPUT);
    pinMode(counterB_Sensor, INPUT);
    pinMode(counterB_Sensor, INPUT);
    pinMode(conveyorCounterA_Sensor, INPUT);
    pinMode(conveyorCounterB_Sensor, INPUT);
    pinMode(conveyorCounterC_Sensor, INPUT);
    pinMode(collectionSlot_Sensor, INPUT);
    pinMode(powerLED, OUTPUT);
    pinMode(collectionSlotLED, OUTPUT);
    pinMode(solenoid1, OUTPUT);
    pinMode(solenoid2, OUTPUT);

    // Set the power LED ON
    digitalWrite(powerLED, HIGH);

    // Set the motor maximum speed
    counterA_Motor.setMaxSpeed(stepperMotorMaxSpeed);  
    counterB_Motor.setMaxSpeed(stepperMotorMaxSpeed);
    counterC_Motor.setMaxSpeed(stepperMotorMaxSpeed);

    // Set the motor speed
    counterA_Motor.setSpeed(stepperMotorSpeed);  // Lower speed to overcome friction
    counterB_Motor.setSpeed(stepperMotorSpeed);
    counterC_Motor.setSpeed(stepperMotorSpeed);
    
    // Initiate the LCD
    lcd.init();
    lcd.backlight();

    // Print welcome message
    lcd.setCursor(5, 1);  // Set the cursor on the sixth column and second row
    lcd.print("Welcome To");
    lcd.setCursor(6, 2);
    lcd.print("FYP 16-4");

    delay(3500);
    lcd.clear();
}

void loop(){
    // Select the pill type to dispense
    while (!validPillType){
        lcd.setCursor(0, 0);
        lcd.print("Select Pill Type");
        lcd.setCursor(0, 1);
        lcd.print("A: Piriton");
        lcd.setCursor(0, 2);
        lcd.print("B: Indomethacin");
        lcd.setCursor(0, 3);
        lcd.print("C: Paracetamol");

        key = keypad.getKey();  // Read the keypad
        if (key){
            if (key == 'A' || key == 'B' || key == 'C'){
                pillType = key;
                validPillType = true;
                lcd.clear();
            }
            else {
                invalidChoice();
            }
        }
    }

    // Select the amount of pills to dispense
    while (!validPillAmount){
        lcd.setCursor(0, 0);
        lcd.print("Enter No. of Pills");
        lcd.setCursor(0, 1);
        lcd.print("to Dispense: ");
        lcd.print(input);
        lcd.setCursor(0, 3);
        lcd.print("*: BACK     #: ENTER");

        key = keypad.getKey();  // Read the keypad
        if (key){
            if (isDigit(key)){
                input += key;
            }
            else if (key == '*'){
                if (input == ""){
                    validPillType = false;
                    lcd.clear();
                    break;
                }
                else {
                    input.remove(input.length() - 1);
                    lcd.clear();
                }
            }
            else if (key == '#'){
                if (input == ""){
                    invalidChoice();
                }
                else {
                    pillAmount = input.toInt();
                    if (pillAmount <= 0){
                        invalidChoice();                    
                    }
                    else {
                        validPillAmount = true;
                        lcd.clear();
                    }                    
                }
            }
            else {
                invalidChoice();
            }
        }
    }

    // Dispense the selected pills
    if (validPillType && validPillAmount){
        switch (pillType){
            case 'A':
                pillName = "Piriton";
                currentConveyorSensor = conveyorCounterA_Sensor;
                currentCounterSensor = counterA_Sensor;
                currentCounterMotor = counterA_Motor;
                break;
                
            case 'B':
                pillName = "Indomethacin";
                currentConveyorSensor = conveyorCounterB_Sensor;
                currentCounterSensor = counterB_Sensor;
                currentCounterMotor = counterB_Motor;
                break;
                
            case 'C':
                pillName = "Paracetamol";
                currentConveyorSensor = conveyorCounterC_Sensor;
                currentCounterSensor = counterC_Sensor;
                currentCounterMotor = counterC_Motor;
                break;
        }
        
        lcd.setCursor(0, 0);
        lcd.print("Dispensing ");
        lcd.print(pillAmount);
        lcd.print(" Pills");
        lcd.setCursor(0, 1);
        lcd.print("of ");
        lcd.print(pillName);
        lcd.setCursor(0, 3);
        lcd.print("Count: ");
        lcd.print(count);
        delay(1000);

        // Dispense a single vial
        digitalWrite(conveyorMotor, HIGH);
        delay(conveyorOnDelay);
        digitalWrite(solenoid1, HIGH);
        delay(solenoidOnTime);
        digitalWrite(solenoid1, LOW);
        delay(conveyorOnDelay);
        digitalWrite(solenoid2, HIGH);
        delay(solenoidOnTime);
        digitalWrite(solenoid2, LOW);

        // Wait until vial reaches selected counter then stop
        while (!digitalRead(currentConveyorSensor)){
            digitalWrite(conveyorMotor, HIGH);
        }
        digitalWrite(conveyorMotor, LOW);

        // Wait until counting is finished to continue
        while (count < pillAmount){
            currentCounterMotor.runSpeed();

            // If pill is detected and has not yet been counted, count it
            if (digitalRead(currentCounterSensor) && !alreadyCounted){
                count++;
                alreadyCounted = true;
                lcd.setCursor(0, 3);
                lcd.print("Count: ");
                lcd.print(count);
            }

            // If no pill is currently being counted, reset boolean state for counting
            else if (!digitalRead(currentCounterSensor)){
                alreadyCounted = false;
            }
        }
        currentCounterMotor.stop();
        delay(1000);

        // Wait until vial reaches collection slot then stop
        while (!digitalRead(collectionSlot_Sensor)){
            digitalWrite(conveyorMotor, HIGH);
        }
        delay(150);
        digitalWrite(conveyorMotor, LOW);
        lcd.clear();

        // Wait until the vial is removed from collection slot to continue
        while (digitalRead(collectionSlot_Sensor)){
            digitalWrite(collectionSlotLED, HIGH);

            lcd.setCursor(0, 0);
            lcd.print("DISPENSING COMPLETE!");
            lcd.setCursor(3, 2);
            lcd.print("Collect Vial");
            lcd.setCursor(4, 3);
            lcd.print("to Proceed");
            lcd.noDisplay();
            delay(blinkDelay);
            lcd.display();
            delay(blinkDelay);
        }
        digitalWrite(collectionSlotLED, LOW);
                
        // Dispensing is now complete, reset all variables
        validPillType = false;
        validPillAmount = false;
        alreadyCounted = false;
        input = "";
        count = 0;

        lcd.clear();
        delay(blinkDelay);
    }
}

void invalidChoice(){
    lcd.clear();
    for (byte i = 0; i < blinks; i++){
        lcd.setCursor(2, 1);
        lcd.print("INVALID CHOICE!");
        lcd.noDisplay();
        delay(blinkDelay);
        lcd.display();
        delay(blinkDelay);
    }
    lcd.clear();
}

/*
// Wiring: SDA pin is connected to pin 20, SCL pin is connected to pin 21 of Arduino Mega
// Connect LCD via I2C, address is 0x27
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);  // 20x4 LCD
bool startUp = true;  // Boolean state to only display start-up message once

// IR Counting Sensor variables
int irSensor = 14;  //Infrared receiver pin variable
int count = 0;  // Initialize count at 0
bool alreadyCounted = false;  // Boolean state for preventing counting object twice

void setup() {
    //Initiate the LCD
    lcd.init();
    lcd.backlight();

    pinMode(irSensor, INPUT);  // Set sensor pin as input
}

void loop() {
    //Display start-up message
    if (startUp){
        //Print 'Welcome To' on the second line of the LCD
        lcd.setCursor(5, 1);  // Set the cursor on the sixth column and second row
        lcd.print("Welcome To");
        lcd.setCursor(6, 2);
        lcd.print("FYP 16-4");

        startUp = false;
        delay(3500);
        lcd.clear();
    }

    // If sensor gives a HIGH signal (object is detected) and object has not yet been counted, count it
    if (digitalRead(irSensor) && !alreadyCounted){
        count++;
        alreadyCounted = true;
        lcd.clear();
        lcd.setCursor(3, 0);
        lcd.print("OBJECT COUNTER");
        lcd.setCursor(0, 2);
        lcd.print("Counted Objects: ");
        lcd.print(count);
    } 

    // If sensor gives a LOW signal (no object is being counted), reset boolean state for counting
    else if (!digitalRead(irSensor)){
        alreadyCounted = false;
    }
    
}


// ********* Stepper motor tutorial **********

// Creates an instance
// Pins entered in sequence IN1-IN3-IN2-IN4 for proper stepping rotation
AccelStepper counterA_Motor(fullstep, 8, 10, 9, 11);

void setup() {
    // Set the max speed
    myStepper.setMaxSpeed(1000);
    myStepper.setSpeed(500);
}

void loop() {
    // Step the motor with constant speed as set by setSpeed()
    myStepper.runSpeed();
}
*/
