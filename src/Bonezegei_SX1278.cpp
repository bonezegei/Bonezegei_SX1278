/*
    Bonezegei_SX1278 Library
    Author: Jofel Batutay (Bonezegei)
    Date: October 23, 2026
*/

#include "Bonezegei_SX1278.h"

Bonezegei_SX1278::Bonezegei_SX1278(uint8_t csPin, uint8_t resetPin, uint8_t dio0Pin) {
    _csPin = csPin;
    _resetPin = resetPin;
    _dio0Pin = dio0Pin;
    _frequency = 433E6;
    _spi = &SPI;
    _onReceiveCallback = nullptr;
}

bool Bonezegei_SX1278::begin(uint32_t frequency, SPIClass &spiBus) {
    _spi = &spiBus;
    _frequency = frequency;

    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    if (_resetPin != 255) {
        pinMode(_resetPin, OUTPUT);
        resetHardware();
    }

    _spi->begin();

    // Check module signature
    uint8_t version = readRegister(SX1278_REG_VERSION);
    if (version != 0x12) {
        return false;
    }

    // Set chip to Sleep to enable LoRa mode switch
    sleep();

    // Configure Defaults
    setFrequency(_frequency);
    setTxPower(17);
    setSpreadingFactor(7);
    setBandwidth(125E3);
    setCodingRate(5);
    setSyncWord(0x12);
    setPreambleLength(8);
    enableCrc(true);

    // FIFO Base Addresses
    writeRegister(SX1278_REG_FIFO_TX_BASE_ADDR, 0x00);
    writeRegister(SX1278_REG_FIFO_RX_BASE_ADDR, 0x00);

    idle();
    return true;
}

void Bonezegei_SX1278::end() {
    sleep();
    digitalWrite(_csPin, HIGH);
}

void Bonezegei_SX1278::resetHardware() {
    digitalWrite(_resetPin, LOW);
    delay(10);
    digitalWrite(_resetPin, HIGH);
    delay(10);
}

uint8_t Bonezegei_SX1278::readRegister(uint8_t address) {
    digitalWrite(_csPin, LOW);
    _spi->beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    _spi->transfer(address & 0x7F); // Read bit (MSB = 0)
    uint8_t response = _spi->transfer(0x00);
    _spi->endTransaction();
    digitalWrite(_csPin, HIGH);
    return response;
}

void Bonezegei_SX1278::writeRegister(uint8_t address, uint8_t value) {
    digitalWrite(_csPin, LOW);
    _spi->beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    _spi->transfer(address | 0x80); // Write bit (MSB = 1)
    _spi->transfer(value);
    _spi->endTransaction();
    digitalWrite(_csPin, HIGH);
}

void Bonezegei_SX1278::setMode(uint8_t mode) {
    writeRegister(SX1278_REG_OP_MODE, SX1278_MODE_LONG_RANGE_MODE | mode);
}

void Bonezegei_SX1278::sleep() {
    setMode(SX1278_MODE_SLEEP);
}

void Bonezegei_SX1278::idle() {
    setMode(SX1278_MODE_STDBY);
}

void Bonezegei_SX1278::receive() {
    writeRegister(SX1278_REG_DIO_MAPPING_1, 0x00); // DIO0 mapped to RxDone
    setMode(SX1278_MODE_RX_CONTINUOUS);
}

void Bonezegei_SX1278::setFrequency(uint32_t frequency) {
    _frequency = frequency;
    uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
    writeRegister(SX1278_REG_FRF_MSB, (uint8_t)(frf >> 16));
    writeRegister(SX1278_REG_FRF_MID, (uint8_t)(frf >> 8));
    writeRegister(SX1278_REG_FRF_LSB, (uint8_t)(frf));
}

void Bonezegei_SX1278::setTxPower(uint8_t power) {
    if (power < 2) power = 2;
    if (power > 17) power = 17;
    // PA_BOOST enabled
    writeRegister(SX1278_REG_PA_CONFIG, 0x80 | (power - 2));
}

void Bonezegei_SX1278::setSpreadingFactor(uint8_t sf) {
    if (sf < 6) sf = 6;
    if (sf > 12) sf = 12;

    uint8_t config2 = readRegister(SX1278_REG_MODEM_CONFIG_2) & 0x0F;
    writeRegister(SX1278_REG_MODEM_CONFIG_2, config2 | (sf << 4));

    // Low Data Rate Optimization handling for SF11 & SF12 with 125kHz
    uint8_t config3 = readRegister(SX1278_REG_MODEM_CONFIG_3);
    if (sf >= 11) {
        config3 |= 0x08;
    } else {
        config3 &= ~0x08;
    }
    writeRegister(SX1278_REG_MODEM_CONFIG_3, config3 | 0x04); // Auto AGC
}

void Bonezegei_SX1278::setBandwidth(uint32_t bw) {
    uint8_t bwValue = 7; // Default 125kHz
    if (bw <= 7.8E3) bwValue = 0;
    else if (bw <= 10.4E3) bwValue = 1;
    else if (bw <= 15.6E3) bwValue = 2;
    else if (bw <= 20.8E3) bwValue = 3;
    else if (bw <= 31.25E3) bwValue = 4;
    else if (bw <= 41.7E3) bwValue = 5;
    else if (bw <= 62.5E3) bwValue = 6;
    else if (bw <= 125E3) bwValue = 7;
    else if (bw <= 250E3) bwValue = 8;
    else bwValue = 9; // 500kHz

    uint8_t config1 = readRegister(SX1278_REG_MODEM_CONFIG_1) & 0x0F;
    writeRegister(SX1278_REG_MODEM_CONFIG_1, config1 | (bwValue << 4));
}

void Bonezegei_SX1278::setCodingRate(uint8_t cr) {
    if (cr < 5) cr = 5;
    if (cr > 8) cr = 8;

    uint8_t crVal = cr - 4;
    uint8_t config1 = readRegister(SX1278_REG_MODEM_CONFIG_1) & 0xF1;
    writeRegister(SX1278_REG_MODEM_CONFIG_1, config1 | (crVal << 1));
}

void Bonezegei_SX1278::setSyncWord(uint8_t syncWord) {
    writeRegister(SX1278_REG_SYNC_WORD, syncWord);
}

void Bonezegei_SX1278::setPreambleLength(uint16_t length) {
    writeRegister(SX1278_REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
    writeRegister(SX1278_REG_PREAMBLE_LSB, (uint8_t)(length));
}

void Bonezegei_SX1278::enableCrc(bool enable) {
    uint8_t config2 = readRegister(SX1278_REG_MODEM_CONFIG_2);
    if (enable) {
        config2 |= 0x04;
    } else {
        config2 &= ~0x04;
    }
    writeRegister(SX1278_REG_MODEM_CONFIG_2, config2);
}

bool Bonezegei_SX1278::send(const uint8_t *data, uint8_t length) {
    idle();

    writeRegister(SX1278_REG_FIFO_ADDR_PTR, 0x00);
    writeRegister(SX1278_REG_PAYLOAD_LENGTH, length);

    for (uint8_t i = 0; i < length; i++) {
        writeRegister(SX1278_REG_FIFO, data[i]);
    }

    setMode(SX1278_MODE_TX);

    uint32_t start = millis();
    while (!(readRegister(SX1278_REG_IRQ_FLAGS) & SX1278_IRQ_TX_DONE_MASK)) {
        if (millis() - start > 2000) { // Timeout
            return false;
        }
        yield();
    }

    writeRegister(SX1278_REG_IRQ_FLAGS, SX1278_IRQ_TX_DONE_MASK);
    idle();
    return true;
}

bool Bonezegei_SX1278::send(const char *str) {
    return send((const uint8_t*)str, strlen(str));
}

bool Bonezegei_SX1278::sendFrame(const Bonezegei_FrameHeader &header, const uint8_t *payload, uint8_t payloadLen) {
    uint8_t totalLen = sizeof(Bonezegei_FrameHeader) + payloadLen;
    uint8_t buffer[256];

    memcpy(buffer, &header, sizeof(Bonezegei_FrameHeader));
    if (payloadLen > 0 && payload != nullptr) {
        memcpy(buffer + sizeof(Bonezegei_FrameHeader), payload, payloadLen);
    }

    return send(buffer, totalLen);
}

int Bonezegei_SX1278::parsePacket() {
    uint8_t irqFlags = readRegister(SX1278_REG_IRQ_FLAGS);
    writeRegister(SX1278_REG_IRQ_FLAGS, irqFlags); // Clear flags

    if ((irqFlags & SX1278_IRQ_RX_DONE_MASK) && !(irqFlags & SX1278_IRQ_PAYLOAD_CRC_ERROR_MASK)) {
        uint8_t currentAddr = readRegister(SX1278_REG_FIFO_RX_CURRENT_ADDR);
        writeRegister(SX1278_REG_FIFO_ADDR_PTR, currentAddr);
        return readRegister(SX1278_REG_RX_NB_BYTES);
    }

    return 0;
}

int Bonezegei_SX1278::readBytes(uint8_t *buffer, uint8_t maxLength) {
    uint8_t rxLength = readRegister(SX1278_REG_RX_NB_BYTES);
    uint8_t len = rxLength < maxLength ? rxLength : maxLength;

    for (uint8_t i = 0; i < len; i++) {
        buffer[i] = readRegister(SX1278_REG_FIFO);
    }

    return len;
}

String Bonezegei_SX1278::readString() {
    uint8_t length = readRegister(SX1278_REG_RX_NB_BYTES);
    uint8_t buffer[256];
    int readLen = readBytes(buffer, length);
    buffer[readLen] = '\0';
    return String((char*)buffer);
}

int16_t Bonezegei_SX1278::getPacketRssi() {
    return -157 + readRegister(SX1278_REG_PKT_RSSI_VALUE);
}

int8_t Bonezegei_SX1278::getPacketSnr() {
    return ((int8_t)readRegister(SX1278_REG_PKT_SNR_VALUE)) / 4;
}

void Bonezegei_SX1278::onReceive(Bonezegei_PacketCallback callback) {
    _onReceiveCallback = callback;
}

void Bonezegei_SX1278::handleInterrupt() {
    int packetLen = parsePacket();
    if (packetLen > 0 && _onReceiveCallback != nullptr) {
        uint8_t buffer[256];
        readBytes(buffer, packetLen);
        _onReceiveCallback(buffer, packetLen, getPacketRssi(), getPacketSnr());
    }
}