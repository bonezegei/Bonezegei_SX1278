#include <Bonezegei_SX1278.h>

#define CS_PIN    10
#define RST_PIN   9
#define DIO0_PIN  2

// SX1278 Register for +20dBm High Power Mode Boost
#define SX1278_REG_PA_DAC  0x4D

Bonezegei_SX1278 lora(CS_PIN, RST_PIN, DIO0_PIN);

// Helper function to unlock maximum +20 dBm output power (100mW)
void enableHighPower20dBm() {
    // 1. Set PA_CONFIG to max PA_BOOST (17 dBm level)
    lora.setTxPower(17);
    
    // 2. Enable High Power +20 dBm DAC register setting (0x87 = +20dBm, 0x84 = Default)
    lora.writeRegister(SX1278_REG_PA_DAC, 0x87);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!lora.begin(433E6)) {
    Serial.println("[ERROR] SX1278 init failed!");
    while (1);
  }

  // ====================================================================
  // MAXIMUM SENSITIVITY & RANGE CONFIGURATION
  // ====================================================================

  // 1. Max Spreading Factor: SF12 (Maximum receiver sensitivity ~ -137 dBm)
  lora.setSpreadingFactor(12);

  // 2. Narrow Bandwidth: 62.5 kHz or 125 kHz (+3dB noise floor improvement)
  lora.setBandwidth(62.5E3);

  // 3. Max Forward Error Correction: CR 4/8 (Maximum resilience to interference)
  lora.setCodingRate(8);

  // 4. Extended Preamble: 12 symbols (Gives receiver AGC more time to lock)
  lora.setPreambleLength(12);

  // 5. CRC Check: Enabled to reject corrupted packets in weak signal areas
  lora.enableCrc(true);

  // 6. Max Output Power: Enable +20 dBm Boost Mode
  enableHighPower20dBm();

  Serial.println("=================================================");
  Serial.println("  Bonezegei_SX1278 High Sensitivity / Max Power ");
  Serial.println("=================================================");
  Serial.println("Frequency : 433 MHz");
  Serial.println("TX Power  : +20 dBm (100mW Boost)");
  Serial.println("SF        : SF12");
  Serial.println("Bandwidth : 62.5 kHz");
  Serial.println("CR        : 4/8");
  Serial.println("Preamble  : 12 Symbols");
  Serial.println("=================================================");

  lora.receive();
}

void loop() {
  // Test Transmission every 10 seconds
  static uint32_t lastTx = 0;
  if (millis() - lastTx > 10000) {
    lastTx = millis();

    Serial.println("\n[TX] Transmitting high-power long-range pulse...");
    
    if (lora.send("PING_MAX_RANGE")) {
      Serial.println("[TX] Transmission successful!");
    } else {
      Serial.println("[TX] Transmission timed out.");
    }

    // Return to continuous listening mode
    lora.receive();
  }

  // Check for incoming packets
  int packetSize = lora.parsePacket();
  if (packetSize > 0) {
    String msg = lora.readString();
    Serial.println("\n[RX] Packet Received!");
    Serial.print("Payload : "); Serial.println(msg);
    Serial.print("RSSI    : "); Serial.print(lora.getPacketRssi()); Serial.println(" dBm");
    Serial.print("SNR     : "); Serial.print(lora.getPacketSnr()); Serial.println(" dB");
  }
}