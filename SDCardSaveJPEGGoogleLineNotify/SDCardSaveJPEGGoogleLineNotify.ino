/*  This example uses the camera to capture a JPEG image,
    and saves the image to SD Card.
*/
#include <Arduino.h>
#include "VideoStream.h"
#include "AmebaFatFS.h"
#include "WiFi.h"
#include "Base64.h"

#define CHANNEL 0
#define FILENAME "image.jpg"

// Enter your WiFi ssid and password
char ssid[] = "Network_SSID5";  // your network SSID (name)
char pass[] = "Password";       // your network password
int status = WL_IDLE_STATUS;

uint32_t img_addr = 0;
uint32_t img_len = 0;

String myScript = "/macros/s/AKfycbzlH557KyYnbICeu-jx17WgZREOgB7Z9u0Q6tF-nQVdDNY5UUle3fOn3E-EISu3H5nl/exec";  //Create your Google Apps Script and replace the "myScript" path.
String myLineNotifyToken = "myToken=XVRWxvrFeoZ2oO5s6IYu06eDaGjALNh8ZU0oCy6Zxhf";                             //Line Notify Token. You can set the value of xxxxxxxxxx empty if you don't want to send picture to Linenotify.
String myFoldername = "&myFoldername=AMB82";                                                              // Set the Google Drive folder name to store your file
String myFilename = "&myFilename=image.jpg";                                                                  // Set the Google Drive file name to store your data
String myImage = "&myFile=";

AmebaFatFS fs;
VideoSetting config(VIDEO_FHD, CAM_FPS, VIDEO_JPEG, 1);

void setup() {
  char buf[512];
  char *p;

  Serial.begin(115200);

  // WiFi init
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    // wait 2 seconds for connection:
    delay(2000);
  }
  WiFiCon();
  Serial.println("");
  Serial.println("===================================");

  // camera init
  Camera.configVideoChannel(CHANNEL, config);
  Camera.videoInit();
  Camera.channelBegin(CHANNEL);
  Serial.println("");
  Serial.println("===================================");

  // SD card init
  if (!fs.begin()) {
    Serial.println("Card Mount Failed");
    delay(1000);
  }
  /* list root directory and put results in buf */
  memset(buf, 0, sizeof(buf));
  fs.readDir(fs.getRootPath(), buf, sizeof(buf));
  String filepath = String(fs.getRootPath()) + String(FILENAME);
  File file = fs.open(filepath);
  if (!file) {
    Serial.println("Failed to open file for reading");
    fs.end();
    // error handler, return etc
  }
  delay(1000);

  Serial.println("Files under: " + String(fs.getRootPath()));
  Serial.println("Read from file: " + filepath);
  Serial.println("file size: " + String(file.size()));

  // Taking Photo
  CamFlash();
  Camera.getImage(CHANNEL, &img_addr, &img_len);
  file.write((uint8_t *)img_addr, img_len);
  file.close();
  Serial.println("Photo write finish......");


  /* the filenames are separated with '\0', so we scan one by one */
  p = buf;
  while (strlen(p) > 0) {
    /* list out file name image will be saved as "image.jpg" */
    // printf("%s\r\n", p);
    if (strstr(p, "image.jpg") != NULL) {
      Serial.println("Found 'image.jpg' in the string.");
    } else {
      //Serial.println("Substring 'image.jpg' not found in the string.");
    }
    p += strlen(p) + 1;
  }

  // file processing
  uint8_t *fileinput;
  file = fs.open(filepath);
  unsigned int fileSize = file.size();
  fileinput = (uint8_t *)malloc(fileSize + 1);
  file.read(fileinput, fileSize);
  fileinput[fileSize] = '\0';
  file.close();
  fs.end();
  Serial.println("");
  Serial.println("===================================");

  // trnasfer to Google Drive
  char *input = (char *)fileinput;
  String imageFile = "data:image/jpeg;base32,";
  char output[base64_enc_len(3)];
  for (unsigned int i = 0; i < fileSize; i++) {
    base64_encode(output, (input++), 3);
    if (i % 3 == 0) {
      imageFile += urlencode(String(output));
    }
  }

  String Data = myFoldername + myFilename + myImage;

  const char *myDomain = "script.google.com";
  String getAll = "", getBody = "";

  Serial.println("Connect to " + String(myDomain));
  WiFiSSLClient client_tcp;

  if (client_tcp.connect(myDomain, 443)) {
    Serial.println("Connection successful");

    client_tcp.println("POST " + myScript + " HTTP/1.1");
    client_tcp.println("Host: " + String(myDomain));
    client_tcp.println("Content-Length: " + String(Data.length() + imageFile.length()));
    client_tcp.println("Content-Type: application/x-www-form-urlencoded");
    client_tcp.println("Connection: keep-alive");
    client_tcp.println();

    client_tcp.print(Data);
    for (unsigned int Index = 0; Index < imageFile.length(); Index = Index + 1000) {
      client_tcp.print(imageFile.substring(Index, Index + 1000));
    }

    int waitTime = 10000;  // timeout 10 seconds
    unsigned int startTime = millis();
    boolean state = false;

    while ((startTime + waitTime) > millis()) {
      Serial.print(".");
      delay(100);
      while (client_tcp.available()) {
        char c = client_tcp.read();
        if (state == true) getBody += String(c);
        if (c == '\n') {
          if (getAll.length() == 0) state = true;
          getAll = "";
        } else if (c != '\r')
          getAll += String(c);
        startTime = millis();
      }
      if (getBody.length() > 0) break;
    }
    client_tcp.stop();
    Serial.println(getBody);
  } else {
    getBody = "Connected to " + String(myDomain) + " failed.";
    Serial.println("Connected to " + String(myDomain) + " failed.");
  }
}

void loop() {
  delay(100);
}

//https://github.com/zenmanenergy/ESP8266-Arduino-Examples/
String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  //char code2;
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      //code2='\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      //encodedString+=code2;
    }
    yield();
  }
  return encodedString;
}

void CamFlash() {
  pinMode(LED_G, OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_G, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(300);                 // wait for a second
    digitalWrite(LED_G, LOW);   // turn the LED off by making the voltage LOW
    delay(300);
  }
}

void WiFiCon() {
  pinMode(LED_B, OUTPUT);
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_B, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(300);                 // wait for a second
    digitalWrite(LED_B, LOW);   // turn the LED off by making the voltage LOW
    delay(300);
  }
}