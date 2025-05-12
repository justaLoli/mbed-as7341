/* implementation of basic functions for AS7341 driver
 * Copyright (c) 2025 justaLoli
 * Licensed under the BSD License(see license.txt).
 * The implementation is inspired by
 *     https://github.com/adafruit/Adafruit_AS7341
 */

#include "AS7341.h"

AS7341::AS7341(PinName sda, PinName scl, uint32_t addr, uint32_t freq) {
    i2c = new I2C(sda, scl);
    i2c->frequency(freq);
    i2c_address = addr;
}

AS7341::~AS7341() { delete i2c; }

bool AS7341::readRegister(char addr, char *val) {
    if (i2c->write((char)i2c_address, (char *)&addr, 1, true) != 0)
        return false;
    if (i2c->read((char)i2c_address, (char *)val, 1) != 0)
        return false;
    return true;
}
bool AS7341::writeRegister(char addr, char val) {
    char data[2] = {addr, val};
    return (i2c->write(i2c_address, data, 2) == 0);
};

bool AS7341::begin() {
    // test i2c communication
    char test_byte = 0x00;
    if (i2c->write(i2c_address, &test_byte, 1) != 0) {
        // error happened
        return false;
    }
    return _init(0);
}
bool AS7341::_init(char sensor_id) {
    // 跳过了 make sure talking to right chip 的部分
    return powerEnable(true);
}
bool AS7341::powerEnable(bool enable_power) {
    char reg_val;
    if (not readRegister(AS7341_ENABLE, &reg_val))
        return false;
    // 设置或清除 bit 0（PON）
    if (enable_power) {
        reg_val |= (1 << 0); // 置位 bit 0
    } else {
        reg_val &= ~(1 << 0); // 清除 bit 0
    }
    return writeRegister(AS7341_ENABLE, reg_val);
};

/**
 * Sets the integration time step count
 *
 * Total integration time will be `(ATIME + 1) * (ASTEP + 1) * 2.78µS`
 */
bool AS7341::setATIME(char atime_value) {
    return writeRegister(AS7341_ATIME, (char)atime_value);
};

/**
 * Sets the integration time step size
 */
bool AS7341::setASTEP(uint16_t astep_value) {
    char data[3];
    data[0] = AS7341_ASTEP_L;
    data[1] = astep_value & 0xff; // 低8位
    data[2] = (astep_value >> 8) & 0xff;
    if (i2c->write(i2c_address, data, 3) != 0)
        return false;
    return true;
};

/**
 * Sets the ADC gain multiplier
 */
bool AS7341::setGain(as7341_gain_t gain_value) {
    return writeRegister(AS7341_CFG1, static_cast<char>(gain_value));
};

bool AS7341::readAllChannels() { return readAllChannels(_channel_readings); };

uint16_t AS7341::getChannel(as7341_color_channel_t channel) {
    return _channel_readings[channel];
}

bool AS7341::readAllChannels(uint16_t *reading_buffers) {
    setSMUXLowChannels(true);
    enableSpectralMeasurement(true); // 启动积分
    delayForData(0);

    // char data[12];
    char reg_addr = AS7341_CH0_DATA_L;
    if (i2c->write(i2c_address, &reg_addr, 1, true) != 0)
        return false;
    // 据说uint16在嵌入式设备里是低位在前的。其实直接读入也应该没有问题。
    if (i2c->read(i2c_address, (char *)reading_buffers, 12) != 0)
        return false;

    setSMUXLowChannels(false);
    enableSpectralMeasurement(true); // 启动积分
    delayForData(0);

    if (i2c->write(i2c_address, &reg_addr, 1, true) != 0)
        return false;
    if (i2c->read(i2c_address, (char *)&reading_buffers[6], 12) != 0)
        return false;
    // for (int i = 0; i < 6; i ++){
    //     reading_buffers[6+i] = (data[2*i] | data[2*i + 1] <<
    //     8);//拼接两个8位到一个16位
    // }
    return true;
};

bool AS7341::delayForData(int time) {
    (void)time;
    while (!getIsDataReady()) {
        wait_ms(1);
    }
    return true;
};
bool AS7341::getIsDataReady() {
    char reg_val;
    readRegister(AS7341_STATUS2, &reg_val);
    return reg_val & (1 << 6);
};

bool AS7341::setSMUXLowChannels(bool f1_f4) {
    bool status = true;
    status &= enableSpectralMeasurement(false);
    status &= setSMUXCommand(AS7341_SMUX_CMD_WRITE);
    if (f1_f4) {
        status &= setup_F1F4_Clear_NIR();
    } else {
        status &= setup_F5F8_Clear_NIR();
    }
    status &= enableSMUX();
    return status;
};

bool AS7341::enableSMUX() {
    char reg_val;
    if (not readRegister(AS7341_ENABLE, &reg_val))
        return false;
    reg_val |= (1 << 4);
    bool res = writeRegister(AS7341_ENABLE, reg_val);
    wait_ms(1000); // wait for whatever reason
    return res;
};

bool AS7341::enableSpectralMeasurement(bool enable_measurement) {

    char reg_val;
    if (not readRegister(AS7341_ENABLE, &reg_val))
        return false;

    if (enable_measurement) {
        reg_val |= (1 << 1);
    } else {
        reg_val &= ~(1 << 1);
    }

    return writeRegister(AS7341_ENABLE, reg_val);
};

bool AS7341::setSMUXCommand(as7341_smux_cmd_t command) {
    char reg_val;
    if (not readRegister(AS7341_CFG6, &reg_val))
        return false;
    // 定义一个掩码，用于清除 bit 3 和 bit 4
    const char mask = ~((1 << 3) | (1 << 4)); // 0b11100111 或 0xE7
    // 清除 reg_val 中 bit 3 和 bit 4 的值
    reg_val &= mask;
    // 将新的 command 值左移3位，使其对齐到 bit 3 的位置
    char command_shifted = (char)command << 3;
    // 将清除后的 reg_val 和 移位后的 command 值进行或运算，得到最终要写入的值
    char new_reg_val = reg_val | command_shifted;
    return writeRegister(AS7341_CFG6, new_reg_val);
};
bool AS7341::setup_F1F4_Clear_NIR() {
    // SMUX Config for F1,F2,F3,F4,NIR,Clear
    writeRegister(byte(0x00), byte(0x30)); // F3 left set to ADC2
    writeRegister(byte(0x01), byte(0x01)); // F1 left set to ADC0
    writeRegister(byte(0x02), byte(0x00)); // Reserved or disabled
    writeRegister(byte(0x03), byte(0x00)); // F8 left disabled
    writeRegister(byte(0x04), byte(0x00)); // F6 left disabled
    writeRegister(
        byte(0x05),
        byte(0x42)); // F4 left connected to ADC3/f2 left connected to ADC1
    writeRegister(byte(0x06), byte(0x00)); // F5 left disbled
    writeRegister(byte(0x07), byte(0x00)); // F7 left disbled
    writeRegister(byte(0x08), byte(0x50)); // CLEAR connected to ADC4
    writeRegister(byte(0x09), byte(0x00)); // F5 right disabled
    writeRegister(byte(0x0A), byte(0x00)); // F7 right disabled
    writeRegister(byte(0x0B), byte(0x00)); // Reserved or disabled
    writeRegister(byte(0x0C), byte(0x20)); // F2 right connected to ADC1
    writeRegister(byte(0x0D), byte(0x04)); // F4 right connected to ADC3
    writeRegister(byte(0x0E), byte(0x00)); // F6/F8 right disabled
    writeRegister(byte(0x0F), byte(0x30)); // F3 right connected to AD2
    writeRegister(byte(0x10), byte(0x01)); // F1 right connected to AD0
    writeRegister(byte(0x11), byte(0x50)); // CLEAR right connected to AD4
    writeRegister(byte(0x12), byte(0x00)); // Reserved or disabled
    writeRegister(byte(0x13), byte(0x06)); // NIR connected to ADC5
    return true;
};
bool AS7341::setup_F5F8_Clear_NIR() {
    // SMUX Config for F5,F6,F7,F8,NIR,Clear
    writeRegister(byte(0x00), byte(0x00)); // F3 left disable
    writeRegister(byte(0x01), byte(0x00)); // F1 left disable
    writeRegister(byte(0x02), byte(0x00)); // reserved/disable
    writeRegister(byte(0x03), byte(0x40)); // F8 left connected to ADC3
    writeRegister(byte(0x04), byte(0x02)); // F6 left connected to ADC1
    writeRegister(byte(0x05), byte(0x00)); // F4/ F2 disabled
    writeRegister(byte(0x06), byte(0x10)); // F5 left connected to ADC0
    writeRegister(byte(0x07), byte(0x03)); // F7 left connected to ADC2
    writeRegister(byte(0x08), byte(0x50)); // CLEAR Connected to ADC4
    writeRegister(byte(0x09), byte(0x10)); // F5 right connected to ADC0
    writeRegister(byte(0x0A), byte(0x03)); // F7 right connected to ADC2
    writeRegister(byte(0x0B), byte(0x00)); // Reserved or disabled
    writeRegister(byte(0x0C), byte(0x00)); // F2 right disabled
    writeRegister(byte(0x0D), byte(0x00)); // F4 right disabled
    writeRegister(byte(0x0E),
                  byte(0x24)); // F8 right connected to ADC2/ F6 right
                               // connected to ADC1
    writeRegister(byte(0x0F), byte(0x00)); // F3 right disabled
    writeRegister(byte(0x10), byte(0x00)); // F1 right disabled
    writeRegister(byte(0x11), byte(0x50)); // CLEAR right connected to AD4
    writeRegister(byte(0x12), byte(0x00)); // Reserved or disabled
    writeRegister(byte(0x13), byte(0x06)); // NIR connected to ADC5
    return true;
};
char AS7341::byte(char t) { return t; };
