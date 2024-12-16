#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>

// Koneksi WiFi
const char* ssid = "ATOMIANS"; 
const char* password = "farchws5";

// Koneksi MQTT
#define mqttServer "broker.hivemq.com"
#define mqttPort 1883

WiFiClient espClient;
PubSubClient client(espClient);

// Pin pada iTCLab Shield
const int pinT1   = 34; // Sensor Suhu T1
const int pinT2   = 35; // Sensor Suhu T2
const int pinLED  = 26; // LED

// Properti LED PWM
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

// Batas suhu kebakaran
const float fire_temperature_limit = 25;

// Variabel suhu
float cel1, cel2;

void setup() {
  // Inisialisasi Serial
  Serial.begin(115200);
  while (!Serial) {
    ; // Tunggu koneksi serial
  }

  // Konfigurasi PWM untuk LED
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(pinLED, ledChannel);
  ledcWrite(ledChannel, 0); // Matikan LED di awal

  // Koneksi WiFi
  Serial.print("Menghubungkan ke ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi terhubung.");

  // Koneksi ke server MQTT
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    Serial.println("Menghubungkan ke server MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Terhubung ke MQTT!");
    } else {
      Serial.print("Gagal, status: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void sendLogMessage(const char* logMessage) {
  client.publish("Log", logMessage);
}

void blinkLED() {
  for (int i = 0; i < 5; i++) { // LED berkedip 5 kali
    ledcWrite(ledChannel, 255); // Nyalakan LED
    delay(500);
    ledcWrite(ledChannel, 0);   // Matikan LED
    delay(500);
  }
}

void cektemp() {
  // Membaca suhu dari sensor
  cel1 = analogRead(pinT1) * 0.322265625 / 10; // Konversi ke Celsius
  cel2 = analogRead(pinT2) * 0.322265625 / 10;

  // Tampilkan suhu
  Serial.print("Suhu T1: ");
  Serial.print(cel1);
  Serial.println(" °C");

  Serial.print("Suhu T2: ");
  Serial.print(cel2);
  Serial.println(" °C");

  // Kirim suhu ke MQTT
  char suhu1[8], suhu2[8];
  dtostrf(cel1, 1, 2, suhu1); // Konversi ke string
  dtostrf(cel2, 1, 2, suhu2);
  client.publish("Suhu1", suhu1);
  client.publish("Suhu2", suhu2);

  Serial.println("Suhu dikirim ke broker MQTT.");

  // Deteksi kebakaran
  if (cel1 > fire_temperature_limit || cel2 > fire_temperature_limit) {
    Serial.println("PERINGATAN: Kebakaran terdeteksi!");
    blinkLED();
  }
}

void loop() {
  if (!client.connected()) {
    // Reconnect ke MQTT jika terputus
    while (!client.connected()) {
      Serial.println("Reconnect ke server MQTT...");
      if (client.connect("ESP32Client")) {
        Serial.println("Terhubung kembali ke MQTT!");
      } else {
        Serial.print("Gagal, status: ");
        Serial.println(client.state());
        delay(2000);
      }
    }
  }
  client.loop(); // Proses loop MQTT

  cektemp();
  delay(1000); // Delay 1 detik


 // Membaca log dari Serial Monitor
  if (Serial.available()) {
    String logMessage = Serial.readStringUntil('\n'); // Membaca hingga baris baru
    logMessage.trim(); // Hapus spasi kosong di awal/akhir
    if (logMessage.length() > 0) {
      Serial.println("Mengirim log ke MQTT: " + logMessage);
      sendLogMessage(logMessage.c_str()); // Kirim ke MQTT
    }
  }
}
