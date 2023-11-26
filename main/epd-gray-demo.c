/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "driver/spi_common.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_system.h"
#include "hal/gpio_types.h"
#include "hal/spi_types.h"
#include "sdkconfig.h"
#include "soc/io_mux_reg.h"
#include "u8g2.h"

extern const char moon_xpm_start[] asm("_binary_moon_xpm_start");
extern const char moon_xpm_end[] asm("_binary_moon_xpm_end");

static const char *TAG = "epd-gray-demo";

#define CS_GPIO         GPIO_NUM_15
#define SCLK_GPIO       GPIO_NUM_4
#define SDIN_GPIO       GPIO_NUM_5
#define BUSY_GPIO       GPIO_NUM_18
#define DC_GPIO         GPIO_NUM_19
#define RESET_GPIO      GPIO_NUM_21
#define CS1_GPIO        GPIO_NUM_22

#define XPM_COLOR_LEVEL 4
#define EPD_WIDTH       122
#define EPD_HEIGHT      250

static void configure_gpio(void)
{
    gpio_reset_pin(BUSY_GPIO);
    gpio_reset_pin(DC_GPIO);
    gpio_reset_pin(RESET_GPIO);
    gpio_reset_pin(CS1_GPIO);
    gpio_reset_pin(CS_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BUSY_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(DC_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CS1_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(CS_GPIO, GPIO_MODE_OUTPUT);

}

static spi_device_handle_t spi_dev;

uint8_t u8g2_esp32_spi_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch(msg) {
    case U8X8_MSG_BYTE_SET_DC:
        gpio_set_level(DC_GPIO, arg_int);
        break;

    case U8X8_MSG_BYTE_INIT: {
        esp_err_t ret;

        spi_bus_config_t bus_cfg = {
            .mosi_io_num = SDIN_GPIO,
            .miso_io_num = 2,
            .sclk_io_num = SCLK_GPIO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 8192,
        };

        spi_device_interface_config_t dev_cfg = {
            .mode = 0,
            .clock_speed_hz = 1000000,
            .spics_io_num = -1,
            .queue_size = 64,
        };

        ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
        ESP_ERROR_CHECK(ret);

        ret = spi_bus_add_device(HSPI_HOST, &dev_cfg, &spi_dev);
        ESP_ERROR_CHECK(ret);
        break;
    }

    case U8X8_MSG_BYTE_SEND: {
        spi_transaction_t trans_desc;
        trans_desc.addr      = 0;
        trans_desc.cmd       = 0;
        trans_desc.flags     = 0;
        trans_desc.length    = 8 * arg_int; // Number of bits NOT number of bytes.
        trans_desc.rxlength  = 0;
        trans_desc.tx_buffer = arg_ptr;
        trans_desc.rx_buffer = NULL;

        while(gpio_get_level(BUSY_GPIO) == 1)
            vTaskDelay(10/portTICK_PERIOD_MS);

        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_dev, &trans_desc));
        break;
    }

    case U8X8_MSG_BYTE_START_TRANSFER:
        gpio_set_level(CS_GPIO, 0);
        gpio_set_level(CS1_GPIO, 0);
        break;
    case U8X8_MSG_BYTE_END_TRANSFER:
        gpio_set_level(CS_GPIO, 1);
        gpio_set_level(CS1_GPIO, 1);
        break;
    default:
        return 0;
        break;
    }
    return 0;
}

uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch(msg) {
    // Initialize the GPIO and DELAY HAL functions.  If the pins for DC and RESET have been
    // specified then we define those pins as GPIO outputs.
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            break;

    // Set the GPIO reset pin to the value passed in through arg_int.
        case U8X8_MSG_GPIO_RESET:
                gpio_set_level(RESET_GPIO, arg_int);
            break;
    // Set the GPIO client select pin to the value passed in through arg_int.
        case U8X8_MSG_GPIO_CS:
                gpio_set_level(CS_GPIO, arg_int);
            break;

        case U8X8_MSG_GPIO_CS1:
                gpio_set_level(CS1_GPIO, arg_int);
            break;

        case U8X8_MSG_GPIO_E:
                return (uint8_t)gpio_get_level(BUSY_GPIO);
            break;

    // Delay for the number of milliseconds passed in through arg_int.
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(arg_int/portTICK_PERIOD_MS);
            break;

        default:
            return 0;
            break;
    }
    return 0;
}

void u8x8_RefreshDisplayM(u8x8_t *u8x8, uint8_t v)
{
    u8x8->display_cb(u8x8, U8X8_MSG_DISPLAY_REFRESH, v, NULL);
}

static char *gray4_xpm_cov(const char *xpm)
{
    int ret;
    char *line_begin;
    char *line_end;
    uint32_t width, height, colors;

    char color_tab[4];
    char *pic;

    int i, j, k;

    /* 判断是否是XPM文件 */
    line_begin = (char *)xpm;
    line_end = strstr(line_begin, "\n");
    if (strstr(line_begin, "XPM") == NULL)
        return NULL;

    /* 忽略掉数组名称那一行 */
    line_begin = line_end + 1;
    line_end = strstr(line_begin, "\n");

    /* 读取图片尺寸和颜色信息 */
    line_begin = line_end + 1;
    line_end = strstr(line_begin, "\n");
    ret = sscanf(line_begin, "\"%d %d %d ", &width, &height, &colors);
    if (ret != 3)
        return NULL;

    ESP_LOGI(TAG, "width %d, height %d", width, height);

    pic = (char *)malloc(width * height);
    if (!pic)
        return NULL;

    /* 这里先默认认为色卡是由暗到亮按顺序排列的，并且没有alpha，可以通过GIMP来调整 */
    /* TODO 增加色卡的判断以及排序，防止色卡不是按照数值进行排序的 */
    for (i = 0; i < 4; i++) {
        line_begin = line_end + 1;
        line_end = strstr(line_begin, "\n");
        color_tab[i] = line_begin[1];
    }

    /* 循环读取数据并按照色卡转换为灰度等级进行存储, 0(全黑) - 3(全白) */
    memset(pic, 0, width * height);
    for (i = 0; i < height; i++) {
        line_begin = line_end + 1;
        line_end = strstr(line_begin, "\n");
        for (j = 0; j < width; j++) {
            for (k = 0; k < 4; k++) {
                /* +1跳过行首的引号 */
                if (line_begin[j + 1] == color_tab[k])
                    pic[width * i + j] = k;
            }
        }
    }

    return pic;
}

u8g2_t u8g2;

void app_main(void)
{
    char *pic;
    int i, j, k;

    configure_gpio();
    vTaskDelay(200/portTICK_PERIOD_MS);
    u8g2_Setup_ssd1619a_400x300_f(&u8g2, &u8g2_cb_r0, u8g2_esp32_spi_byte_cb, u8g2_esp32_gpio_and_delay_cb);
    u8x8_InitDisplay(u8g2_GetU8x8(&u8g2));
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_ClearBufferV(&u8g2, 0xff);
    u8g2_SendBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_wqy14_t_gb2312);
    u8g2_DrawUTF8(&u8g2, 13, 100, "你好，世界");
    u8g2_SendBuffer(&u8g2);

    u8g2_DrawStr(&u8g2, 5, 250, "Hello world");
    u8g2_UpdateDisplay(&u8g2);
    u8x8_RefreshDisplayM(u8g2_GetU8x8(&u8g2), 0x1);

    pic = gray4_xpm_cov(moon_xpm_start);
    if (!pic)
        return;

    u8g2_ClearBufferV(&u8g2, 0xff);
    u8g2_SendBuffer(&u8g2);

    /* 分3次进行刷新,因为全白不需要刷新 */
    for (i = 0; i < 3; i++) {
        u8g2_ClearBufferV(&u8g2, 0xff);
        for (j = 0; j < 300; j++) {
            for (k = 0; k < 400; k++) {
                if (pic[400 * j + k] < i+1)
                    u8g2_DrawPixel(&u8g2, k, j);
            }
        }
        u8g2_UpdateDisplay(&u8g2);
        u8x8_RefreshDisplayM(u8g2_GetU8x8(&u8g2), i < 2 ? 0x11 : 0x12 );
    }

    free(pic);
    u8g2_SetPowerSave(&u8g2, 1);
}
