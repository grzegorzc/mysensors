//#define MY_DEBUG
//na sztywno ustawione MySensors NODE_ID
#define MY_NODE_ID 2
#define MY_RADIO_RF24
#define MY_REPEATER_FEATURE
#include <MySensors.h>
//#include <MyConfig.h>
//#include <Wire.h>
//#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <DHT.h>
#include <DallasTemperature.h>
//#include <OneWire.h>
#include <TimeLib.h>
//#include <Streaming.h>

#define OLED_RESET -1
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

unsigned long teraz = millis();
unsigned long czas = teraz;


#define CHILD_ID_HUM 101  //id czujnika wilgotności z dht
#define CHILD_ID_TEMP 100 //id temperatury z dht
//od strony kontrolera widać to szesnastkowo jako złożenie nodeId+childId
//tutaj 264 i 265
//wysokie numery dajemy ze względu na sposób numeracji czujników 1-wire (poniżej)

#define HUMIDITY_SENSOR_DIGITAL_PIN 3 //Do tego pinu podłączamy DHT22
#define ONE_WIRE_BUS 4 // A do tego 1-wire DS18B20
#define MAX_ATTACHED_DS18B20 4 //maksymalna ilość czujników na 1-wire

unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DHT dht;
float lastTemp = -1;
float lastHum = -1;
float lastTemperature[MAX_ATTACHED_DS18B20];
unsigned long lastTimeReceived;
//boolean dispOut = 0; //czy wyświetlać tem zewn.
float lastTempOut = -1;
float lastHumOut = -1;
//int x = 0;
boolean isMessage = false;
#define TEMPOUT 100
#define HUMOUT 101

int numSensors = 0;
boolean receivedConfig = false;
boolean metric = true;
unsigned long lastProbe = 0;
MyMessage msg(0, V_TEMP);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
boolean timeReceived = false;

void before()
{
  // Startup up the OneWire library
  sensors.begin();
}
void setup()
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, OLED_RESET)) { // Address 0x3D for 128x64
    Serial.println(F("E"));
  //  for(;;); // Don't proceed, loop forever
  }
  
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  //display.display();
  //delay(1000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
}

void presentation() {

  // Startup and initialize MySensors library. Set callback for incoming messages.
  //  begin(NULL,NODE_ID,false);

  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("NOD2", "231");

  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);

  // Pobieramy ilość podłączonych czujników 1-wire (DS18B20)
  numSensors = sensors.getDeviceCount();
  // Startup OneWire
  // Present all sensors to controller
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    present(i, S_TEMP);
  }
 display.clearDisplay();
  // Present light sensor to controller
//  present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);

//  metric = getControllerConfig().isMetric;
  requestTime(receiveTime);
//  delay(2000);
  ////  display.clear();
  lastProbe = millis();
  //  incomingMessage();

}

// This is called when a new time value was received
void receiveTime(unsigned long controllerTime) {
  // Ok, set incoming time
  controllerTime=controllerTime; //Dodajemy latem 2h a zimą 1h bo kontroler podaje czas UTC
//  display.print(controllerTime);
//  display.display();
//  Serial.println(controllerTime);
  setTime(controllerTime);
  //  RTC.set(controllerTime); // this sets the RTC to the time from controller - which we do want periodically
  timeReceived = true;
  lastTimeReceived = millis();
}

void loop()
{
  unsigned long now = millis();

  //if (lastTimeReceived < now-3600000) gw.requestTime(receiveTime); //aktualizacja czasu co godzinę

  // Process incoming messages (like config from server)
  // Tę część wykonujemy co SLEEP_TIME, tu akurat odczytujemy czujniki i wysyłamy do kontrolera
  if (now - lastProbe > SLEEP_TIME) {
    lastProbe = now;

    // Fetch temperatures from Dallas sensors
    sensors.requestTemperatures();
    // Read temperatures and send them to controller
    for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
      // Fetch and round temperature to one decimal
//      float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
      float temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(i)) * 10.) / 10.;
//      float temperature = ((sensors.getTempCByIndex(i)));
//            Serial.print("T");
//            Serial.print(i);
//            Serial.print(": ");
//            Serial.println(temperature);
      // Only send data if temperature has changed and no error
      //      if (lastTemperature[i] != temperature && temperature != -127.00) {
      if (temperature != -127.00 && temperature != 85.00) {
        // Send in the new temperature
        send(msg.setSensor(i).set(temperature, 1));
        lastTemperature[i] = temperature;
      }
    }

    //    delay(dht.getMinimumSamplingPeriod());  //tego używany tylko jeśli node jest usypiany przez gw.sleep
    float temperature = dht.getTemperature();
    if (isnan(temperature)) {
//            Serial.println("Błąd odczytu temperatury z DHT");
      //    } else if (temperature != lastTemp) {
    } else {
      lastTemp = temperature;
//      if (!metric) {
//        temperature = dht.toFahrenheit(temperature);
//      }
      send(msgTemp.set(temperature, 1));
//            Serial.print("T: ");
//            Serial.println(temperature);
    }

    float humidity = dht.getHumidity();
    if (isnan(humidity)) {
//              Serial.println("Błąd odczytu wilgotn. z DHT");
      //    } else if (humidity != lastHum) {
    } else {
      lastHum = humidity;
      send(msgHum.set(humidity, 1));
//              Serial.print("H: ");
//              Serial.println(humidity);
    }

    //    int lightLevel = analogRead(LIGHT_SENSOR_ANALOG_PIN);
    //    Serial.print("Light: ");
    //    Serial.println(lightValue);
    //    if (lightValue != lastLightLevel) {
  //  send(msgLight.set(lightValue));
  //  lastLightLevel = lightValue;
    //    }

    //Jeśli nie pobrano czasu z kontrolera to odpytuj co SLEEP_TIME
    requestTime(receiveTime);
  }
  //Koniec części wykonywanej co SLEEP_TIME

  //Aktualizujemy czas na LCD (co 1s)


  czas = millis();
  if (czas - teraz >= 1000)
  {
    teraz = (czas);

/**    x = x + 1;
    if (x > 4) {
      dispOut = 1;
    }
    if (x > 8) {
      dispOut = 0;
      x = 0;
    }**/
 
    //Tu wyświetlamy czas i datę
      display.setTextSize(2);      // Normal 1:1 pixel scale
      display.setTextColor(WHITE); // Draw white text
      display.setCursor(0, 0);     // Start at top-left corner
      display.cp437(true);         // Use full 256 char 'Code Page 437' font
      display.clearDisplay();
      //display.setCursor(0, 1);
//      print2digits(hour());
//      display.print(":");
//      print2digits(minute());
//      display.println("");
//      display.print(":");
//      print2digits(second());

/**    // Display date in the lower right corner
      display.setCursor(6, 1);
      display.print(" ");
      print2digits(day());
      display.print("/");
      print2digits(month());
      display.print("/");
      display.print(year());
    // Display abbreviated Day-of-Week in the lower left corner
      display.print(dayShortStr(weekday()));
**/


//        if (dispOut==0){
          display.setCursor(0, 0);
          display.print("T i:");
          display.print(lastTemp,1);
          display.write(223); // Degree-sign
          display.print("C");
          display.setCursor(10, 21);
          display.print("o:");
          display.print(lastTempOut,1);
          display.write(223); // Degree-sign
          display.print("C");
//        } else {
          display.setCursor(0, 42);
          display.print("H i:");
          display.print(lastHum,0);
          display.print("%");
//          display.setCursor(20, 48);
//          display.print("o:");
//          display.print(lastHumOut,0);
//          display.print("%");
          display.display();
//        }

//    display.setCursor(15,0);
//    display.print(x);
    display.display();    
    // Warning!
//        if(timeStatus() != timeSet) {
//          display.setCursor(0, 1);
//          display.print(F("RTC ERROR: SYNC!"));
//        }

  }
   

  //gw.sleep(SLEEP_TIME); //Nie usypiamy noda bo ma działać cały czas

}

/**
void printDigits(int digits) {
    if(digits < 10)
      display.print('0');
    display.print(digits);
    display.display();
}**/

void print2digits(int number) {
  // Output leading zero
  if (number >= 0 && number < 10) {
        display.write('0');
  }
    display.print(number);
    display.display();
}


//Odbieramy odczyty z czujnika zewnętrznego (NODE_ID 3)
void receive(const MyMessage &message) {
  switch (message.sender) {
    case 3:
      switch (message.sensor) {
        case 100: //temperatura na zewnątrz z DHT
          lastTempOut = message.getFloat();
          break;
      //  case 101: //wilgotność na zewnątrz z DHT
      //    lastHumOut = message.getFloat();
      //    break;
      }
      break;
  }
}



/// Tutaj mamy przykłądowe testy grafiki OLED

/**
void testdrawline() {
  int16_t i;

  display.clearDisplay(); // Clear display buffer

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, WHITE);
    display.display(); // Update screen with each newly-drawn line
    delay(1);
  }
  for(i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for(i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();

  for(i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, WHITE);
    display.display();
    delay(1);
  }
  for(i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, WHITE);
    display.display();
    delay(1);
  }

  delay(2000); // Pause for 2 seconds
}
**/

/**
void testdrawrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, WHITE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}
**/

/**
void testfillrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=3) {
    // The INVERSE color is used so rectangles alternate white/black
    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, INVERSE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}
**/

/**
void testdrawcircle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}
**/

/**
void testfillcircle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=3) {
    // The INVERSE color is used so circles alternate white/black
    display.fillCircle(display.width() / 2, display.height() / 2, i, INVERSE);
    display.display(); // Update screen with each newly-drawn circle
    delay(1);
  }

  delay(2000);
}
**/


/**
void testdrawroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}
**/


/**
void testfillroundrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2-2; i+=2) {
    // The INVERSE color is used so round-rects alternate white/black
    display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i,
      display.height()/4, INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}
**/


/**
void testdrawtriangle(void) {
  display.clearDisplay();

  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=5) {
    display.drawTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, WHITE);
    display.display();
    delay(1);
  }

  delay(2000);
}
**/

/**
void testfilltriangle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=5) {
    // The INVERSE color is used so triangles alternate white/black
    display.fillTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}
**/


/**
void testdrawchar(void) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  for(int16_t i=0; i<256; i++) {
    if(i == '\n') display.write(' ');
    else          display.write(i);
  }

  display.display();
  delay(2000);
}
**/


/**
void testdrawstyles(void) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Hello, world!"));

  display.setTextColor(BLACK, WHITE); // Draw 'inverse' text
  display.println(3.141592);

  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.print(F("0x")); display.println(0xDEADBEEF, HEX);

  display.display();
  delay(2000);
}
**/



/**
void testscrolltext(void) {
  display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println(F("scroll"));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}
**/


/**
void testdrawbitmap(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}
**/
/**
#define XPOS   0 // Indexes into the 'icons' array in function below
#define YPOS   1
#define DELTAY 2

void testanimate(const uint8_t *bitmap, uint8_t w, uint8_t h) {
  int8_t f, icons[NUMFLAKES][3];

  // Initialize 'snowflake' positions
  for(f=0; f< NUMFLAKES; f++) {
    icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
    icons[f][YPOS]   = -LOGO_HEIGHT;
    icons[f][DELTAY] = random(1, 6);
    Serial.print(F("x: "));
    Serial.print(icons[f][XPOS], DEC);
    Serial.print(F(" y: "));
    Serial.print(icons[f][YPOS], DEC);
    Serial.print(F(" dy: "));
    Serial.println(icons[f][DELTAY], DEC);
  }

  for(;;) { // Loop forever...
    display.clearDisplay(); // Clear the display buffer

    // Draw each snowflake:
    for(f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, WHITE);
    }

    display.display(); // Show the display buffer on the screen
    delay(200);        // Pause for 1/10 second

    // Then update coordinates of each flake...
    for(f=0; f< NUMFLAKES; f++) {
      icons[f][YPOS] += icons[f][DELTAY];
      // If snowflake is off the bottom of the screen...
      if (icons[f][YPOS] >= display.height()) {
        // Reinitialize to a random position, just off the top
        icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
        icons[f][YPOS]   = -LOGO_HEIGHT;
        icons[f][DELTAY] = random(1, 6);
      }
    }
  }
}
**/
