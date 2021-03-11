#define MY_NODE_ID 2
#define MY_RADIO_NRF24
#define MY_REPEATER_FEATURE
#include <MySensors.h>
#include <MyConfig.h>
#include <SPI.h>
#include <DHT.h>  
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//#include <DS1302RTC.h>
#include <TimeLib.h>
#include <Streaming.h>

// Init the DS1302
// Set pins:  CE, IO,CLK
DS1302RTC RTC(6, 7, 8);
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
LiquidCrystal_I2C lcd(0x27,16,2);

unsigned long teraz=millis();
unsigned long czas=teraz;

//na sztywno ustawione MySensors NODE_ID
//#define NODE_ID 2

#define CHILD_ID_HUM 101  //id czujnika wilgotności z dht
#define CHILD_ID_TEMP 100 //id temperatury z dht
                          //od strony kontrolera widać to szesnastkowo jako złożenie nodeId+childId
                          //tutaj 264 i 265
                          //wysokie numery dajemy ze względu na sposób numeracji czujników 1-wire (poniżej)
                          
#define CHILD_ID_LIGHT 50 //id czujnika oświetlenia
#define HUMIDITY_SENSOR_DIGITAL_PIN 3 //Do tego pinu podłączamy DHT22
#define ONE_WIRE_BUS 4 // A do tego 1-wire DS18B20
#define MAX_ATTACHED_DS18B20 16 //maksymalna ilość czujników na 1-wire

unsigned long SLEEP_TIME = 300000; // Sleep time between reads (in milliseconds)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
const int lightPin = A0; //wejście czujnika oświetlenia, fotorezystor do VCC i rezystor 10k do masy
const int lcdLed = 5; //sterowanie podświetleniem LCD, pobiera ok 15mA więc można podpiąć bezpośrednio

int unsigned lightValue = 0;
int unsigned lightMin = 0;
int unsigned lightMax = 1023;

DHT dht;
float lastTemp=-1;
float lastHum=-1;
float lastTemperature[MAX_ATTACHED_DS18B20];
unsigned long lastTimeReceived;
boolean dispOut=0; //czy wyświetlać tem zewn.
float lastTempOut=-1;
float lastHumOut=-1;
int x=0;
boolean isMessage = false;
#define TEMPOUT 100
#define HUMOUT 101


int numSensors=0;
boolean receivedConfig = false;
boolean metric = true;
unsigned long lastProbe=0;
MyMessage msg(0,V_TEMP);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgLight(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
int lastLightLevel;
boolean timeReceived=false;

void setup()  
{ 
//Serial.println("Startujemy...");
//Ustawienie podświetlenia zależnie od natężenia światła
  lightValue = analogRead(lightPin);
  // apply the calibration to the sensor reading
  lightValue = map(lightValue, lightMin, lightMax, 0, 255);
  // in case the sensor value is outside the range seen during calibration
  lightValue = constrain(lightValue, 0, 255);
  // fade the LCD using the calibrated value:
  analogWrite(lcdLed, lightValue);
//  Serial.println(lightValue);
//Inicjalizacja zegara RTC, tutaj trzeba dostosować kod zależnie od posiadanego modułu  
  lcd.begin(16,2);
  lcd.init();
  lcd.backlight();
/*  lcd.print("RTC activated");
  delay(1000);
  lcd.clear();
  if (RTC.haltRTC())
    lcd.print("Clock stopped!");
  else
    lcd.print("Clock working.");
  // Check write-protection
  lcd.setCursor(0,1);
  if (RTC.writeEN())
    lcd.print("Write allowed.");
  else
    lcd.print("Write protected.");
  delay ( 1000 );
  // Setup Time library  
  lcd.clear();
  lcd.print("RTC Sync");
  setSyncProvider(RTC.get); // the function to get the time from the RTC
  if(timeStatus() == timeSet)
    lcd.print(" Ok!");
  else
    lcd.print(" FAIL!");
  delay ( 1000 );
*/
//  lcd.clear();
  // Request latest time from controller at startup
  lcd.print("Pobieram czas...");
  lcd.print("...z kontrolera");
  requestTime(receiveTime);
//  Serial.println("Pobieram czas...");

//  sensors.begin();

  // Startup and initialize MySensors library. Set callback for incoming messages. 
  begin(NULL,NODE_ID,false);
  
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Dallas_DHT22_LCD_relay", "2.3.1");

  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

// Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);

  // Pobieramy ilość podłączonych czujników 1-wire (DS18B20)
  numSensors = sensors.getDeviceCount();
  // Startup OneWire 
  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     present(i, S_TEMP);
  }
  
  // Present light sensor to controller
  present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  
  metric = getControllerConfig().isMetric;
  //requestTime(receiveTime);  
  delay(2000);
////  lcd.clear();
  lastProbe = millis();
  incomingMessage();

}

// This is called when a new time value was received
void receiveTime(unsigned long controllerTime) {
  // Ok, set incoming time 
  controllerTime=controllerTime; //Dodajemy latem 2h a zimą 1h bo kontroler podaje czas UTC
  lcd.print(controllerTime);
//  Serial.println(controllerTime);
  setTime(controllerTime);
//  RTC.set(controllerTime); // this sets the RTC to the time from controller - which we do want periodically
  timeReceived = true;
  lastTimeReceived=millis();
}

void loop()      
{  
  unsigned long now = millis();

 if (lastTimeReceived < now-3600000) gw.requestTime(receiveTime); //aktualizacja czasu co godzinę

//Automatyczna regulacja podświetlenia
  lightValue = analogRead(lightPin);
  // apply the calibration to the sensor reading
  lightValue = map(lightValue, lightMin, lightMax, 0, 255);
  // in case the sensor value is outside the range seen during calibration
  lightValue = constrain(lightValue, 0, 255);
  // fade the LCD using the calibrated value:
  analogWrite(lcdLed, lightValue);
  analogWrite(lcdLed, 200);
  // Process incoming messages (like config from server)
//Tę część wykonujemy co SLEEP_TIME, tu akurat odczytujemy czujniki i wysyłamy do kontrolera
  if (now-lastProbe > SLEEP_TIME) {
    lastProbe = now;

    // Fetch temperatures from Dallas sensors
    sensors.requestTemperatures(); 
    // Read temperatures and send them to controller 
    for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
      // Fetch and round temperature to one decimal
      float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
//      Serial.print("T");
//      Serial.print(i);
//      Serial.print(": ");
//     Serial.println(temperature);
      // Only send data if temperature has changed and no error
//      if (lastTemperature[i] != temperature && temperature != -127.00) {
      if (temperature != -127.00) {
        // Send in the new temperature
        send(msg.setSensor(i).set(temperature,1));
        lastTemperature[i]=temperature;
      }
    }
    
//    delay(dht.getMinimumSamplingPeriod());  //tego używany tylko jeśli node jest usypiany przez gw.sleep
    float temperature = dht.getTemperature();
    if (isnan(temperature)) {
//      Serial.println("Błąd odczytu temperatury z DHT");
//    } else if (temperature != lastTemp) {
    } else {
      lastTemp = temperature;
      if (!metric) {
        temperature = dht.toFahrenheit(temperature);
      }
      send(msgTemp.set(temperature, 1));
//      Serial.print("T: ");
//      Serial.println(temperature);
    }
  
    float humidity = dht.getHumidity();
    if (isnan(humidity)) {
//        Serial.println("Błąd odczytu wilgotn. z DHT");
        } else if (humidity != lastHum) {
    } else {
        lastHum = humidity;
        send(msgHum.set(humidity, 1));
//        Serial.print("H: ");
//        Serial.println(humidity);
    }

    int lightLevel = analogRead(LIGHT_SENSOR_ANALOG_PIN); 
//    Serial.print("Light: ");
//    Serial.println(lightValue);
    if (lightValue != lastLightLevel) {
      send(msgLight.set(lightValue));
      lastLightLevel = lightValue;
    }

  //Jeśli nie pobrano czasu z kontrolera to odpytuj co SLEEP_TIME
requestTime(receiveTime);
  }
//Koniec części wykonywanej co SLEEP_TIME

  //Aktualizujemy czas na LCD (co 1s)
  
  czas=millis();
  if (czas-teraz>=1000)
  {
    teraz=(czas);
    x=x+1;
    if (x>4){
      dispOut=1;
    }
    if (x>8){
      dispOut=0;
      x=0;
    }
    //Tu wyświetlamy czas i datę
    lcd.clear();
    lcd.setCursor(0, 1);
    print2digits(hour());
    lcd.print(":");
    print2digits(minute());
    lcd.print(":");
    print2digits(second());
    // Display date in the lower right corner
    lcd.setCursor(6, 1);
      lcd.print(" ");
    print2digits(day());
    lcd.print("/");
    print2digits(month());
    lcd.print("/");
    lcd.print(year());
//    Display abbreviated Day-of-Week in the lower left corner
    lcd.print(dayShortStr(weekday()));
    
    if (dispOut==0){  
      lcd.setCursor(0, 0);
      lcd.print("i:");
      lcd.print(lastTemp,1);
      lcd.print(" o:");
      lcd.print(lastTempOut,1);
      lcd.write(223); // Degree-sign
      lcd.print("C");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Wilg i/o:");
      lcd.print(lastHum,0);
      lcd.print("% ");
      lcd.print(lastHumOut,0);
      lcd.print("%");
    }
lcd.setCursor(15,0);
lcd.print(x); 
    // Warning!
    if(timeStatus() != timeSet) {
      lcd.setCursor(0, 1);
      lcd.print(F("RTC ERROR: SYNC!"));
    }
  }

//gw.sleep(SLEEP_TIME); //Nie usypiamy noda bo ma działać cały czas

}

void printDigits(int digits){
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}

void print2digits(int number) {
  // Output leading zero
  if (number >= 0 && number < 10) {
    lcd.write('0');
  }
  lcd.print(number);
}

//Odbieramy odczyty z czujnika zewnętrznego (NODE_ID 3)
void receive(const MyMessage &message) {
  switch (message.sender) {
        case 3:
             switch (message.sensor){
                   case 100: //temperatura na zewnątrz z DHT
                         lastTempOut = message.getFloat();
                  break; 
                  case 101: //wilgotność na zewnątrz z DHT
                         lastHumOut = message.getFloat();
                  break;
          } 
          break; 
  }
}
