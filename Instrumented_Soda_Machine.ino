// Code to connect an Arduino Yun to three flex sensors and measure the volume of soda
// being dispensed (Coca-Cola(R), Diet Coke(R) and Powerade(R)). The measurement will be uploaded
// to your ThingSpeak channel every time one of the three drinks are dispensed.
//
// Copyright (c) 2014 The MathWorks, Inc.

#include <HttpClient.h>
#include <Bridge.h>

// ThingSpeak Settings
String thingSpeakAddress = "http://api.thingspeak.com/update?";
String writeAPIKey = "key=??"; //Enter the write API Key for your ThingSpeak Channel
String tsfield1Name = "&field1=";
String tsfield2Name = "&field2=";
String tsfield3Name = "&field3=";

// Pin selection
const int flex1Pin = A0;
const int flex2Pin = A1;
const int flex3Pin = A2;

// Variables to hold pin readings
float flex1Voltage = 0;
float flex2Voltage = 0;
float flex3Voltage = 0;

// Inital baseline values expected for each flex sensor
float pflex1Threshold = 58;
float pflex2Threshold = 81;
float pflex3Threshold = 65;

// Variable used to calculate when Thingspeak should be updated
long timeLastUpdate;

// Settings to calculate the volume of soda dispensed
float dispenseRate = 0.01; // Assumption is that 10 milli liter of drink is dispensed every 0.1 second
int measurementPeriod = 100; // Check every 0.1 seconds

// Index counter for average calculation
int avgIndex = 1;
float flex1Avg = 0;
float flex2Avg = 0;
float flex3Avg = 0;

// Initial values to keep track of the volume of soda being dispensed
float cokeDisp = 0;
float dietCDisp = 0;
float poweradeDisp = 0;

// Total volume in liters of soda dispensed per measurement period
float totalCokeVol = 0;
float totalDietCokeVol = 0;
float totalPoweradeVol = 0;

// Assume that the serial connection is not available
bool serialEnabled = false;

void setup()
{
  // Needed for internet connection
  Bridge.begin();
  // Initialize serial communication
  Serial.begin(9600);
  // wait up to 5 seconds for the serial interface to start
  long waitStartedAt = millis();
  while (waitStartedAt + 5000 > millis() && !serialEnabled)
  {
    if (!Serial)
    {
      serialEnabled = false;
    }
    else
    {
      serialEnabled = true;
      Serial.println("You are connected to the Console!");
    }
  }

  // Set the pin modes to read the flex sensor values
  pinMode(flex1Pin, INPUT);
  pinMode(flex2Pin, INPUT);
  pinMode(flex3Pin, INPUT);

  // Store the current time. This will be used to calculate when ThingSpeak needs to be updated
  timeLastUpdate = millis();
}

void loop()
{
  HttpClient client;

  // Define how often an attempt will be made to update ThingSpeak
  float updateTime = 60000; //mins*secs*msecs

  // Fetch the flex sensor values
  flex1Voltage = analogRead(flex1Pin);
  flex2Voltage = analogRead(flex2Pin);
  flex3Voltage = analogRead(flex3Pin);

  // Calculate 50 point (5 min window since we are sampling 10 times a second) non-overlapping average of the above readings

  if (avgIndex < 51)
  {
    flex1Avg = flex1Avg + flex1Voltage;
    flex2Avg = flex2Avg + flex2Voltage;
    flex3Avg = flex3Avg + flex3Voltage;
    avgIndex++;
  }
  else
  {
    // Calculate the average
    flex1Avg = flex1Avg / 50;
    flex2Avg = flex2Avg / 50;
    flex3Avg = flex3Avg / 50;

    // Check if the average value has deviated from the previously set baseline value. If deviation is greater
    // than 3 units, then update baseline
    if (abs(flex1Avg - pflex1Threshold) > 3)
    {
      pflex1Threshold = flex1Avg;
    }
    if (abs(flex2Avg - pflex2Threshold) > 3)
    {
      pflex2Threshold = flex2Avg;
    }
    if (abs(flex3Avg - pflex3Threshold) > 3)
    {
      pflex3Threshold = flex3Avg;
    }

    // Reset the average calculation so far
    flex1Avg = 0;
    flex2Avg = 0;
    flex3Avg = 0;
    avgIndex = 1;
  }

  //Check if flex sensor is flexed – indicating a drink dispense
  if ((pflex1Threshold - flex1Voltage) > 5)
  {
    dietCDisp++;
  }

  if ((pflex2Threshold - flex2Voltage) > 5)
  {
    cokeDisp++;
  }

  if ((pflex3Threshold - flex3Voltage) > 5)
  {
    poweradeDisp++;
  }

  // If connected to a computer, send the flex sensor values to the Serial display
  if (serialEnabled)
  {
    Serial.println(flex1Avg);
    Serial.println(flex2Avg);
    Serial.println(flex3Avg);
    Serial.println("\n");
    Serial.println(flex1Voltage);
    Serial.println(flex2Voltage);
    Serial.println(flex3Voltage);
    Serial.println("\n");
    Serial.println(cokeDisp);
    Serial.println(dietCDisp);
    Serial.println(poweradeDisp);

  }

  // If connected to a computer, print time values
  if (serialEnabled)
  {
    Serial.println("Time");
    Serial.println(millis() - timeLastUpdate);
    Serial.println(updateTime);
    Serial.println();
  }

  // If the time since the last update is greater than updateTime, calculate the total dispensed volume of each drink and update your ThingSpeak channel
  if (millis() - timeLastUpdate > updateTime)
  {
    // Total volume is calculated as number of times the flex sensors registered a flex in the entire
    // measurement period times the volume dispensed per reading
    totalCokeVol = cokeDisp * dispenseRate;
    totalDietCokeVol = dietCDisp * dispenseRate ;
    totalPoweradeVol = poweradeDisp * dispenseRate;

    // Update ThingSpeak only if one of the three sodas has been dispensed. Also ensure that
    // only values greater than 0.1 liters are sent to ThingSpeak
    if ((totalCokeVol > 0.1) || (totalDietCokeVol > 0.1) || (totalPoweradeVol > 0.1))
    { // Create string for ThingSpeak post
      String request_string = thingSpeakAddress + writeAPIKey + tsfield1Name + totalCokeVol + tsfield2Name + totalDietCokeVol + tsfield3Name + totalPoweradeVol;

      // Make a HTTP request
      client.get(request_string);
    }

    //Reset the timeLastUpdate variable
    timeLastUpdate = millis();

    // Resetting the counters
    cokeDisp = 0;
    dietCDisp = 0;
    poweradeDisp = 0;
  }
  else
  {
    // If millis() output has rolled over then the difference above will be negative. Resetting the
    // timeLastUpdate value for this case.
    if (millis() < timeLastUpdate)
    {
      timeLastUpdate = millis();
    }
  }

  // Delay till next reading
  delay(measurementPeriod);
}
