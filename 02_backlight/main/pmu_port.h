#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include <cstdint>
enum class I2cStatus : int { 
    Ok = 0, 
    Fail = -1 
};

esp_err_t pmu_init();
void pmu_isr_handler();
I2cStatus pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
I2cStatus pmu_register_write_byte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);