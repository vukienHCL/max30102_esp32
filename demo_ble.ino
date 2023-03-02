#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Wire.h>
#include "MAX30105.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "heartRate.h"

#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);
MAX30105 particleSensor;


const byte RATE_SIZE = 8; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
long start_t = 0;
long end_t = 0;

float beatsPerMinute;
int beatAvg;
float temp;
float hum;
int flag_c; 
int spo2;
struct human_data
{
  int Hr;
  int Spo2;
};

human_data data;

//Default Temperature is in Celsius
//Comment the next line for Temperature in Fahrenheit


//BLE server name
#define bleServerName "MAX30102_ESP32"
#define temperatureCelsius



// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;
unsigned long start = 0;
unsigned long endt =0;


bool deviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

//#ifdef temperatureCelsius
  BLECharacteristic TemperatureCelsiusCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor TemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));
//  #else
//  BLECharacteristic TemperatureFahrenheitCharacteristics("f78ebbff-c8b7-4107-93de-889a6a06d408", BLECharacteristic::PROPERTY_NOTIFY);
//  BLEDescriptor TemperatureFahrenheitDescriptor(BLEUUID((uint16_t)0x2901));
//  #endif
  
  BLECharacteristic HumidityCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor HumidityDescriptor(BLEUUID((uint16_t)0x2903));
// Spo2 Characteristic and Descripto
  BLECharacteristic Spo2Characteristics("f78ebbff-c8b7-4107-93de-889a6a06d408", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor Spo2Descriptor(BLEUUID((uint16_t)0x2901));

 bool device_connected = false;

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
   

    
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
      ESP.restart();
  }
};

void initMAX30102(){
   if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0);
//  particleSensor.setup(0); //Configure sensor. Turn off LEDs
  particleSensor.enableDIETEMPRDY();
}

void measureHR()
  {
    long irValue = particleSensor.getIR();
    int y = 0;
    float temperature = 25;
    float temperatureF = 300;
   
  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }
   if (irValue< 50000)
   {
      flag_c = 0;
      HumidityCharacteristics.setValue("No finger");
      HumidityCharacteristics.notify();
   }
   else
      {
        flag_c =1;
        static char humidity[7];
        dtostrf(beatAvg, 6, 1, humidity);
        HumidityCharacteristics.setValue(humidity);
        HumidityCharacteristics.notify();
      }
           
      //Notify humidity reading from BME
    
    
  }


void MeasureSpo2()
{
  spo2 = 95;
 
 }

 void display_human_data(float hr, float sp, int flag)
 {
   if(flag == 1)
   {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Human data");

    display.setCursor(0,16);
    display.println("BPM=");
    display.setCursor(24,16);
    display.print(hr);

    display.setCursor(0,24);
    display.println("Spo2=");
    display.setCursor(36,24);
    display.print(sp);

   }
   else
   {
     display.clearDisplay();
     display.setCursor(0,0);
     display.println("No finger!Please place your finger");

   }
  display.display();
  display.clearDisplay();   
 }
 
void setup() {
  // Start serial communication 
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // Init BME Sensor
  initMAX30102();

  // Create the BLE Device
  BLEDevice::init(bleServerName);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *max30102Service = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics and Create a BLE Descriptor

  // Humidity
  max30102Service->addCharacteristic(&HumidityCharacteristics);
  HumidityDescriptor.setValue("BME humidity");
  HumidityCharacteristics.addDescriptor(new BLE2902());

  // Spo2
//  max30102Service->addCharacteristic(&Spo2Characteristics);
//  Spo2Descriptor.setValue("spo2");
//  Spo2Characteristics.addDescriptor(new BLE2902());
  
  // Start the service
  max30102Service->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

   measureHR();
   MeasureSpo2();
  if (deviceConnected) {

 if (flag_c == 0)
   {
      HumidityCharacteristics.setValue("No finger");
      HumidityCharacteristics.notify();
   }
   else
      {
        static char humidity[7];
        dtostrf(beatAvg, 6, 1, humidity);
        HumidityCharacteristics.setValue(humidity);
        HumidityCharacteristics.notify();
        // static char spo2_data[7];
        // dtostrf(spo2, 6, 1, spo2_data);
        // Spo2Characteristics.setValue(spo2_data);
        // Spo2Characteristics.notify(); 
      }
      display_human_data(beatAvg, spo2, flag_c);
  }
  display_human_data(beatAvg, spo2, flag_c);

}
