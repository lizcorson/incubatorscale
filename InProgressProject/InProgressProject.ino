/******************************************
	PURPOSE:	Weight scale with outputs to SD card and LCD screen
	Created by      Liz Corson, ENCH686
        Inspired by:    tutorials on makecourse.com
	DATE:		11/2015
*******************************************/

#include <RTClib.h>
#include <Wire.h>
#include "RTClib.h"
 
RTC_DS1307 RTC;
#include <SPI.h>

#include "HX711.h"

/*
 This sketch shows how to use a Catalex MicroSD card adapter for datalogging
 using the Arduino standard SD.h library. 	
 The circuit:
 * 1k (or higher) potentiometer as 'data input device': swiper connected to analog pin 0, other two pins: connect to 5V and GND

 * Catalex MicroSD card adapter attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** SCK - pin 13
 ** CS - pin 10
 ** GND - GND
 ** Vcc - 5V
 
*/

#include <SD.h>      //this includes the SD card libary that comes with the Arduino
#include <TimerOne.h>//this is a library that uses the (16 bit) timer 1 of the arduino to trigger interrupts in certain time intervals.
                     //here we use it to read sensor values precisely every 500ms. 

#include <LiquidCrystal_I2C.h>//this is the special I2C LCD display library that came with the display

LiquidCrystal_I2C myDisplay(0x27,16,2);  //this instantiates an LCD object named "myDisplay"
                                         //We set the LCD address to 0x27 (this is a hexadecimal number, which is equal to 0b100111); this is the I2C 
                                         //bus address for the PCF8574 port extender chip that controls the display (see tutorial video)
                                         //and define the number of columns (16) and rows (2) of the display
                                         //this tells the methods in the library what display we are dealing with


#define chipSelect 10//we are using pin#10 as chip select pin for the SD card
#define potentiometerSwiperPin 0//use A0 as sensor input

//volatile int sensorValue;//this is the variable used in the Interrupt Service Routine (ISR) for 'reporting' the potentiometer value to the main loop.
volatile int buttonVals[3];
volatile unsigned long buttonTime;//this is the variable use in the ISR to record the time when the sensor was readout.
volatile byte buttonFlag;//this flag is used to communicate to the main loop that a new value was read.

const byte tareButtonPin = 6;    // the number of the tare button pin
const byte powerButtonPin = 9;    // the number of the power button pin
const byte recordButtonPin = 2;     // the number of the record button pin
byte buttonState = 0;         // variable for reading the pushbutton status
byte lastTimeWritten = 0;

// HX711.DOUT	- pin 3 
// HX711.PD_SCK	- pin 4 

HX711 scale(4, 3);		// parameter "gain" is ommited; the default value 128 is used by the library

int powerStatus = 1;

void setup()
{
  Serial.begin(9600);//start setial port at 9600 baud
  Wire.begin();
  RTC.begin();  
  myDisplay.init();                      //initialize the lcd - this sets the character canvas to 5x8 pixels and some other hardware specifics
                                         //Note: This .init() method also starts the I2C bus, i.e. there does not need to be
                                         //a separate "Wire.begin();" statement in the setup.
 
  myDisplay.backlight();//this turns the backlight on

  
  //set pin modes
  pinMode(recordButtonPin, INPUT);
  pinMode(tareButtonPin, INPUT);
  pinMode(powerButtonPin, INPUT);
  pinMode(chipSelect, OUTPUT);//set chip select PIN as output. If you use another pin, apparently #10 needs to be an output anyway to be able to use the SD.h library.
  
  //set up scale
  scale.set_scale(493.5);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();				        // reset the scale to 0
  
  //set up SD card
  Serial.print("Initializing SD card..."); 
  if (!SD.begin(chipSelect)) { // see if the card is present and can be initialized:
    Serial.println("Card failed, or not present");
    myDisplay.setCursor(0,1);
    myDisplay.print("SD Card Missing");
    return;//exit the setup function. This quits the setup() and the program counter jumps to the loop().
  }
  Serial.println("card initialized.");//otherwise, tell us that the card was successfully initialized.
   
  Timer1.initialize(100000); // initialize timer1, and set a 100 ms second period for the interrupt interval (i.e. the ISR will be called
  //every 100000 us to read out the potentiometer that simulates a sensor in this tutorial.
  Timer1.attachInterrupt(buttonCheck);  // attaches the readoutPotentiometer() function as 'Interrupt Service Routine' (ISR) to the timer1 interrupt
  //this means that every time 100000 us have passed, the readoutPotentiometer() routine will be called.

}

void loop()
{
  DateTime now = RTC.now();
  String dataString = "";//instantiate (make) an object of the string class for assembling a text line of the datalog
  int currentWeight;
  int msgTime;
  byte msgFlag;
  if (powerStatus) {
    currentWeight = (int) scale.get_units(10); // note: I'm concerned this may be too slow with the timer interrupt function
    myDisplay.setCursor(0,0);
    String printString = String("Weight: ") + String(currentWeight) + String("g   ");
    myDisplay.print(printString);
    
    //clear old messages
    if (millis() - msgTime > 5000 && msgFlag) {
    myDisplay.setCursor(0,1);
    myDisplay.print("                ");
    msgFlag = 0;
    }
  }

  

  if (buttonFlag == 1)    //if there is a button pushed
  {
    
    //check which button
    if (buttonVals[0] == HIGH && powerStatus) { // if it's the record button AND power is ON
      dataString = getDigitalTime(now) + String(",") + String(currentWeight); //concatenate (add together) a string consisting of the time and the sensor reading at that time
                         //the time and the reading are separated by a 'comma', which acts as the delimiter enabling to read the datalog.txt file as two columns into
                         //a spread sheet program like excel.             
    
      File dataFile = SD.open("datalog.txt", FILE_WRITE);//open a file named datalog.txt. FILE_WRITE mode specifies to append the string at the end of the file
                         //file names must adhere to the "8.3 format"(max 8 char in the name, and a 3 char extension)
                         //if there is no file of that name on the SD card, this .open method will create a new file.
                         //This line actually instantiates an File object named "datafile"
      buttonFlag = 0;      //reset the button reading flag. This prevents the loop from running until a new sensor reading comes from the ISR.    
   
      if (dataFile && compareLogTimes(lastTimeWritten, buttonTime) == 1) {       // if the file is available, write to it ('datafile' is returned 1 if SD.open was successful.
        dataFile.println(dataString);//print the concatenated data string and finish the line with a carriage return (println adds the CR automatically after printing the string)
        dataFile.close();   //close the file. IT is a good idea to always open/close a file before and after writing to it. That way, if someone removes the card the file is most
                          //likely o.k. and can be read with the computer.
      
        Serial.println(dataString);// print the string also to the serial port, so we can see what is going on.
        lastTimeWritten = buttonTime;
        myDisplay.setCursor(0,1);
        myDisplay.print("Recorded weight.");
        msgTime = millis();
        msgFlag = 1;
      }  
    
      // if SD.open is not successful it returns a 0, i.e. the else{} is executed if the file could not be opened/created successfully.
      else {
        Serial.println("error opening datalog.txt");//in that case print an error message
        myDisplay.setCursor(0,1);
        myDisplay.print("Record failed.  ");
        msgTime = millis();
        msgFlag = 1;
      }
    }
   if (buttonVals[1] == HIGH && powerStatus) { // if tare button pushed and power is ON
        scale.tare();				        // reset the scale to 0
        Serial.println("Taring.");
        myDisplay.setCursor(0,1);
        myDisplay.print("Taring.         ");
        buttonFlag = 0;      //reset the button reading flag. This prevents the loop from running until a new sensor reading comes from the ISR.    
        msgTime = millis();
        msgFlag = 1;
     
   } 
  if (buttonVals[2] == HIGH) { // if power button is pushed
    if (powerStatus == 1) {
      powerStatus = 0; 
      Serial.println("Powering down.");
      scale.power_down();
      myDisplay.noBacklight();//this turns the backlight off
      myDisplay.noDisplay(); //turn display off
    }
    else {
      powerStatus = 1;
      Serial.println("Powering up.");
      scale.power_up();
      scale.tare();				        // reset the scale to 0 when powering up
      myDisplay.display(); //turn display on
      myDisplay.backlight(); // turn backlight on

    }
   }
    

  }
}
/* Original version
void readoutPotentiometer()//this is the ISR routine that is executed everytime the timer1 interrupt is called.
{
  sensorValue = analogRead(potentiometerSwiperPin);//read out the potentiometer swiper
  sensorTime = millis();  //note the time
  sensorFlag = 1;         //set the flag that tells the loop() that there is a new sensor value to be printed.
}*/

//write a new timer interrupt function for all buttons
/*void readoutPotentiometer() {
  buttonState = digitalRead(recordButtonPin);
  if (buttonState == HIGH) {   //if the button is pressed then get the potentiometer value
    sensorValue = analogRead(potentiometerSwiperPin);//read out the potentiometer swiper
    sensorTime = millis();  //note the time

    sensorFlag = 1;         //set the flag that tells the loop() that there is a new sensor value to be printed.
  }
}*/

void buttonCheck () {
  int recordButtonState = digitalRead(recordButtonPin);
  int tareButtonState = digitalRead(tareButtonPin);
  int powerButtonState = digitalRead(powerButtonPin);
  
  if (recordButtonState == HIGH || tareButtonState == HIGH || powerButtonState == HIGH) {
    buttonFlag = 1; //set the flag that tells the loop() that a button was pushed
    buttonTime = millis(); //note the time
  }
  
  buttonVals[0] = recordButtonState;
  buttonVals[1] = tareButtonState;
  buttonVals[3] = powerButtonState;
}

int compareLogTimes(int oldtime, int newtime) {
  int result = 0;
  if (oldtime == newtime) { // if new time is the same as old time
    result = 0;
  } else if (abs(newtime - oldtime) < 500) {// or if they are within a certain threshold -- 500 is placeholder
    result = 0;
  }  else {  // otherwise, the times are considered different
    result = 1; 
  }
  //Serial.println(newtime - oldtime);
  return result;
}

String getDigitalTime (DateTime now) {
  String hourstring = "";
  String minstring = "";
  String secstring = "";  
  
  //add zeros to front of 1 character time components  
  if (now.hour() < 10) {
    hourstring = String(0) + String(now.hour());
  }
  if (now.minute() < 10) {
    minstring = String(0) + String(now.minute());
  }
  if (now.second() < 10) {
    secstring = String(0) + String(now.second());
  }
  String result = String(now.year()) + '/' + String(now.month()) + '/' + String(now.day()) + ' ' + hourstring + ':' + minstring + ':' + secstring;  
  return result;
}
