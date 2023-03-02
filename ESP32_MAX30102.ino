#include <Wire.h>
#include "MAX30105.h" 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>

#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);
MAX30105 particleSensor;
#define fSpO2 0.7 //filter factor for estimated SpO2
#define fRate 0.95 //low pass filter for IR/red LED value to eliminate AC component
#define fbpmrate 0.95 // low pass filter coefficient for HRM in bpm
#define firrate 0.85 //IR filter coefficient to remove notch , should be smaller than fRate
float aveRed = 0;//DC component of RED signal
float aveIr = 0;//DC component of IR signal
float sumIrRMS = 0; //sum of IR square
float sumRedRMS = 0; // sum of RED square
unsigned int counter = 0; //loop counter
float eSpO2 = 95.0;//initial value of estimated SpO2
float Ebpm;//estimated Heart Rate (bpm)
byte validData; 
float Ebpm_Ar[5];
float eSpO2_Ar[5];
void initMAX30102() 
{
  byte  ledBrightness = 0x7F;   //Options: 0=Off to 255=50mA
  byte  sampleAverage = 4;      //Options: 1, 2, 4, 8, 16, 32
  byte  ledMode       = 2;      //Options: 1 = IR only, 2 = Red + IR on MH-ET LIVE MAX30102 board
  int   sampleRate    = 200;    //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int   pulseWidth    = 411;    //Options: 69, 118, 215, 411
  int   adcRange      = 16384;  //Options: 2048, 4096, 8192, 16384
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("Please check wiring/power/solder jumper at MAX30102 board. ");
  }
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
}
void setup()
{
  // Start serial communication 
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // Init BME Sensor
  initMAX30102();
}
float HRM_estimator(float fir ,float aveIr)
{
  static uint32_t crosstime = 0; //falling edge , zero crossing time in msec
  static uint32_t crosstime_prev = 0;//previous falling edge , zero crossing time in msec
  static float bpm = 60.0;
  static float ebpm = 60.0;
  static float eir = 0.0; //estimated lowpass filtered IR signal to find falling edge without notch
  static float eir_prev = 0.0;
  // Heart Rate Monitor by finding falling edge
  eir = eir * firrate + fir * (1.0 - firrate); //estimated IR : low pass filtered IR signal
  if ( ((eir - aveIr) * (eir_prev - aveIr) < 0 ) && ((eir - aveIr) < 0.0)) 
  { 
    crosstime = millis();
    if ( ((crosstime - crosstime_prev ) > (60 * 1000 / 180)) && ((crosstime - crosstime_prev ) < (60 * 1000 / 45)) ) 
    {
      bpm = 60.0 * 1000.0 / (float)(crosstime - crosstime_prev) ; //get bpm
      ebpm = ebpm * fbpmrate + (1.0 - fbpmrate) * bpm;//estimated bpm by low pass filtered
    } 
    crosstime_prev = crosstime;
  }
  eir_prev = eir;
  return (ebpm);
}
void measure()
{
  uint32_t ir, red ;//raw data
  float fred, fir; //floating point RED ana IR raw values
  float SpO2 = 0; //raw SpO2 before low pass filtered
  particleSensor.check(); 
  while (particleSensor.available()) 
  {
    red = particleSensor.getFIFOIR(); 
    ir = particleSensor.getFIFORed(); 
    counter++;
    fred = (float)red;
    fir = (float)ir;
    aveRed = aveRed * fRate + (float)red * (1.0 - fRate);//average red level by low pass filter
    aveIr = aveIr * fRate + (float)ir * (1.0 - fRate); //average IR level by low pass filter
    sumRedRMS += (fred - aveRed) * (fred - aveRed); //square sum of alternate component of red level
    sumIrRMS += (fir - aveIr) * (fir - aveIr);//square sum of alternate component of IR level
    Ebpm = HRM_estimator(fir, aveIr); //Ebpm is estimated BPM
    if (!(counter % 20)) 
    {
      if ( millis() > 6000) 
      {
        if ( ir < 50000)
        {
          validData=0;
        }
        else
        {
          validData=1;    
          for(int i=0;i<5;i++)
          {
            Ebpm_Ar[i]=Ebpm;
            eSpO2_Ar[i]=eSpO2;
          } 
        }
      }
    }
    if (!(counter % 100)) 
    {
      float R = (sqrt(sumRedRMS) / aveRed) / (sqrt(sumIrRMS) / aveIr);
      SpO2 = -23.3 * (R - 0.4) + 100 ; 
      if (SpO2 > 100.0 ) SpO2 = 100.0;
      eSpO2 = fSpO2 * eSpO2 + (1.0 - fSpO2) * SpO2;
      sumRedRMS = 0.0; 
      sumIrRMS = 0.0; 
      counter = 0;
      break;
    }
    particleSensor.nextSample();
  }
}
void display_human_data()
{
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  if(validData)
  { 
    display.println("Human data");
    display.setCursor(0,16);
    display.printf("BPM=%0.2f",Ebpm);
    display.setCursor(0,24);
    display.printf("Spo2=%0.2f",eSpO2);
    char humidity[72]={'\0'};
    char h1[35];
    char h2[35];
    char s[2]={0x5f, 0x5f};
    char *loc1 = h1;
    char *loc2 = h2;
    floatToString(Ebpm_Ar,loc1);
    floatToString(eSpO2_Ar,loc2);
    strncat(humidity,h1,35);
    strncat(humidity,s,2);
    strncat(humidity,h2,35);
//    Serial.printf("%s\r\n",h1);
//    Serial.printf("%s\r\n",h2);
    Serial.printf("%s\r\n",humidity);
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
void floatToString(float testFloats[5], char *loc)
{
    char testBuffer[255] = {'\0'};
    size_t testBufferSpace = 255;
    size_t tempLen;
    for(int i = 0; i < 5; ++i)
    {
        snprintf(loc, testBufferSpace, "%0.1f ", testFloats[i]);
        tempLen = strlen(loc);
        loc += tempLen;
    }
}
void loop()
{
  measure();
  display_human_data();
}
