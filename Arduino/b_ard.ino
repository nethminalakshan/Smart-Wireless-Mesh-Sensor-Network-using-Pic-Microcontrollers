#include <SPI.h>
#include <RF24.h>
#include <DHT.h>

// -------- PIN SETUP --------
#define DHTPIN 2
#define DHTTYPE DHT11

#define VIB 3
#define FLAME 4
#define GAS_AO A0
#define LED 6

// -------- OBJECT --------
DHT dht(DHTPIN, DHTTYPE);
RF24 radio(9, 10);

// -------- ADDRESS --------
const byte addressC[6] = "NODEC";

// -------- STRUCT --------
struct SensorData {
  byte nodeID;
  float temp;
  float hum;
  int gas;
  int vibration;
  int flame;
};

SensorData data;

// -------- VARIABLES --------
float lastTemp = 0;
float lastHum = 0;
int gasSmooth = 0;

// -------- SETUP --------
void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(VIB, INPUT);
  pinMode(FLAME, INPUT);
  pinMode(LED, OUTPUT);

  // NRF Setup
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.setPALevel(RF24_PA_LOW);
  radio.setRetries(15, 15);

  radio.openWritingPipe(addressC);
  radio.stopListening();

  Serial.println("=== Sensor System Started (Node B) ===");

  // Gas sensor warm-up
  delay(20000);
}

// -------- LOOP --------
void loop() {

  // -------- READ SENSORS --------
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  int gasValue = analogRead(GAS_AO);
  int vibration = digitalRead(VIB);
  int flame = digitalRead(FLAME);

  Serial.println("------ Sensor Data ------");

  // -------- DHT FIX --------
  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT11 Error!");
  } else {
    lastTemp = temp;
    lastHum = hum;

    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.println(" %");
  }

  // -------- GAS SMOOTH (simple) --------
  gasSmooth = (gasSmooth * 3 + gasValue) / 4;

  Serial.print("Gas Value: ");
  Serial.println(gasSmooth);

  if (gasSmooth > 400) {
    Serial.println("⚠️ Gas Level HIGH!");
  } else {
    Serial.println("Gas Level Normal");
  }

  // -------- FLAME --------
  Serial.print("Flame: ");
  if (flame == LOW) {
    Serial.println("🔥 FIRE DETECTED!");
  } else {
    Serial.println("No Fire");
  }

  // -------- VIBRATION --------
  Serial.print("Vibration: ");
  if (vibration == HIGH) {
    Serial.println("📳 Vibration Detected!");
  } else {
    Serial.println("No Vibration");
  }

  Serial.println("--------------------------\n");

  // -------- PREPARE DATA --------
  data.nodeID = 2;
  data.temp = lastTemp;
  data.hum = lastHum;
  data.gas = gasSmooth;
  data.vibration = vibration;
  data.flame = flame;

  // -------- SEND TO NODE C --------
  digitalWrite(LED, HIGH);
  radio.write(&data, sizeof(data));
  delay(50);
  digitalWrite(LED, LOW);

  delay(1500);
}