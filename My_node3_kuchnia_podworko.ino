// Enable debug prints to serial monitor
//#define MY_DEBUG 
// Enable and select radio type attached
#define MY_RADIO_NRF24


#include <SPI.h>
#include <MySensors.h>
#include <DHT.h>  
#include <DallasTemperature.h>
#include <OneWire.h>

#define NODE_ID 3
#define CHILD_ID_HUM 101
#define CHILD_ID_TEMP 100
#define DHT_DATA_PIN 3
#define ONE_WIRE_BUS 4 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16
unsigned long SLEEP_TIME = 300000; // Sleep time between reads (in milliseconds)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//MySensor gw;
float lastTemp;
float lastHum;
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;
boolean receivedConfig = false;
boolean metric = true; 
MyMessage msg(0,V_TEMP);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
DHT dht;

void presentation()  {
  // Startup OneWire 
  delay(NODE_ID*3000);
 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Dallas_DHT22", "2.3.2");

  sensors.begin();
  // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     present(i, S_TEMP);
  }
  
  metric = getControllerConfig().isMetric;
}

void setup()  
{ 
    dht.setup(DHT_DATA_PIN); // set data pin of DHT sensor
  if (SLEEP_TIME <= dht.getMinimumSamplingPeriod()) {
    Serial.println("Warning: UPDATE_INTERVAL is smaller than supported by the sensor!");
  }
  // Sleep for the time of the minimum sampling period to give the sensor time to power up
  // (otherwise, timeout errors might occure for the first reading)
  sleep(dht.getMinimumSamplingPeriod());

}

void loop()      
{  
  // Process incoming messages (like config from server)
//  process(); 

  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures(); 

  // Read temperatures and send them to controller 
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
 
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
//    Serial.print("T");
//    Serial.print(i);
//    Serial.print(": ");
//    Serial.println(temperature);
 
    // Only send data if temperature has changed and no error
  //  if (lastTemperature[i] != temperature && temperature != -127.00) {
 
      // Send in the new temperature
      send(msg.setDestination(0).setSensor(i).set(temperature,1));
      lastTemperature[i]=temperature;
//    }
  }

  sleep(dht.getMinimumSamplingPeriod());

  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
  } else {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    send(msgTemp.setDestination(0).setSensor(100).set(temperature,1));
    send(msgTemp.setDestination(2).setSensor(100).set(temperature,1));
//    Serial.print("T: ");
//    Serial.println(temperature);
  }
  
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else {
      lastHum = humidity;
      send(msgHum.setDestination(0).setSensor(101).set(humidity,1));
      send(msgHum.setDestination(2).setSensor(101).set(humidity,1));
//      Serial.print("H: ");
//      Serial.println(humidity);
  }

  sleep(SLEEP_TIME); //sleep a bit
}
