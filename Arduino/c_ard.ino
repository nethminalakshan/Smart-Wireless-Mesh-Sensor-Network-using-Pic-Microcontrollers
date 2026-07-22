#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -------- OBJECTS --------
RF24 radio(9, 10);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------- ADDRESS --------
const byte addressC[6] = "NODEC";

// -------- STRUCT (MUST MATCH NODE B) --------
struct SensorData {
  byte nodeID;
  float temp;
  float hum;
  int gas;
  int vibration;
  int flame;
};

SensorData data;

// -------- LED --------
#define LED 6

// -------- LCD PAGE CONTROL --------
int page = 0;
unsigned long lastChange = 0;

// -------- SETUP --------
void setup() {
  Serial.begin(9600);

  pinMode(LED, OUTPUT);

  lcd.init();
  lcd.backlight();

  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.setPALevel(RF24_PA_LOW);

  radio.openReadingPipe(0, addressC);
  radio.startListening();

  Serial.println("Node C Ready (Receiving from B)");
}

// -------- LCD DISPLAY FUNCTION --------
void displayLCD() {

  // Switch page every 2 seconds
  if (millis() - lastChange > 2000) {
    page++;
    if (page > 1) page = 0;
    lastChange = millis();
    lcd.clear();
  }

  if (page == 0) {
    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.print(data.temp,1);

    lcd.setCursor(9,0);
    lcd.print("H:");
    lcd.print(data.hum,0);

    lcd.setCursor(0,1);
    lcd.print("G:");
    lcd.print(data.gas);
  }

  else {
    lcd.setCursor(0,0);
    lcd.print("V:");
    lcd.print(data.vibration);

    lcd.setCursor(0,1);
    lcd.print("F:");
    lcd.print(data.flame);
  }
}

// -------- LOOP --------
void loop() {

  if (radio.available()) {

    radio.read(&data, sizeof(data));

    digitalWrite(LED, HIGH);

    // -------- SERIAL OUTPUT --------
    Serial.println("========= FROM NODE B =========");

    Serial.print("NODE: ");
    Serial.println(data.nodeID);

    Serial.print("TEMP: ");
    Serial.println(data.temp);

    Serial.print("HUM : ");
    Serial.println(data.hum);

    Serial.print("GAS : ");
    Serial.println(data.gas);

    Serial.print("VIB : ");
    Serial.println(data.vibration);

    Serial.print("FLAME: ");
    Serial.println(data.flame);

    Serial.println("===============================\n");
  }

  // -------- LCD UPDATE --------
  displayLCD();

  delay(50);
  digitalWrite(LED, LOW);
}