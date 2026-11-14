#pragma once

// Pin assignments for the Waveshare ESP32-S3-Touch-AMOLED-1.75.
// Source: github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75 (libraries/Mylibrary/pin_config.h)

#define XPOWERS_CHIP_AXP2101

// 466x466 AMOLED display with CO5300 over QSPI.
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RESET 1
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// CST9217 capacitive touch controller over I2C.
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11
#define TP_RESET 2

// ES8311 audio. The actual MCLK is GPIO42 (verified on the board with
// PlaneRadar2.0); GPIO16 was listed previously. The codec uses a clock derived
// from BCLK, so MCLK has little effect on its configuration.
#define I2S_MCK_IO 16
#define I2S_BCK_IO 9
#define I2S_DI_IO 10
#define I2S_WS_IO 45
#define I2S_DO_IO 8
#define PA 46

// The QMI8658 six-axis IMU shares I2C with touch, RTC, and PMU.
// Its 7-bit address is 0x6B. This pinout has no IMU interrupt pin, so firmware
// polls the sensor rather than waiting on a wake GPIO.
#define QMI8658_I2C_ADDR 0x6B

// TF card slot (not currently used).
#define SDMMC_CLK -1
#define SDMMC_CMD -1
#define SDMMC_DATA -1
#define SDMMC_CS -1
