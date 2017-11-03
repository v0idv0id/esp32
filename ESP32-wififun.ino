#include "WiFi.h" 
#include <MQTTClient.h>  // https://github.com/256dpi/arduino-mqtt 



const char* ssid     = "STERNENGAST";
const char* password = "abcdEFGH1234";


#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  300

RTC_DATA_ATTR int bootCount = 0;

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

long long chipid;
String sysid;
int sensorPin = 34;

void wscan() {
  Serial.println("WiFi scanning...");
  int n = WiFi.scanNetworks();
  Serial.println("WiFi scan done!");
  while (n == 0) {
    Serial.print(n);
    Serial.println(" WiFi networks found");
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
}



void wconnect() {
  static bool ledStatus = false;
  long tc=0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {

    if(tc>300) {
      Serial.println("Wifi timeout ... sleep");
      esp_deep_sleep_start();  
    }
    tc++;
    ledStatus = !ledStatus;
    digitalWrite(LED_BUILTIN, ledStatus);
    Serial.print(".");
    delay(100);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}




void sysinfo() {
  chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
//  Serial.printf("ESP32 Chip ID = %04X", (uint16_t)(chipid >> 32)); //print High 2 bytes
//  Serial.printf("%08X\n", (uint32_t)chipid); //print Low 4bytes.
}

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}


void niceCallback() {
  static bool ledStatus = false;
  ledStatus = !ledStatus;

  digitalWrite(LED_BUILTIN, ledStatus);
}



void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  delay(100); 
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  print_wakeup_reason();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);




 
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("Setup done");
  sysinfo();
  sysid = String((long)chipid, HEX);
  //wscan();
  wconnect();


  client.begin("broker.hivemq.com", net);
  Serial.print("\nmqtt connecting...");
  static bool ledStatus = false;

  long tc=0;
  while (!client.connect((const char*)chipid)) {
    if(tc>300) {
      Serial.println("MQTT connect timeout ... sleep");
      esp_deep_sleep_start();  
    }
    tc++;
    Serial.print("*");
    ledStatus = !ledStatus;
    digitalWrite(LED_BUILTIN, ledStatus);
    delay(100);
  }

  Serial.println("\nmqtt connected!");

  String stringOne =  String(String("mjw/") + sysid + String("/boot"));
  client.publish(stringOne, String(bootCount),1,1);
  Serial.println("loop start");

}



void loop()
{
  client.loop();
  
  client.publish(String("mjw/" + sysid + "/temp"), String(temperatureRead()),1,1);
  client.publish(String("mjw/" + sysid + "/voltage"), String(analogRead(sensorPin)),1,1);
  client.disconnect();
  Serial.println("Going to sleep...");

  delay(100);
  esp_deep_sleep_start();



}
