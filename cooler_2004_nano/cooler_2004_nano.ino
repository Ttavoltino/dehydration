#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <OneWire.h> // tempsensor 
#include <DallasTemperature.h> //temp sensor 
DS3231 myRTC;
DateTime myDT;
LiquidCrystal_I2C lcd(0x27, 20, 04);
Adafruit_BME280 bme;
//Constants
#define numPins 6
#define on HIGH
#define off LOW
#define ONE_WIRE_BUS 7

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
//Variables
int evTemp ;
bool chkFan;
unsigned long lastTime = 0;
unsigned long timerDelay = 2000;
int temperature = EEPROM.read(0);  // Set up  Temperature
byte humyditi = EEPROM.read(4);     // Set up humidity
byte state = EEPROM.read(5);        // True work like dehidratation False work for cooling
float hum;                          //Stores humidity value
float temp;                         //Stores temperature value
uint32_t timeStamp = EEPROM.get(6, timeStamp);
uint32_t compressorTs;
bool serialStatus = EEPROM.read(10);
byte fan_up_speed = EEPROM.read(11);
byte serialCheck = 0;
bool cooler_status = false;
bool dehumi_status = false;
bool humi_status = false;
bool heat_status = false;
bool cooler_set = true;
bool hum_set = true;
bool kompressor = true;
bool fanOnOff = EEPROM.read(12);
const byte Pin[numPins] = { 3, 4, 5, 6, 9, 8 };  //fan inner, Heating, Humyfider, Circulation, fan_up_ctrl, Compressor SSR
byte downArrow[] = { 0x00, 0x04, 0x04, 0x04, 0x04, 0x1F, 0x0E, 0x04 };
byte upArrow[] = { 0x04, 0x0E, 0x1F, 0x04, 0x04, 0x04, 0x04, 0x00 };
byte heatSymbol [] = { 0x04, 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x1F };
byte coolerSymbol [] = { 0x04, 0x15, 0x0E, 0x04, 0x0E, 0x15, 0x04, 0x00 };
byte dehumySymbol [] = { 0x00, 0x00, 0x04, 0x0E, 0x17, 0x17, 0x17, 0x0E };
byte humySymbol [] = { 0x00, 0x00, 0x04, 0x0E, 0x1F, 0x1F, 0x1F, 0x0E };
byte circSymbol [] = { 0x10, 0x11, 0x0A, 0x04, 0x10, 0x11, 0x0A, 0x04 };
byte comprWaiting []=  {  0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x00, 0x0C, 0x0C};
const int tachoPin = 2;
volatile unsigned long counter = 0;

void countPulses() {
  counter ++;
}
void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.print("Loading");
  /*----------------------------------------------------------------------------
    In order to synchronise your clock module, insert timetable values below !
    ----------------------------------------------------------------------------*/
  // myRTC.setClockMode(false);
  // myRTC.setHour(12);
  // myRTC.setMinute(40);
  // myRTC.setSecond(50);

  // myRTC.setDate(25);
  // myRTC.setMonth(2);
  // myRTC.setYear(24);
  lcd.begin();
  sensors.begin();
  lcd.createChar(0, downArrow);
  lcd.createChar(1, upArrow);
  lcd.createChar(2, heatSymbol);
  lcd.createChar(3, coolerSymbol);
  lcd.createChar(4, dehumySymbol);
  lcd.createChar(5, humySymbol);
  lcd.createChar(6, circSymbol);
  lcd.createChar(7, comprWaiting);
  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("Meat Dehydrator");
  lcd.setCursor(1, 1);
  lcd.print("By Martin Atanasov");
  lcd.setCursor(4, 2);
  lcd.print("Ver. 2.5.9");
  for (byte i = 0; i < numPins; i++) {  //for each pin
    pinMode(Pin[i], OUTPUT);
    digitalWrite(Pin[i], off);
    delay(650);
    Serial.print(".");
  }
  myDT = RTClib::now();
  bme.begin(0x76);
  //delay(3000);
  lcd.clear();
  Serial.print("\n");
  compressorTs = (myDT.unixtime() + 120);
  attachInterrupt(digitalPinToInterrupt(tachoPin), countPulses, FALLING);
}
void loop() {
  myDT = RTClib::now();
  if ((millis() - lastTime) > timerDelay) {

  hum = bme.readHumidity();
  temp = bme.readTemperature();
  sensors.requestTemperatures(); 
  evTemp = sensors.getTempCByIndex(0);


  serialPrint();

  lastTime = millis();

  }
  //delay(2000);
  readSerial();
  tempCtrl();
  humCtrl();
  fanUpCtrl();
  lcdPrint();
}
void tempCtrl() { 
  if ( state !=2){
    if (cooler_set) {
      if (temp >= (temperature +0.5)) {
        hum_set = false;
        coolerCtrl(on);
        cooler_status = true;
      } else if (temp <= (temperature -0.1)) {
        coolerCtrl(off);
        cooler_status = false;
        hum_set = true;
      }
    }
    if (temp <= (temperature - 1)) {
      digitalWrite(Pin[1], on);
      heat_status = true;
    } else if (temp >= (temperature - 0.5)){
      digitalWrite(Pin[1], off);
      heat_status = false;
    }
  } else {
    digitalWrite(Pin[1], off);
    coolerCtrl(off);
    cooler_status = false;
  }
}
void humCtrl() {
  if (state !=2){
    if (hum_set) {
      if (hum >= (humyditi + 2.9) && state==1) {
        coolerCtrl(on);
        cooler_set = false;
        dehumi_status = true;
        // if (temp <= temperature ) {
        //   digitalWrite(Pin[1], on);
        //   heat_status = true;
        // } else /* if (temp >= (temperature + 0.9))*/ {
        //   digitalWrite(Pin[1], off);
        //   heat_status = false;
        // }
      } else if (hum <= humyditi ) {
        coolerCtrl(off);
        cooler_set = true;
        dehumi_status = false;
      }
    }
    if (hum <= (humyditi - 2.9) && state==1) {
      digitalWrite(Pin[2], on);
      humi_status = true;
    } else if (hum >= (humyditi - 1)) {
      
      digitalWrite(Pin[2], off);
      humi_status = false;
    }
  } else {
    digitalWrite(Pin[2], off);
    dehumi_status = false;
    humi_status = false;
  }
  
}
void fanUpCtrl() {
  
  
  analogWrite(Pin[4], fan_up_speed);

  digitalWrite(Pin[3], fanOnOff); // WARNING
 
  //  switch (myDT.minute()) {
  //   case 0:
  //     chkFan = true;
  //     break;
  //   case 15:
  //     chkFan = true;
  //     break;
  //   case 30:
  //     chkFan = true;
  //     break;
  //   case 45:
  //   //   chkFan = true;
  //   //   break;
  //   // case 4:
  //   //   chkFan = true;
  //   // case 5:
  //   //   chkFan = true;
  //   //   break;
  //   // case 12:
  //   //   chkFan = true;
  //   //   break;
  //   // case 14:
  //   //   chkFan = true;
  //   //   break;
  //   // case 16:
  //   //   chkFan = true;
  //   //   break;
  //   // case 18:
  //   //   chkFan = true;
  //   //   break;
  //   // case 21:
  //   //   chkFan = true;
  //   //   break;
  //   // case 22:
  //   //   chkFan = true;
  //   //   break;
  //   default:
  //     chkFan = false;
  // }
  // if (chkFan && state) {
  //   fanOnOff =EEPROM.read(12);
  // } else {
  //   fanOnOff =false;
  // }
  

}
void serialPrint() {
  if (serialStatus) {
    long  rpm = counter * 23.48 / 2;
    Serial.print("Cooler:");
    if (cooler_status == true) {

      Serial.print("ON");
    } else {
      Serial.print("OFF,");
    }
    Serial.print(" Dehum:");
    if (dehumi_status == true) {
      Serial.print("ON,");
    } else {
      Serial.print("OFF,");
    }
    Serial.print(" Heat:");
    if (heat_status == true) {
      Serial.print("ON,");
    } else {
      Serial.print("OFF,");
    }
    Serial.print(" Humy:");
    if (humi_status == true) {
      Serial.print("ON,");
    } else {
      Serial.print("OFF,");
    }
    Serial.print(" Mode:");
    if (state == 1) {
      Serial.print("Dehydrating,");
    } else if (state == 0){
      Serial.print("Cooler/Freeze,");
    } else if (state == 2){
      Serial.print("OFF,");
    }
    Serial.print(" Humy:");
    if (hum_set == true) {
      Serial.print("Enabled,");
    } else {
      Serial.print("Disabled,");
    }
    Serial.print(" Cooler:");
    if (cooler_set == true) {
      Serial.print("Enabled,");
    } else {
      Serial.print("Disabled,");
    }
    Serial.print(" Fan: ");
    if (fanOnOff) {
      Serial.print(map(fan_up_speed ,26 ,255 ,1 ,100));
      Serial.print("% ");
    Serial.print(rpm);
    Serial.print("rpm, ");
    } else {
      Serial.print("OFF,");
    }
    Serial.print("T:");
    Serial.print(temperature);
    Serial.print(", ");
    Serial.print("H:");
    Serial.print(humyditi);
    Serial.print(",");
    Serial.print(" Day/s:");
    Serial.println(timeReturn());
    Serial.print("Hum:");
    Serial.print(hum);
    Serial.print("  Temp:");
    Serial.print(temp);
    Serial.print("  Evap Temp:");
    Serial.println(evTemp); 
    counter = 0;
  } else {
    if (serialCheck < 1){
    Serial.println("Serial Disabled, Enter 0 for MENU");
    serialCheck = 2;
    }
    
  }
}
void lcdPrint() {
  lcd.cursor_off();
  //lcd.clear();
  lcd.setCursor(0, 0);
  printDigitsLcd(myDT.day());
  lcd.print("/");
  printDigitsLcd(myDT.month());
  lcd.print("/");
  lcd.print(myDT.year());
  lcd.setCursor(12, 0);
  printDigitsLcd(myDT.hour());
  lcd.print(":");
  printDigitsLcd(myDT.minute());
  lcd.print(":");
  printDigitsLcd(myDT.second());
  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.print(temp);
  if (cooler_status) {
    lcd.write(byte(0));
    lcd.print("  ");
  } else if (heat_status) {
    lcd.write(byte(1));
  } else {
    lcd.print("   ");
  }
  lcd.setCursor(12, 1);
  lcd.print("H:");
  lcd.print(hum);
  if (dehumi_status) {
    lcd.write(byte(0));
  } else if (humi_status) {
    lcd.write(byte(1));
  } else {
    lcd.print("   ");
  }
  lcd.setCursor(0, 2);
  lcd.print("Mode:");
  if (state==1) {
    lcd.print("Dehydr.");
  } else if (state == 0) {
    lcd.print("Cooler ");
  } else if (state == 2) {
    lcd.print("OFF   ");
  }
  lcd.setCursor(13, 2);
  lcd.print("Day:");
  printDigitsLcd(timeReturn());
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  if (cooler_status){
    lcd.setCursor(1, 3);
    lcd.write(byte(3));
  }
  if (dehumi_status){
    lcd.setCursor(3, 3);
    lcd.write(byte(4));
  }
  if (heat_status){
    lcd.setCursor(5, 3);
    lcd.write(byte(2));
  }
  if (humi_status){
    lcd.setCursor(7, 3);
    lcd.write(byte(5));    
  }
  if (fanOnOff){
    lcd.setCursor(9, 3);
    lcd.write(byte(6));
  }
  // printDigitsLcd(cooler_status);
  // printDigitsLcd(dehumi_status);
  // printDigitsLcd(heat_status);
  // printDigitsLcd(humi_status);
}
void printDigitsLcd(byte digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  if (digits < 10)
    lcd.print('0');
  lcd.print(digits);
}
byte timeReturn() {
  byte dayResult = ((myDT.unixtime() - timeStamp) / 86400);
  return dayResult;
}
void compressor(byte i) {
  if (myDT.unixtime() >= compressorTs)   {
    if (i == HIGH) {
      digitalWrite(Pin[5], on);
      kompressor = true;
    } else if (i == LOW) {
      digitalWrite(Pin[5], off);
      if (kompressor == true) {
        compressorTs = (myDT.unixtime() + 120);
        delay(100);
        kompressor = false;
      }
    }
   
  }
}
void coolerCtrl (byte i){
  if (i == on){
    digitalWrite(Pin[0], on);
  } else {
    digitalWrite(Pin[0], off);
  }
  if (evTemp >= 5 && i == on){
    compressor(on);
  } else {
    compressor(off);
  }

}
  
void readSerial() {
  String error = " Wrong Input\n";
  String done = " DONE\n";
  int pause = 3000;
  String enaDis[2] = { "Enable\n", "Disable\n" };

   if (Serial.available() && Serial.parseInt() == 0) {
     Serial.flush();
     Serial.read();
     //Serial.end();
    // Serial.begin(1152000);
     Serial.print("MENU: \n1- Temperature Selection\n2- Humidity Selection\n3- Mode\n4- Reset Count Days\n5- Serial (Enable-Disable)\n6- Fan Speed %\n7- Circulation FAN (ON-OFF) \n\n");
    while (Serial.available()==0) {};
   
    if (Serial.available()) {
      while (!Serial.available());
    byte a = Serial.parseInt();
    if (a == 1) {
      Serial.print("Temp: ");
      while (!Serial.available()) {}
      EEPROM.update(0, Serial.parseInt());
      temperature = EEPROM.read(0);
      Serial.println(temperature);
      Serial.println(done);
      delay(pause);
    } else if (a == 2) {
      Serial.print("Hum: ");
      while (!Serial.available()) {}
      EEPROM.update(4, Serial.parseInt());
      humyditi = EEPROM.read(4);
      Serial.println(humyditi);
      Serial.println(done);
      delay(pause);
    } else if (a == 3) {
      Serial.print("Mode(0 - Cooler/Freeze, 1-Dehydration, 2-OFF): ");
      while (!Serial.available()) {}
      EEPROM.update(5, Serial.parseInt());
      state = EEPROM.read(5);
      if (state == 1) {
        Serial.println(" Dehydration ");
      } else if (state == 0) {
        Serial.println(" Cooler/Freeze ");
      } else if (state == 2) {
        Serial.println(" OFF ");
      }
      Serial.println(done);
      delay(pause);
    } else if (a == 4) {
      Serial.print("Reset Day?(0 - NO, 1-YES): ");
      while (!Serial.available()) {}
      if (Serial.parseInt()) {
        EEPROM.put(6, myDT.unixtime());
        EEPROM.get(6, timeStamp);
        Serial.print(" Day Counter Reset");
      } else {
        Serial.print("Day Counter Not Reset");
      }
      delay(pause);
    } else if (a == 5) {
      Serial.print("Serial(0 - OFF, 1-ON): ");
      while (!Serial.available()) {}
      EEPROM.update(10, Serial.parseInt());
      serialStatus = EEPROM.read(10);
      if (serialStatus == 0)
        Serial.println(enaDis[1]);
      else if (serialStatus == 1) {
        Serial.println(enaDis[0]);
      } else {
        Serial.println(error);
      }
    } else if (a == 6) {
      Serial.print("Fan Speed %");
      while (!Serial.available()) {}
      EEPROM.update(11, map(Serial.parseInt() ,1 , 100 ,26 ,255));
      fan_up_speed = EEPROM.read(11);
      Serial.print(map(fan_up_speed ,26 ,255 ,1 ,100));
      Serial.println(done);
      delay(pause);

    } else if (a == 7) {
      Serial.print("Circulation (0 - OFF, 1-ON): ");
      while (!Serial.available()) {}
      EEPROM.update(12, Serial.parseInt());
      fanOnOff = EEPROM.read(12);
      if (fanOnOff) {
        Serial.print(" ON");
      } else {
        Serial.print(" OFF");
      }
      Serial.println(done);
      delay(pause);
    }

    else {
      Serial.println(error);
      delay(pause);
    }
  }
   }
}