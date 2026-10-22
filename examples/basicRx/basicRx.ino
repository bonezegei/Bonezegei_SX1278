#include <Bonezegei_SX1278.h>

#define CS_PIN    10
#define RST_PIN   9
#define DIO0_PIN  2

Bonezegei_SX1278 lora(CS_PIN, RST_PIN, DIO0_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!lora.begin(433E6)) {
    Serial.println("SX1278 initialization failed!");
    while (1);
  }

  lora.receive();
  Serial.println("SX1278 Receiver Ready!");
}

void loop() {
  int packetSize = lora.parsePacket();
  if (packetSize > 0) {
    String message = lora.readString();
    Serial.print("Received: ");
    Serial.println(message);
    Serial.print("RSSI: ");
    Serial.print(lora.getPacketRssi());
    Serial.print(" dBm, SNR: ");
    Serial.print(lora.getPacketSnr());
    Serial.println(" dB");
  }
}