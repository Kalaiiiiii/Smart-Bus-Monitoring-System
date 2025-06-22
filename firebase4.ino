#include <FB_Const.h>
#include <FB_Error.h>
#include <FB_Network.h>
#include <FB_Utils.h>
#include <Firebase.h>
#include <FirebaseESP8266.h>
#include <FirebaseFS.h>
#include <MB_File.h>
#include <LiquidCrystal.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <TinyGPSPlus.h>
#define MQ3_ANALOG_PIN A0        // Analog pin on ESP8266
#define BUZZER_PIN     4         // D2 on NodeMCU = GPIO4

const int THRESHOLD = 400;       // Analog value above which buzzer will sound
// GPS Pins
#define RXPin D7
#define TXPin D8
#define GPSBaud 9600

// IR sensors
int sensor1 = D5;  // Sensor 1
int sensor2 = D6;  // Sensor 2

// WiFi Credentials
#define WIFI_SSID "your_wifi_name"
#define WIFI_PASSWORD "your_wifi_password"

// Firebase Credentials
#define FIREBASE_HOST "your_credential_1"
#define FIREBASE_API_KEY "your_credential_2"
#define FIREBASE_AUTH "your_credential_3"

// Initialize components
SoftwareSerial gpsSerial(RXPin, TXPin);
TinyGPSPlus gps;

// Firebase Objects
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// People counter
int count = 0;

void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // Make sure buzzer is off initially
  gpsSerial.begin(GPSBaud);// GPS module baud rate
  Serial.println("Waiting for GPS signal...");
  
  // Set sensor pins as INPUT
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  Serial.println("GPS & Counter Initialized...");
    
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to WiFi!");

  // Firebase Configuration
  config.host = FIREBASE_HOST;
  config.api_key = FIREBASE_API_KEY;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase Connected!");

}

void loop() {
  Serial.println("Loop started...");
  int analogValue = analogRead(MQ3_ANALOG_PIN);  // Read sensor analog output

  Serial.print("Alcohol Sensor Value: ");
  Serial.println(analogValue);

  if (analogValue > THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH);  // Alcohol detected → buzzer ON
  } else {
    digitalWrite(BUZZER_PIN, LOW);   // No alcohol → buzzer OFF
  }

  // Read IR sensor states
  int sensor1State = digitalRead(sensor1);
  int sensor2State = digitalRead(sensor2);

  // Entry Condition (Sensor 1 triggered first)
  if (sensor1State == LOW && sensor2State == HIGH) {
    count++;
    Serial.println("Passenger Entered");
    delay(600);
    while (digitalRead(sensor2) == LOW);
    delay(600);
  }

  // Exit Condition (Sensor 2 triggered first)
  if (sensor2State == LOW && sensor1State == HIGH) {
    count--;
    if (count < 0) count = 0;
    Serial.println("Passenger Exited");
    delay(600);
    while (digitalRead(sensor1) == LOW);
    delay(600);
  }

  Serial.print("Current Count: ");
  Serial.println(count);

  //Update passenger count to Firebase
  if (Firebase.ready()) {
    Serial.println("Updating Passenger Count...");
    if (Firebase.setInt(firebaseData, "/buses/bus101/passengers", count)) {
      Serial.println("Passenger count updated!");
    } else {
      Serial.println("Firebase Error (Passenger): " + firebaseData.errorReason());
    }
  } else {
    Serial.println("Firebase not ready for passengers update!");
  }

  //Check if GPS data is available before sending it
  bool gpsUpdated = false;
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    Serial.write(c);  // Print raw GPS data for debugging
    gps.encode(c);
    gpsUpdated = true;
  }

  if (gpsUpdated && gps.location.isValid()) {
      float latitude = gps.location.lat();
      float longitude = gps.location.lng();
      Serial.print("Latitude: ");
      Serial.print(latitude, 6);
      Serial.print(" | Longitude: ");
      Serial.println(longitude, 6);

      //Update GPS coordinates to Firebase separately
      if (Firebase.ready()) {
        Serial.println("Updating GPS location...");
        if (Firebase.setFloat(firebaseData, "/buses/bus101/location/latitude", latitude) &&
            Firebase.setFloat(firebaseData, "/buses/bus101/location/longitude", longitude)) {
              Serial.println("GPS location updated!");
        } else {
          Serial.println("Firebase Error (GPS): " + firebaseData.errorReason());
        }
      } else {
        Serial.println("Firebase not ready for GPS update!");
      }
  } else {
    Serial.println("No valid GPS data available...");
  }
  
  delay(500); // Wait before next update
}

void sendDummyBusData() {
  if (Firebase.ready()) {
    Firebase.setFloat(firebaseData, "/buses/bus102/location/latitude", 10.030657);
    Firebase.setFloat(firebaseData, "/buses/bus102/location/longitude", 76.336030);
    Firebase.setInt(firebaseData, "/buses/bus102/passengers", 44);
    Firebase.setFloat(firebaseData, "/buses/bus201/location/latitude",10.018991);
    Firebase.setFloat(firebaseData, "/buses/bus201/location/longitude", 76.343589);
    Firebase.setInt(firebaseData, "/buses/bus201/passengers", 14);
    
    Serial.println("Dummy data for bus102 and bus210 sent to Firebase.");
  } else {
    Serial.println("Firebase not ready!");
  }
}
