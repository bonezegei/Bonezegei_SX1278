#include <Bonezegei_SX1278.h>

#define CS_PIN    10
#define RST_PIN   9
#define DIO0_PIN  2

Bonezegei_SX1278 lora(CS_PIN, RST_PIN, DIO0_PIN);

// Helper function to print hex bytes with leading zeros
void printHexByte(uint8_t b) {
  if (b < 0x10) Serial.print("0");
  Serial.print(b, HEX);
  Serial.print(" ");
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize transceiver on target frequency (433 MHz)
  if (!lora.begin(433E6)) {
    Serial.println("[ERROR] SX1278 initialization failed!");
    while (1);
  }

  // RF Physical Configurations
  lora.setSpreadingFactor(7);   // Default SF: 7 (Matches most transmitters)
  lora.setBandwidth(125E3);     // Default BW: 125 kHz
  lora.setCodingRate(5);        // CR 4/5
  lora.setSyncWord(0x12);       // 0x12 = Private, 0x34 = Public / LoRaWAN
  lora.enableCrc(true);         // Reject corrupted hardware frames

  // Enable continuous receive mode
  lora.receive();

  Serial.println("==================================================");
  Serial.println("   Bonezegei_SX1278 Universal Receiver / Sniffer   ");
  Serial.println("   Listening promiscuously to all raw frames...  ");
  Serial.println("==================================================");
}

void loop() {
  int packetSize = lora.parsePacket();
  
  if (packetSize > 0) {
    uint8_t buffer[256];
    int len = lora.readBytes(buffer, packetSize);

    // Header Metadata Display
    Serial.println("\n=================[ PACKET RECEIVED ]=================");
    Serial.print("Length  : "); Serial.print(len); Serial.println(" bytes");
    Serial.print("RSSI    : "); Serial.print(lora.getPacketRssi()); Serial.println(" dBm");
    Serial.print("SNR     : "); Serial.print(lora.getPacketSnr()); Serial.println(" dB");

    // Raw Hexadecimal Payload Dump
    Serial.print("HEX Dump: ");
    for (int i = 0; i < len; i++) {
      printHexByte(buffer[i]);
    }
    Serial.println();

    // Readable ASCII Payload Representation
    Serial.print("ASCII   : ");
    for (int i = 0; i < len; i++) {
      if (buffer[i] >= 32 && buffer[i] <= 126) {
        Serial.print((char)buffer[i]); // Printable characters
      } else {
        Serial.print('.');             // Non-printable substitute
      }
    }
    Serial.println();

    // Protocol Header Inspection Hook
    if (len >= sizeof(Bonezegei_FrameHeader)) {
      Bonezegei_FrameHeader header;
      memcpy(&header, buffer, sizeof(Bonezegei_FrameHeader));
      
      Serial.println("--- Detected Frame Header ---");
      Serial.print("Dest Addr : 0x"); Serial.println(header.destAddr, HEX);
      Serial.print("Src Addr  : 0x"); Serial.println(header.srcAddr, HEX);
      Serial.print("Packet ID : "); Serial.println(header.packetId);
      Serial.print("Hops      : "); Serial.println(header.hopCount);
      Serial.print("Flags     : 0x"); Serial.println(header.flags, HEX);
    }
    
    Serial.println("=====================================================");
  }
}