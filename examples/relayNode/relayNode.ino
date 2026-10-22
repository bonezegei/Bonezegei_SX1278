#include <Bonezegei_SX1278.h>

#define CS_PIN    10
#define RST_PIN   9
#define DIO0_PIN  2

#define RELAY_NODE_ID  0x05  // ID of this Relay Node
#define MAX_HOPS       4     // Maximum times a packet can be forwarded

Bonezegei_SX1278 lora(CS_PIN, RST_PIN, DIO0_PIN);

// Ring buffer to keep track of recently seen packets (Deduplication)
struct PacketRecord {
  uint8_t srcAddr;
  uint8_t packetId;
};

#define HISTORY_SIZE 16
PacketRecord seenPackets[HISTORY_SIZE];
uint8_t historyIdx = 0;

// Checks if we have already handled this specific packet
bool isDuplicate(uint8_t srcAddr, uint8_t packetId) {
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (seenPackets[i].srcAddr == srcAddr && seenPackets[i].packetId == packetId) {
      return true;
    }
  }
  return false;
}

// Store packet signature into ring buffer
void rememberPacket(uint8_t srcAddr, uint8_t packetId) {
  seenPackets[historyIdx].srcAddr = srcAddr;
  seenPackets[historyIdx].packetId = packetId;
  historyIdx = (historyIdx + 1) % HISTORY_SIZE;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!lora.begin(433E6)) {
    Serial.println("[ERROR] SX1278 initialization failed!");
    while (1);
  }

  // RF Setup
  lora.setTxPower(17);          // High power output for extended reach
  lora.setSpreadingFactor(7);
  lora.setBandwidth(125E3);
  lora.setSyncWord(0x12);
  lora.enableCrc(true);

  // Switch to listening mode
  lora.receive();

  Serial.println("=========================================");
  Serial.print("  SX1278 Active Relay Node ID: 0x");
  Serial.println(RELAY_NODE_ID, HEX);
  Serial.println("=========================================");
}

void loop() {
  int packetSize = lora.parsePacket();

  if (packetSize > 0) {
    uint8_t buffer[256];
    int len = lora.readBytes(buffer, packetSize);

    // Validate if packet is large enough to contain our frame header
    if (len < (int)sizeof(Bonezegei_FrameHeader)) {
      lora.receive(); // Resume listening
      return;
    }

    // Extract frame header and payload reference
    Bonezegei_FrameHeader header;
    memcpy(&header, buffer, sizeof(Bonezegei_FrameHeader));

    uint8_t *payload = buffer + sizeof(Bonezegei_FrameHeader);
    uint8_t payloadLen = len - sizeof(Bonezegei_FrameHeader);

    int16_t rssi = lora.getPacketRssi();
    int8_t snr = lora.getPacketSnr();

    // 1. Deduplication Check
    if (isDuplicate(header.srcAddr, header.packetId)) {
      Serial.print("[RELAY] Dropping duplicate packet #");
      Serial.print(header.packetId);
      Serial.print(" from 0x");
      Serial.println(header.srcAddr, HEX);
      
      lora.receive();
      return;
    }

    // Record packet signature
    rememberPacket(header.srcAddr, header.packetId);

    // 2. Check if packet is destined for this specific Relay Node
    if (header.destAddr == RELAY_NODE_ID) {
      Serial.print("[RX CONSUMED] Destination match! Payload: ");
      for (int i = 0; i < payloadLen; i++) Serial.print((char)payload[i]);
      Serial.println();
      
      lora.receive();
      return;
    }

    // 3. Check Hop Limit
    if (header.hopCount >= MAX_HOPS) {
      Serial.print("[RELAY] Packet #");
      Serial.print(header.packetId);
      Serial.println(" reached MAX_HOPS limit. Dropping.");
      
      lora.receive();
      return;
    }

    // 4. Relay / Re-transmit Frame
    Serial.print("[RELAYING] Packet #");
    Serial.print(header.packetId);
    Serial.print(" (Src: 0x"); Serial.print(header.srcAddr, HEX);
    Serial.print(" -> Dest: 0x"); Serial.print(header.destAddr, HEX);
    Serial.print(") | Hops: "); Serial.print(header.hopCount);
    Serial.print(" | RSSI: "); Serial.print(rssi); Serial.println(" dBm");

    // Increment hop counter
    header.hopCount++;

    // Random backoff delay (50ms - 200ms) to prevent collisions with other relays
    delay(random(50, 200));

    // Send modified frame header + same payload
    if (lora.sendFrame(header, payload, payloadLen)) {
      Serial.println("[RELAY] Forwarded successfully!");
    } else {
      Serial.println("[RELAY] Retransmission failed.");
    }

    // Return to continuous receiving mode
    lora.receive();
  }
}