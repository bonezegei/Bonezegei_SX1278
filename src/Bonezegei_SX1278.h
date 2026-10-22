/*
    Bonezegei_SX1278 Library
    Author: Jofel Batutay (Bonezegei)
    Date: October 23, 2026
*/

#ifndef BONEZEGEI_SX1278_H
#define BONEZEGEI_SX1278_H

#include <Arduino.h>
#include <SPI.h>

// Registers
#define SX1278_REG_FIFO                    0x00
#define SX1278_REG_OP_MODE                 0x01
#define SX1278_REG_FRF_MSB                 0x06
#define SX1278_REG_FRF_MID                 0x07
#define SX1278_REG_FRF_LSB                 0x08
#define SX1278_REG_PA_CONFIG               0x09
#define SX1278_REG_FIFO_ADDR_PTR           0x0D
#define SX1278_REG_FIFO_TX_BASE_ADDR       0x0E
#define SX1278_REG_FIFO_RX_BASE_ADDR       0x0F
#define SX1278_REG_FIFO_RX_CURRENT_ADDR    0x10
#define SX1278_REG_IRQ_FLAGS               0x12
#define SX1278_REG_RX_NB_BYTES             0x13
#define SX1278_REG_PKT_SNR_VALUE           0x19
#define SX1278_REG_PKT_RSSI_VALUE          0x1A
#define SX1278_REG_MODEM_CONFIG_1          0x1D
#define SX1278_REG_MODEM_CONFIG_2          0x1E
#define SX1278_REG_PREAMBLE_MSB            0x20
#define SX1278_REG_PREAMBLE_LSB            0x21
#define SX1278_REG_PAYLOAD_LENGTH          0x22
#define SX1278_REG_MODEM_CONFIG_3          0x26
#define SX1278_REG_SYNC_WORD               0x39
#define SX1278_REG_DIO_MAPPING_1           0x40
#define SX1278_REG_VERSION                 0x42

// Modes
#define SX1278_MODE_LONG_RANGE_MODE        0x80
#define SX1278_MODE_SLEEP                  0x00
#define SX1278_MODE_STDBY                  0x01
#define SX1278_MODE_TX                     0x03
#define SX1278_MODE_RX_CONTINUOUS          0x05
#define SX1278_MODE_RX_SINGLE              0x06

// IRQ Flags
#define SX1278_IRQ_RX_DONE_MASK            0x40
#define SX1278_IRQ_PAYLOAD_CRC_ERROR_MASK  0x20
#define SX1278_IRQ_TX_DONE_MASK            0x08

// Generic Frame Structure for future Mesh/Protocol encapsulation
struct __attribute__((packed)) Bonezegei_FrameHeader {
    uint8_t destAddr;    // Destination node ID (0xFF for Broadcast)
    uint8_t srcAddr;     // Source node ID
    uint8_t packetId;    // Sequence/Packet ID
    uint8_t hopCount;    // Hops remaining or elapsed
    uint8_t flags;       // Protocol flags (e.g. ACK request, Mesh control)
};

// Callback type for protocol routing hooks
typedef void (*Bonezegei_PacketCallback)(uint8_t* payload, uint8_t length, int16_t rssi, int8_t snr);

class Bonezegei_SX1278 {
public:
    Bonezegei_SX1278(uint8_t csPin, uint8_t resetPin, uint8_t dio0Pin = 255);
    
    bool begin(uint32_t frequency = 433E6, SPIClass &spiBus = SPI);
    void end();

    // Transceiver Configuration
    void setFrequency(uint32_t frequency);
    void setTxPower(uint8_t power);           // 2 to 17 dBm
    void setSpreadingFactor(uint8_t sf);      // SF6 to SF12
    void setBandwidth(uint32_t bw);           // e.g., 125E3, 250E3, 500E3
    void setCodingRate(uint8_t cr);           // 5 (4/5) to 8 (4/8)
    void setSyncWord(uint8_t syncWord);       // 0x12 (Private) / 0x34 (Public)
    void setPreambleLength(uint16_t length);
    void enableCrc(bool enable = true);

    // High-Level Data TX/RX
    bool send(const uint8_t *data, uint8_t length);
    bool send(const char *str);
    
    // Mesh/Protocol Ready Tx Interface
    bool sendFrame(const Bonezegei_FrameHeader &header, const uint8_t *payload, uint8_t payloadLen);

    int parsePacket();
    int readBytes(uint8_t *buffer, uint8_t maxLength);
    String readString();

    // Signal Metrics
    int16_t getPacketRssi();
    int8_t getPacketSnr();

    // Operational Modes
    void sleep();
    void idle();
    void receive();

    // Event & Protocol Hooks
    void onReceive(Bonezegei_PacketCallback callback);
    void handleInterrupt();

    // Direct Register Access (Useful for low-level protocol development)
    uint8_t readRegister(uint8_t address);
    void writeRegister(uint8_t address, uint8_t value);

private:
    uint8_t _csPin;
    uint8_t _resetPin;
    uint8_t _dio0Pin;
    uint32_t _frequency;
    SPIClass *_spi;
    Bonezegei_PacketCallback _onReceiveCallback;

    void setMode(uint8_t mode);
    void resetHardware();
};

#endif
