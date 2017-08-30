#include <Adafruit_BMP085.h>
#include <AssetTracker.h>
#include <HttpClient.h>
#include <ParticleSoftSerial.h>

#if !defined(PARTICLE)
 #include <Wire.h>
#endif

#if (SYSTEM_VERSION >= 0x00060000)
  SerialLogHandler logHandler;
#endif

#define SENDER   Serial1
#define RECEIVER SoftSer
#define PROTOCOL SERIAL_8N1
#define EMR_PIN D7
#define BUT_PIN D5
#define FSR_PIN A0
#define GAS_PIN A1
#define SPE_PIN D4
#define PSS_RX D2 // RX must be interrupt enabled (on Photon/Electron D0/A5 are not)
#define PSS_TX D3

ParticleSoftSerial SoftSer(PSS_RX, PSS_TX);
Adafruit_BMP085 bmp;
// Connect SCL to i2c clock - on '168/'328 Arduino Uno/Duemilanove/etc thats Analog 5, Particle on D1
// Connect SDA to i2c data - on '168/'328 Arduino Uno/Duemilanove/etc thats Analog 4, Particle on D0

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// GPS
int transmittingData = 1;   // Used to keep track of the last time we published data
long lastPublish = 0;       // How many minutes between publishes? 10+ recommended for long-time continuous publishing!
int delayMinutes = 10;      // Creating an AssetTracker named 't' for us to reference
AssetTracker t = AssetTracker();
// A FuelGauge named 'fuel' for checking on the battery state
FuelGauge fuel;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// GPS

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// http
unsigned int nextTime = 0;    // Next time to contact the server
HttpClient http;
// Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {
    { "Content-Type", "application/json" },
    { "Accept" , "application/json" },
    //{ "Accept" , "*/*"},
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};
http_request_t request;
http_response_t response;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// http

int BassTab[] = {1911, 1702, 1516, 1431, 1275, 1136, 1012}; //bass 1~7
int fsrReading;     // the analog reading from the FSR resistor divider
int gas_value;
int receivedAl;
int receivedA2;
int receivedA3;
int receivedA4;
int pressure_alram;
int pulse_alram;
int fsr_alram;
int emr_alram;
int gas_alram;
int altitude_Helmet;   //helemet's Altitude Value
int altitude_Ring;

int send;

char szTX[64];
char szRX[64];
char string[300];
char string1[15];
char string_la[15];
char string_lo[15];
char SUPADUPA[25];
char DUPADUPA[25];


void setup()
{
    send = 1;
    pressure_alram = 0;
    pulse_alram = 0;
    strcpy(DUPADUPA, "49004c000351353337353037");
    strcpy(SUPADUPA, "4a0028000d51353432393339");

    //https://requestb.in/14snbel1
    request.hostname = "www.safetybob.n-e.kr";
    request.port = 8088;
    request.path = "/electron/gpsTest.do";

//   request.hostname = "requestb.in";
//   request.port = 80;
//   request.path = "/14snbel1";

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////GPS
  // Sets up all the necessary AssetTracker bits
    t.begin();
    // Enable the GPS module. Defaults to off to save power.
    // Takes 1.5s or so because of delays.
    t.gpsOn();

    // These three functions are useful for remote diagnostics. Read more below.
    Particle.function("tmode", transmitMode);
    Particle.function("batt", batteryStatus);
    Particle.function("gps", gpsPublish);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////GPS
  
    Serial.begin(9600);
    pinInit();

    if (!bmp.begin()) {
        Serial.println("Could not find a valid BMP085 sensor, check wiring!");
        while (!bmp.begin()) {}
    }
    RECEIVER.begin(9600);
}

void loop()
{
    string[0] = '\0';
    gas_value = analogRead(GAS_PIN);
    fsrReading = analogRead(FSR_PIN);
  Serial.println(gas_value);
  Serial.println("test11111111111111111111111111111");
 Serial.println(fsrReading);

    if(SoftSer.available()) {
        altitude_Helmet = bmp.readAltitude();
        receivedAl = SoftSer.read();
        if(receivedAl == 255) {
            receivedA2 = SoftSer.read();
            receivedA3 = SoftSer.read();
            receivedA4 = SoftSer.read();
            if(receivedA4 == 1) {
                Serial.println(receivedA2);
                Serial.print(receivedA3);
                Serial.print("---");
                Serial.println(altitude_Helmet);
                altitude_Ring = receivedA3;
                if(receivedA2 == 30) {
                    pulse_alram = 1;
                    strcpy(string1, "pulse");
                 }
            } else {
                Serial.println("fail to receive");  
                receivedAl = 0;
                receivedA2 = 0;
                receivedA3 = 0;
                receivedA4 = 0;
                altitude_Ring = altitude_Helmet;
            }
        }
    }

    if((altitude_Ring - altitude_Helmet) > 3) {
        Serial.println("Warning !! Keep safety rings nearby");
        pressure_alram = 1;
        strcpy(string1, "Ring");
    } else {
        pressure_alram = 0;
    }

    if(fsrReading >= 3000) {
        fsr_alram = 1;
        strcpy(string1, "Fsr");
    }
    if(gas_value >= 1200) {
        gas_alram = 1;
        strcpy(string1, "Gas");
    }
    
    if(digitalRead(EMR_PIN) == LOW) {
        emr_alram = 1;
        strcpy(string1, "Emergency");
    }
    

    if(digitalRead(BUT_PIN) == LOW) {
      fsr_alram = 0;
      gas_alram = 0;
      pulse_alram = 0;
      emr_alram = 0;
      send = 1;
      //pressure_alram =0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// GPS

    t.updateGPS();
//    Serial.println(t.readLatLon());
            
    if((fsr_alram == 1) || (gas_alram == 1) || (pulse_alram == 1) || (pressure_alram == 1) || (emr_alram == 1)) {
        sound(6);
    } else {
        strcpy(string1, "Location");
    }

    strcat(string, "{\"deviceId\":");
    strcat(string, DUPADUPA);
    strcat(string, ",\"kind\":\"");
    strcat(string, string1);
    strcat(string, "\",\"latitude\":\"");
    strcat(string, string_la);
    strcat(string, "\",\"longitude\":\"");
    strcat(string, string_lo);
    strcat(string, "\"}");
//    Serial.println(string);
        
    // if the current time - the last time we published is greater than your set delay...
    if (millis()-lastPublish > delayMinutes*60*1000/30) {
      // Remember when we published
        lastPublish = millis();
        //Particle.publish("A", pubAccel, 60, PRIVATE);
        // Dumps the full NMEA sentence to serial in case you're curious
        //Serial.println(t.preNMEA());
        
        String pubAccel1 = String::format("%f", t.readLatDeg());
        strcpy(string_la, pubAccel1);
        String pubAccel2 = String::format("%f", t.readLonDeg());
        strcpy(string_lo, pubAccel2);
        Serial.println("GPS update");
        Serial.println("-------------------------------------------");        
        // GPS requires a "fix" on the satellites to give good data,
        // so we should only publish data if there's a fix
        if (t.gpsFix()) {
            // Only publish if we're in transmittingData mode 1;
            if (transmittingData) {
                // Short publish names save data!
                Particle.publish("G", t.readLatLon(), 60, PRIVATE);
            }
            
        }
    } 
    //Serial.println(string);
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////GPS

    if ((send == 1) && ((fsr_alram == 1) || (gas_alram == 1) || (pulse_alram == 1) || (pressure_alram == 1) || (emr_alram == 1))) {
        Serial.println("**************  Warning !! ****************");
        request.body = string;    
//        http.get(request, response, headers);
        Serial.println(string);
        Serial.println("-------------------------------------------");
        send = 0;
    }

  ////////////////////////////////////// http
    //if (nextTime > millis()) return; millis()-lastPublish > delayMinutes*60*1000/20
   if ((60*1000*3) < (millis() - nextTime)) {
       
        nextTime = millis();
        Serial.println();
        Serial.println("Sending status messaes every two minutes");

    // The library also supports sending a body with your request:
        request.body = string;    
        http.get(request, response, headers);
        Serial.print("Application>\tResponse status: ");
        Serial.println(response.status);
        Serial.print("Application>\tHTTP Response Body: ");
        Serial.println(response.body);
        Serial.println(string);
        Serial.println("-------------------------------------------");
    ////////////////////////////////////////// http
    }
  delay(100);
}





void pinInit()
{
  pinMode(SPE_PIN, OUTPUT);
  digitalWrite(SPE_PIN, LOW);
  pinMode(BUT_PIN, INPUT);
  pinMode(EMR_PIN, INPUT);
}
void sound(uint8_t note_index)
{
  for (int i = 0; i < 100; i++)
  {
    digitalWrite(SPE_PIN, HIGH);
    delayMicroseconds(BassTab[note_index]);
    digitalWrite(SPE_PIN, LOW);
    delayMicroseconds(BassTab[note_index]);
  }
}

////////////////////////////////////////////////////////// GPS

// Allows you to remotely change whether a device is publishing to the cloud
// or is only reporting data over Serial. Saves data when using only Serial!
// Change the default at the top of the code.
int transmitMode(String command) {
    transmittingData = atoi(command);
    return 1;
}

// Actively ask for a GPS reading if you're impatient. Only publishes if there's
// a GPS fix, otherwise returns '0'
int gpsPublish(String command) {
    if (t.gpsFix()) {
        Particle.publish("G", t.readLatLon(), 60, PRIVATE);
        // uncomment next line if you want a manual publish to reset delay counter
        // lastPublish = millis();
        return 1;
    } else {
        return 0;
    }
}

// Lets you remotely check the battery status by calling the function "batt"
// Triggers a publish with the info (so subscribe or watch the dashboard)
// and also returns a '1' if there's >10% battery left and a '0' if below
int batteryStatus(String command){
    // Publish the battery voltage and percentage of battery remaining
    // if you want to be really efficient, just report one of these
    // the String::format("%f.2") part gives us a string to publish,
    // but with only 2 decimal points to save space
    Particle.publish("B",
          "v:" + String::format("%.2f",fuel.getVCell()) +
          ",c:" + String::format("%.2f",fuel.getSoC()),
          60, PRIVATE
    );
    // if there's more than 10% of the battery left, then return 1
    if (fuel.getSoC()>10){ return 1;}
    // if you're running out of battery, return 0
    else { return 0;}
}
//////////////////////////////////////////////////////////////GPS