#include <Bonezegei_SX1278.h>

#define CS_PIN    10
#define RST_PIN   9
#define DIO0_PIN  2

Bonezegei_SX1278 lora(CS_PIN, RST_PIN, DIO0_PIN);
uint32_t counter = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!lora.begin(433E6)) {
    Serial.println("SX1278 initialization failed!");
    while (1);
  }

  Serial.println("SX1278 Transmitter Ready!");
}

void loop() {
  String message = "Hello LoRa #" + String(counter++);
  Serial.print("Sending: ");
  Serial.println(message);

  if (lora.send(message.c_str())) {
    Serial.println("Packet sent successfully!");
  } else {
    Serial.println("Failed to send packet.");
  }

  delay(3000);
}