#include <Bonezegei_SX1278.h>

#define CS_PIN    10
#define RST_PIN   9
#define DIO0_PIN  2

#define NODE_ID   0x02 // Node Address

Bonezegei_SX1278 lora(CS_PIN, RST_PIN, DIO0_PIN);

// Callback hook handling incoming network frames
void handleNetworkPacket(uint8_t *payload, uint8_t length, int16_t rssi, int8_t snr) {
  if (length < sizeof(Bonezegei_FrameHeader)) return;

  Bonezegei_FrameHeader header;
  memcpy(&header, payload, sizeof(Bonezegei_FrameHeader));

  uint8_t *data = payload + sizeof(Bonezegei_FrameHeader);
  uint8_t dataLen = length - sizeof(Bonezegei_FrameHeader);

  // Check target destination or broadcast address
  if (header.destAddr == NODE_ID || header.destAddr == 0xFF) {
    Serial.print("[MESH RECV] From Node 0x");
    Serial.print(header.srcAddr, HEX);
    Serial.print(" | RSSI: ");
    Serial.print(rssi);
    Serial.print(" | Data: ");
    for (uint8_t i = 0; i < dataLen; i++) Serial.print((char)data[i]);
    Serial.println();
  } else {
    // Forward / Relay frame in Mesh topology
    Serial.print("[MESH RELAY] Forwarding packet from 0x");
    Serial.print(header.srcAddr, HEX);
    Serial.print(" to 0x");
    Serial.println(header.destAddr, HEX);
    
    header.hopCount++;
    lora.sendFrame(header, data, dataLen);
    lora.receive();
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!lora.begin(433E6)) {
    Serial.println("SX1278 Init Failed!");
    while (1);
  }

  lora.onReceive(handleNetworkPacket);
  lora.receive();
  Serial.println("Mesh Protocol Receiver Ready!");
}

void loop() {
  // Process non-blocking packet interrupts/polling
  lora.handleInterrupt();
  delay(10);
}