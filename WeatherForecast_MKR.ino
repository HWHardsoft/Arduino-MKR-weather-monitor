
/*
 *  Application note: Weather monitor for ArduiTouch MKR  
 *                    (Arduino MKR 1010 and MKR ENV Shield are needed)
 *  Version 1.1
 *  Copyright (C) 2019  Hartmut Wendt  www.zihatec.de
 *  
 *  based on sources of Art Deco Weather Forecast Display: http://www.educ8s.tv  
 *  
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */   

#include <SPI.h>
// uncomment the following line for Arduino MKR1000
// #include <WiFi101.h>

// uncomment the following line for Arduino MKR1010 
#include <WiFiNINA.h>


#include <ArduinoJson.h>
#include "Adafruit_ILI9341.h"
#include <Adafruit_GFX.h>
#include <MKRENV.h>

/*______Define colors _______*/
// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
#define GREY     0xC618
/*_______End of color definitions______*/

/*______Define pins for ArduiTouch _______*/
#define TFT_CS   A3
#define TFT_DC   0
#define TFT_MOSI 8
#define TFT_CLK  9
#define TFT_MISO 10
#define TFT_LED  A2  
/*_______End of pin definitions______*/


/*______Wifi definitions_______*/
char* ssid     = "yourssid";      // SSID of local network
char* password = "yourpassword";   // Password on network

int status = WL_IDLE_STATUS;
WiFiClient client;
char servername[]="api.openweathermap.org";  // remote server we will connect to
/*_______End of Wifi definitions______*/

/*______Weather definitions_______*/
String APIKEY = "your_api_key";
String CityID = "your_city_id"; //your city
int TimeZone = 1; //GMT +2
String weatherDescription ="";
String weatherLocation = "";
float Temperature;
boolean night = false;
/*_______End of Weather definitions______*/


String result;
int  counter = 10;

extern  unsigned char  cloud[];
extern  unsigned char  thunder[];
extern  unsigned char  wind[];

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);


void setup() {
  delay(3000);
  Serial.begin(115200);
  
  // initialize the TFT
  tft.begin();          
  tft.setRotation(3);   // landscape mode  
  tft.fillScreen(BLACK);// clear screen 

  tft.setCursor(70,110);
  tft.setTextColor(WHITE,BLACK);
  tft.setTextSize(2);
  tft.print("Connecting...");

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LOW);    // low to turn TFT backlight on;

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, password);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");

  // init MKR ENV shield
  if (!ENV.begin()) {
    Serial.println("Failed to initialize MKR ENV shield!");
    while (1);
  }


}

void loop() {

    if(counter == 10) //Get new data every 30 minutes
    {
      counter = 0;
      getWeatherData();
      getMKRENVData();
      
    }else
    {
      counter++;
      delay(5000);
      Serial.println(counter); 
    }
}


/********************************************************************//**
 * @brief     readout MRK ENV Shield and show the result on the screen
 * @param[in] None
 * @return    None
 *********************************************************************/
void getMKRENVData()
{
  String sensor_value;
  
  // temperature  
  sensor_value = ENV.readTemperature();
  Serial.print("Temperature = ");
  Serial.print(sensor_value);
  Serial.println(" °C");
  tft.setCursor(160,10);
  tft.setTextSize(2);
  tft.print("Temperature:");
  tft.setCursor(160,40);
  tft.setTextSize(3);
  tft.print(sensor_value + "  C");
  tft.setCursor(273,33);
  tft.setTextSize(2);
  tft.print("o");

  // humidity
  sensor_value = ENV.readHumidity();
  Serial.print("Humidity    = ");
  Serial.print(sensor_value);
  Serial.println(" %");
  tft.setCursor(160,90);
  tft.setTextSize(2);
  tft.print("Humidity:");
  tft.setCursor(160,120);
  tft.setTextSize(3);
  tft.print(sensor_value + " %");

  // air pressure
  sensor_value = ENV.readPressure();
  Serial.print("Pressure    = ");
  Serial.print(sensor_value);
  Serial.println(" kPa");
  tft.setCursor(160,170);
  tft.setTextSize(2);
  tft.print("Pressure:");
 
  tft.setCursor(160,200);
  tft.setTextSize(3);
  tft.print(sensor_value);
  tft.setCursor(280,205);
  tft.setTextSize(2);
  tft.print("kPa");
  
}


/********************************************************************//**
 * @brief     client function to send/receive GET request data 
 * @param[in] None
 * @return    None
 *********************************************************************/
void getWeatherData() 
{
  String result ="";
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(servername, httpPort)) {
        return;
    }
      // We now create a URI for the request
    String url = "/data/2.5/forecast?id="+CityID+"&units=metric&cnt=1&APPID="+APIKEY;

       // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + servername + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server
    while(client.available()) {
        result = client.readStringUntil('\r');
    }

    result.replace('[', ' ');
    result.replace(']', ' ');

    char jsonArray [result.length()+1];
    result.toCharArray(jsonArray,sizeof(jsonArray));
    jsonArray[result.length() + 1] = '\0';

    StaticJsonBuffer<1024> json_buf;
    JsonObject &root = json_buf.parseObject(jsonArray);
    if (!root.success())
    {
      Serial.println("parseObject() failed");
    }

    String location = root["city"]["name"];
    String temperature = root["list"]["main"]["temp"];
    String weather = root["list"]["weather"]["main"];
    String description = root["list"]["weather"]["description"];
    String idString = root["list"]["weather"]["id"];
    String timeS = root["list"]["dt_txt"];

    timeS = convertGMTTimeToLocal(timeS);

    int length = temperature.length();
    if(length==5)
    {
      temperature.remove(length-1);
    }

    Serial.println(location);
    Serial.println(weather);
    Serial.println(temperature);
    Serial.println(description);
    Serial.println(temperature);
    Serial.println(timeS);

    clearScreen();

    int weatherID = idString.toInt();
    printData(timeS,temperature, timeS, weatherID);
} 


/********************************************************************//**
 * @brief     display the weather forecast on left display side 
 * @param[in] timeString  // displayed time  
 *            temperature // displayed outside temperature
 *            weatherID   // id for future weather condition
 * @return    None
 *********************************************************************/
void printData(String timeString, String temperature, String time, int weatherID)
{
  tft.setCursor(35,60);
  tft.setTextColor(WHITE,BLACK);
  tft.setTextSize(2);
  tft.print(timeString);

  printWeatherIcon(weatherID);

  tft.setCursor(27,172);
  tft.setTextColor(WHITE,BLACK);
  tft.setTextSize(2);
  tft.print(temperature);

  tft.setCursor(83,170);
  tft.setTextColor(WHITE,BLACK);
  tft.setTextSize(1);
  tft.print("o");
  tft.setCursor(93,172);
  tft.setTextColor(WHITE,BLACK);
  tft.setTextSize(2);
  tft.print("C");
}

/********************************************************************//**
 * @brief     creates the small weather picture on the left side 
 * @param[in] id // id for future weather condition
 * @return    None
 *********************************************************************/
void printWeatherIcon(int id)
{
 switch(id)
 {
  case 800: drawClearWeather(); break;
  case 801: drawFewClouds(); break;
  case 802: drawFewClouds(); break;
  case 803: drawCloud(); break;
  case 804: drawCloud(); break;
  
  case 200: drawThunderstorm(); break;
  case 201: drawThunderstorm(); break;
  case 202: drawThunderstorm(); break;
  case 210: drawThunderstorm(); break;
  case 211: drawThunderstorm(); break;
  case 212: drawThunderstorm(); break;
  case 221: drawThunderstorm(); break;
  case 230: drawThunderstorm(); break;
  case 231: drawThunderstorm(); break;
  case 232: drawThunderstorm(); break;

  case 300: drawLightRain(); break;
  case 301: drawLightRain(); break;
  case 302: drawLightRain(); break;
  case 310: drawLightRain(); break;
  case 311: drawLightRain(); break;
  case 312: drawLightRain(); break;
  case 313: drawLightRain(); break;
  case 314: drawLightRain(); break;
  case 321: drawLightRain(); break;

  case 500: drawLightRainWithSunOrMoon(); break;
  case 501: drawLightRainWithSunOrMoon(); break;
  case 502: drawLightRainWithSunOrMoon(); break;
  case 503: drawLightRainWithSunOrMoon(); break;
  case 504: drawLightRainWithSunOrMoon(); break;
  case 511: drawLightRain(); break;
  case 520: drawModerateRain(); break;
  case 521: drawModerateRain(); break;
  case 522: drawHeavyRain(); break;
  case 531: drawHeavyRain(); break;

  case 600: drawLightSnowfall(); break;
  case 601: drawModerateSnowfall(); break;
  case 602: drawHeavySnowfall(); break;
  case 611: drawLightSnowfall(); break;
  case 612: drawLightSnowfall(); break;
  case 615: drawLightSnowfall(); break;
  case 616: drawLightSnowfall(); break;
  case 620: drawLightSnowfall(); break;
  case 621: drawModerateSnowfall(); break;
  case 622: drawHeavySnowfall(); break;

  case 701: drawFog(); break;
  case 711: drawFog(); break;
  case 721: drawFog(); break;
  case 731: drawFog(); break;
  case 741: drawFog(); break;
  case 751: drawFog(); break;
  case 761: drawFog(); break;
  case 762: drawFog(); break;
  case 771: drawFog(); break;
  case 781: drawFog(); break;

  default:break; 
 }
}

/********************************************************************//**
 * @brief     convert the GMT time to local time 
 * @param[in] timeS // tiem string to convert
 * @return    converted time string
 *********************************************************************/
String convertGMTTimeToLocal(String timeS)
{
 int length = timeS.length();
 timeS = timeS.substring(length-8,length-6);
 int time = timeS.toInt();
 time = time+TimeZone;

 if(time > 21 ||  time<7)
 {
  night=true;
 }else
 {
  night = false;
 }
 timeS = String(time)+":00";
 return timeS;
}


/********************************************************************//**
 * @brief     clears the screen  
 * @param[in] None
 * @return    None
 *********************************************************************/
void clearScreen()
{
    tft.fillScreen(BLACK);
}


/********************************************************************//**
 * @brief     draw picture for weather without rain, snow and clouds 
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawClearWeather()
{
  if(night)
  {
    drawTheMoon();
  }else
  {
    drawTheSun();
  }
}

/********************************************************************//**
 * @brief     draw cloud picture with sun or moon behind 
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawFewClouds()
{
  if(night)
  {
    drawCloudAndTheMoon();
  }else
  {
    drawCloudWithSun();
  }
}

/********************************************************************//**
 * @brief     draw the sun without any clouds 
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawTheSun()
{
    tft.fillCircle(64,120,26,YELLOW);
}

/********************************************************************//**
 * @brief     draw the moon without any clouds 
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawTheFullMoon()
{
    tft.fillCircle(64,120,26,GREY);
}


/********************************************************************//**
 * @brief     draw the moon  
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawTheMoon()
{
    tft.fillCircle(64,120,26,GREY);
    tft.fillCircle(75,113,26,BLACK);
}


/********************************************************************//**
 * @brief     draw a single cloud  
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawCloud()
{
     tft.drawBitmap(0,75,cloud,128,90,GREY);
}


/********************************************************************//**
 * @brief     draw a cloud with sun behind  
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawCloudWithSun()
{
     tft.fillCircle(73,110,20,YELLOW);
     tft.drawBitmap(0,76,cloud,128,90,BLACK);
     tft.drawBitmap(0,80,cloud,128,90,GREY);
}

/********************************************************************//**
 * @brief     draw rain with sun or moon behind  
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawLightRainWithSunOrMoon()
{
  if(night)
  {
    drawCloudTheMoonAndRain();
  }else
  {
    drawCloudSunAndRain();
  }
}


/********************************************************************//**
 * @brief     draw light rain (under the cloud)  
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawLightRain()
{
     tft.drawBitmap(0,75,cloud,128,90,GREY);
     tft.fillRoundRect(50, 145, 3, 13, 1, BLUE);
     tft.fillRoundRect(65, 145, 3, 13, 1, BLUE);
     tft.fillRoundRect(80, 145, 3, 13, 1, BLUE);
}


/********************************************************************//**
 * @brief     draw moderate rain (under the cloud)  
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawModerateRain()
{
     tft.drawBitmap(0,75,cloud,128,90,GREY);
     tft.fillRoundRect(50, 145, 3, 15, 1, BLUE);
     tft.fillRoundRect(57, 142, 3, 15, 1, BLUE);
     tft.fillRoundRect(65, 145, 3, 15, 1, BLUE);
     tft.fillRoundRect(72, 142, 3, 15, 1, BLUE);
     tft.fillRoundRect(80, 145, 3, 15, 1, BLUE);
}


/********************************************************************//**
 * @brief     draw heavy rain (under the cloud)  
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawHeavyRain()
{
     tft.drawBitmap(0,75,cloud,128,90,GREY);
     tft.fillRoundRect(43, 142, 3, 15, 1, BLUE);
     tft.fillRoundRect(50, 145, 3, 15, 1, BLUE);
     tft.fillRoundRect(57, 142, 3, 15, 1, BLUE);
     tft.fillRoundRect(65, 145, 3, 15, 1, BLUE);
     tft.fillRoundRect(72, 142, 3, 15, 1, BLUE);
     tft.fillRoundRect(80, 145, 3, 15, 1, BLUE);
     tft.fillRoundRect(87, 142, 3, 15, 1, BLUE);
}

/********************************************************************//**
 * @brief     draw thunderstrom with rain 
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawThunderstorm()
{
     tft.drawBitmap(0,80,thunder,128,90,YELLOW);
     tft.drawBitmap(0,75,cloud,128,90,GREY);
     tft.fillRoundRect(48, 142, 3, 15, 1, BLUE);
     tft.fillRoundRect(55, 142, 3, 15, 1, BLUE);
     tft.fillRoundRect(74, 142, 3, 15, 1, BLUE);
     tft.fillRoundRect(82, 142, 3, 15, 1, BLUE);
}


/********************************************************************//**
 * @brief     draw light snow fall (under the cloud)  
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawLightSnowfall()
{
     tft.drawBitmap(0,70,cloud,128,90,GREY);
     tft.fillCircle(50, 140, 3, GREY);
     tft.fillCircle(65, 143, 3, GREY);
     tft.fillCircle(82, 140, 3, GREY);
}


/********************************************************************//**
 * @brief     draw moderate snow fall (under the cloud)  
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawModerateSnowfall()
{
     tft.drawBitmap(0,75,cloud,128,90,GREY);
     tft.fillCircle(50, 145, 3, GREY);
     tft.fillCircle(50, 155, 3, GREY);
     tft.fillCircle(65, 148, 3, GREY);
     tft.fillCircle(65, 158, 3, GREY);
     tft.fillCircle(82, 145, 3, GREY);
     tft.fillCircle(82, 155, 3, GREY);
}


/********************************************************************//**
 * @brief     draw heavy snow fall (under the cloud)   
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawHeavySnowfall()
{
     tft.drawBitmap(0,75,cloud,128,90,GREY);
     tft.fillCircle(40, 145, 3, GREY);
     tft.fillCircle(52, 145, 3, GREY);
     tft.fillCircle(52, 155, 3, GREY);
     tft.fillCircle(65, 148, 3, GREY);
     tft.fillCircle(65, 158, 3, GREY);
     tft.fillCircle(80, 145, 3, GREY);
     tft.fillCircle(80, 155, 3, GREY);
     tft.fillCircle(92, 145, 3, GREY);     
}

/********************************************************************//**
 * @brief     draw cloud with sun and rain   
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawCloudSunAndRain()
{
     tft.fillCircle(73,110,20,YELLOW);
     tft.drawBitmap(0,72,cloud,128,90,BLACK);
     tft.drawBitmap(0,75,cloud,128,90,GREY);
     tft.fillRoundRect(50, 145, 3, 13, 1, BLUE);
     tft.fillRoundRect(65, 145, 3, 13, 1, BLUE);
     tft.fillRoundRect(80, 145, 3, 13, 1, BLUE);
}


/********************************************************************//**
 * @brief     draw cloud and moon   
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawCloudAndTheMoon()
{
     tft.fillCircle(94,100,18,GREY);
     tft.fillCircle(105,93,18,BLACK);
     tft.drawBitmap(0,72,cloud,128,90,BLACK);
     tft.drawBitmap(0,75,cloud,128,90,GREY);
}


/********************************************************************//**
 * @brief     draw cloud and moon and rain  
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawCloudTheMoonAndRain()
{
     tft.fillCircle(94,100,18,GREY);
     tft.fillCircle(105,93,18,BLACK);
     tft.drawBitmap(0,72,cloud,128,90,BLACK);
     tft.drawBitmap(0,75,cloud,128,90,GREY);
     tft.fillRoundRect(50, 145, 3, 11, 1, BLUE);
     tft.fillRoundRect(65, 145, 3, 11, 1, BLUE);
     tft.fillRoundRect(80, 145, 3, 11, 1, BLUE);
}

/********************************************************************//**
 * @brief     draw wind
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawWind()
{  
     tft.drawBitmap(0,75,wind,128,90,GREY);   
}

/********************************************************************//**
 * @brief     draw fog symbol   
 * @param[in] None
 * @return    None
 *********************************************************************/
void drawFog()
{
  tft.fillRoundRect(45, 100, 40, 4, 1, GREY);
  tft.fillRoundRect(40, 110, 50, 4, 1, GREY);
  tft.fillRoundRect(35, 120, 60, 4, 1, GREY);
  tft.fillRoundRect(40, 130, 50, 4, 1, GREY);
  tft.fillRoundRect(45, 140, 40, 4, 1, GREY);
}

/********************************************************************//**
 * @brief     clear the of the weather picture only   
 * @param[in] None
 * @return    None
 *********************************************************************/
void clearIcon()
{
     tft.fillRect(0,80,128,100,BLACK);
}
