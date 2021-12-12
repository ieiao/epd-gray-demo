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
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_system.h"
#include "hal/gpio_types.h"
#include "hal/spi_types.h"
#include "sdkconfig.h"
#include "soc/io_mux_reg.h"

static const char *TAG = "epd-gray-demo";

#define CS_GPIO         15
#define SCLK_GPIO       4
#define SDIN_GPIO       5
#define BUSY_GPIO       18
#define DC_GPIO         19
#define RESET_GPIO      21

#define XPM_COLOR_LEVEL 16
#define EPD_WIDTH       122
#define EPD_HEIGHT      250

extern const char R_C_xpm_start[] asm("_binary_R_C_16_xpm_start");
extern const char R_C_xpm_end[] asm("_binary_R_C_16_xpm_end");

const char epd_init_sequence[] = {
0x01,0xf9,0x00,0x3a,0x06,0x3b,0x0b,0x11,0x03,0x44,0x00,0x0f,0x45,
0x00,0xf9,0x2c,0xa0,0x3c,0x33,0x32,0x66,0x66,0x26,0x04,0x55,0xaa,
0x08,0x91,0x11,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x19,0x19,0x0a,
0x0a,0x5e,0x1e,0x1e,0x0a,0x39,0x14,0x00,0x00,0x00,0x02,0x21,0x83
};

char epd_gray_lut[]= {
0x99,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x02,0x21,0x83
};

static void configure_gpio(void)
{
    gpio_reset_pin(BUSY_GPIO);
    gpio_reset_pin(DC_GPIO);
    gpio_reset_pin(RESET_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BUSY_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(DC_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RESET_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_level(RESET_GPIO, 1);
}

static void epd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    while(gpio_get_level(BUSY_GPIO) == 1)
        vTaskDelay(10/portTICK_PERIOD_MS);
    gpio_set_level(DC_GPIO, dc);
}

static spi_device_handle_t configure_spi(void)
{
    esp_err_t ret;
    spi_device_handle_t spi_dev;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SDIN_GPIO,
        .miso_io_num = 2,
        .sclk_io_num = SCLK_GPIO,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
    };
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 4000000,
        .mode = 0,
        .spics_io_num = CS_GPIO,
        .queue_size = 64,
        .pre_cb = epd_spi_pre_transfer_callback,
    };

    ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(HSPI_HOST, &dev_cfg, &spi_dev);
    ESP_ERROR_CHECK(ret);

    return spi_dev;
}

static void epd_cmd(spi_device_handle_t dev, const char c)
{
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &c;
    t.user = (void *)0;
    spi_device_polling_transmit(dev, &t);
}

static void epd_data(spi_device_handle_t dev, const char d)
{
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &d;
    t.user = (void *)1;
    spi_device_polling_transmit(dev, &t);
}

static void epd_data_n(spi_device_handle_t dev, const char *d, uint32_t length)
{
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));
    t.length = 8 * length;
    t.tx_buffer = d;
    t.user = (void *)1;
    spi_device_polling_transmit(dev, &t);
}

static void epd_init(spi_device_handle_t dev)
{
    int i;

    gpio_set_level(RESET_GPIO, 0);
    vTaskDelay(10/portTICK_PERIOD_MS);
    gpio_set_level(RESET_GPIO, 1);
    vTaskDelay(30/portTICK_PERIOD_MS);

    epd_cmd(dev, 0x12);
    vTaskDelay(100/portTICK_PERIOD_MS);

    for (i = 0; i < 0x33; i++) {
        if (i == 0x0 || i == 0x3 || i == 0x5 || i == 0x7 ||
            i == 0x9 || i == 0xc || i == 0xf || i == 0x11 ||
            i == 0x13 || i == 0x32) {
            epd_cmd(dev, epd_init_sequence[i]);
        } else
            epd_data(dev, epd_init_sequence[i]);
    }
}

static void epd_set_cursor(spi_device_handle_t dev)
{
    epd_cmd(dev, 0x4e);
    epd_data(dev, 0);

    epd_cmd(dev, 0x4f);
    epd_data(dev, 0);
}

static void epd_full_screen_write(spi_device_handle_t dev, const char *d)
{
    epd_cmd(dev, 0x24);
    epd_data_n(dev, d, 4000);
}

static void epd_display_update(spi_device_handle_t dev)
{
    epd_cmd(dev, 0x22);
    epd_data(dev, 0xc7);
    epd_cmd(dev, 0x20);
}

static void epd_update_lut(spi_device_handle_t dev, const char *lut)
{
    epd_cmd(dev, 0x32);
    epd_data_n(dev, epd_gray_lut, 32);
}

static char *parse_xpm_file(const char *data)
{
    char *line_begin;
    char *line_end;

    char pic_color_tab[XPM_COLOR_LEVEL] = {0};
    char *pic_gray_bin;

    int i, j, k;

    pic_gray_bin = (char *)malloc(EPD_WIDTH * EPD_HEIGHT);
    if (!pic_gray_bin) {
        ESP_LOGE(TAG, "%d: 申请内存失败", __LINE__);
        return NULL;
    }

    line_begin = (char *)data;
    line_end = strstr(line_begin, "\n");
    if (strstr(line_begin, "XPM") == NULL) {
        ESP_LOGE(TAG, "%d: 似乎并不是XPM格式文件", __LINE__);
        return NULL;
    }

    /* 忽略掉数组名称那一行 */
    line_begin = line_end + 1;
    line_end = strstr(line_begin, "\n");

    /* 读取图片尺寸和颜色信息 */
    /* TODO 这里需要对尺寸进行一下检测*/
    line_begin = line_end + 1;
    line_end = strstr(line_begin, "\n");

    /* 这里先默认认为色卡是由暗到亮按顺序排列的，并且没有alpha，可以通过GIMP来调整 */
    /* TODO 增加色卡的判断以及排序，防止色卡不是按照数值进行排序的 */
    for (i = 0; i < XPM_COLOR_LEVEL; i++) {
        line_begin = line_end + 1;
        line_end = strstr(line_begin, "\n");
        pic_color_tab[i] = line_begin[1];
    }

    /* 循环读取数据并按照色卡转换为灰度等级进行存储, 0(全黑) - 15(全白) */
    memset(pic_gray_bin, 0, EPD_WIDTH * EPD_HEIGHT);
    for (i = 0; i < EPD_HEIGHT; i++) {
        line_begin = line_end + 1;
        line_end = strstr(line_begin, "\n");
        for (j = 0; j < EPD_WIDTH; j++) {
            for (k = 0; k < XPM_COLOR_LEVEL; k++) {
                /* +1跳过行首的引号 */
                if (line_begin[j + 1] == pic_color_tab[k])
                    pic_gray_bin[EPD_WIDTH * i + j] = k;
            }
        }
    }

    return pic_gray_bin;
}

void app_main(void)
{
    char *pic_gray_bin;
    char *epd_draw_buffer;
    spi_device_handle_t dev;
    int i, j, k;

    epd_draw_buffer = (char *)malloc(128/8 * 250);
    pic_gray_bin = parse_xpm_file(R_C_xpm_start);

    configure_gpio();
    dev = configure_spi(); 
    epd_init(dev);

    ESP_LOGI(TAG, "墨水屏灰阶示例程序");

    /* 清屏 */
    memset(epd_draw_buffer, 0xff, 128/8 * 250);
    epd_set_cursor(dev);
    epd_full_screen_write(dev, epd_draw_buffer);
    epd_display_update(dev);

    /* 分15次进行刷新,因为全白不需要刷新 */
    for (i = 0; i < 15; i++) {
        if (i <= 4)
            epd_gray_lut[16] = 0x3;
        if (i >=12)
            epd_gray_lut[16] = 0x1;
        else
            epd_gray_lut[16] = 0x2;

        epd_update_lut(dev, epd_gray_lut);

        memset(epd_draw_buffer, 0xff, 128/8*250);
        for (j = 0; j < EPD_HEIGHT; j++) {
            for (k = 0; k < EPD_WIDTH; k++) {
                if (pic_gray_bin[EPD_WIDTH * j + k] <= i) {
                    epd_draw_buffer[128/8 * j + k / 8] &= ~(1 << (7 - k%8));
                }
            }
        }

        epd_set_cursor(dev);
        epd_full_screen_write(dev, epd_draw_buffer);
        epd_display_update(dev);
    }
}
