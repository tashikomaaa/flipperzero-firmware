#pragma once

#include <furi_hal_crc.h>
#include <furi_hal_flash.h>
#include <furi_hal_i2c.h>
#include <furi_hal_light.h>
#include <furi_hal_power.h>
#include <furi_hal_resources.h>
#include <furi_hal_sd.h>
#include <furi_hal_spi.h>
#include <furi_hal_version.h>
#include <check.h>

#define furi_assert(value) (void)(value)

void furi_hal_init();

void delay(float milliseconds);

void delay_us(float microseconds);