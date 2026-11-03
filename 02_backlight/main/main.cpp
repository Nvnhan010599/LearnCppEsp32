#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "sdkconfig.h"

#include "pmu_port.h"

namespace pmu_cfg {
constexpr const char *TAG = "main";
constexpr i2c_port_num_t kI2cPort = static_cast<i2c_port_num_t>(CONFIG_I2C_MASTER_PORT_NUM);
constexpr uint32_t kI2cFreqHz = CONFIG_I2C_MASTER_FREQUENCY;
constexpr gpio_num_t kSdaIo = static_cast<gpio_num_t>(CONFIG_PMU_I2C_SDA);
constexpr gpio_num_t kSclIo = static_cast<gpio_num_t>(CONFIG_PMU_I2C_SCL);
constexpr int kTimeoutMs = 1000;
constexpr uint16_t kPmuAddr = 0x34;

}

static i2c_master_bus_handle_t i2c_bus_handle = nullptr;
static i2c_master_dev_handle_t pmu_dev_handle = nullptr;

// I2C init with new API
esp_err_t i2c_init() {
    
    i2c_master_bus_config_t bus_config = {
        .i2c_port = pmu_cfg::kI2cPort,
        .sda_io_num = pmu_cfg::kSdaIo,
        .scl_io_num = pmu_cfg::kSclIo,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
            .allow_pd = 0
        }
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = pmu_cfg::kPmuAddr,
        .scl_speed_hz = pmu_cfg::kI2cFreqHz,
        .scl_wait_us = 0,
        .flags = {
            .disable_ack_check = 0
        }
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &pmu_dev_handle));

    return ESP_OK;
}

// PMU read function using new API
I2cStatus pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
    (void)devAddr;
    esp_err_t ret = i2c_master_transmit_receive(pmu_dev_handle, &regAddr, 1, data, len, pmu_cfg::kTimeoutMs);
    if (ret != ESP_OK) {
        ESP_LOGE(pmu_cfg::TAG, "PMU READ FAILED!");
        return I2cStatus::Fail;
    }
    return I2cStatus::Ok;
}

// PMU write function using new API
I2cStatus pmu_register_write_byte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
    (void)devAddr;
    uint8_t *buffer = (uint8_t *)malloc(len + 1);
    if (!buffer) return I2cStatus::Fail;
    buffer[0] = regAddr;
    memcpy(&buffer[1], data, len);

    esp_err_t ret = i2c_master_transmit(pmu_dev_handle, buffer, len + 1, pmu_cfg::kTimeoutMs);
    free(buffer);

    if (ret != ESP_OK) {
        ESP_LOGE(pmu_cfg::TAG, "PMU WRITE FAILED!");
        return I2cStatus::Fail;
    }
    return I2cStatus::Ok;
}

// PMU event task
static void pmu_hander_task(void *args) {
    while (1) {
        pmu_isr_handler();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(i2c_init());
    ESP_LOGI(pmu_cfg::TAG, "I2C initialized successfully");

    ESP_ERROR_CHECK(pmu_init());

    xTaskCreate(pmu_hander_task, "App/pwr", 4 * 1024, nullptr, 10, nullptr);
}
