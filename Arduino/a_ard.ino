#include <SPI.h>
#include <RF24.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11

#define GAS A0
#define VIB 3
#define FLAME 4
#define LED 5

RF24 radio(9, 10);
DHT dht(DHTPIN, DHTTYPE);

const byte addressB[6] = "NODEB";

struct SensorData {
  byte nodeID;
  float temp;
  int gas;
  int vibration;
  int flame;
};

SensorData data;

void setup() {
  Serial.begin(9600);

  pinMode(VIB, INPUT);
  pinMode(FLAME, INPUT);
  pinMode(LED, OUTPUT);

  dht.begin();

  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.setPALevel(RF24_PA_LOW);

  radio.openWritingPipe(addressB);
  radio.stopListening();
}

void loop() {
  data.nodeID = 1;
  data.temp = dht.readTemperature();
  data.gas = analogRead(GAS);
  data.vibration = digitalRead(VIB);
  data.flame = digitalRead(FLAME);

  digitalWrite(LED, HIGH);
  radio.write(&data, sizeof(data));
  delay(100);
  digitalWrite(LED, LOW);

  delay(1000);
}