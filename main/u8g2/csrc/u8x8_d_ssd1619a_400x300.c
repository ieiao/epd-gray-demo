/*

  u8x8_d_HINK_E042A13_A0.c


  SSD1619A: 400x300 B/W
  command 
    0x22: assign actions
    0x20: execute actions

  Notes:
    - Introduced a refresh display message, which copies RAM to display
    - Charge pump and clock are only enabled for the transfer RAM to display
    - U8x8 will not really work because of the two buffers in the SSD1606, however U8g2 should be ok.

*/


#include "u8x8.h"
#include "esp_log.h"

/* HINK-E042A13-A0 4.2" B/W EPD */

static const uint8_t u8x8_d_ssd1619a_400x300_HINK_E042A13_A0_swrst_seq[] = {
    U8X8_START_TRANSFER(),
    U8X8_C(0x12),
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static const uint8_t u8x8_d_ssd1619a_400x300_HINK_E042A13_A0_init_seq[] = {
    U8X8_START_TRANSFER(),
    U8X8_CA(0x74, 0x54),
    U8X8_CA(0x7e, 0x3b),
    U8X8_CAA(0x2b, 0x04, 0x63),
    U8X8_CAAAA(0x0c, 0x8b, 0x9c, 0x96, 0x0f),
    U8X8_CAAA(0x01, 0x2b, 0x01, 0x00),
    U8X8_CA(0x11, 0x03),
    U8X8_CAA(0x44, 0x00, 0x31),
    U8X8_CAAAA(0x45, 0x00, 0x00, 0x2b, 0x01),
    U8X8_CA(0x3c, 0x01),
    U8X8_CA(0x18, 0x80),
    U8X8_CA(0x22, 0xb1),
    U8X8_C(0x20),
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static const uint8_t u8x8_d_ssd1619a_refresh_full_init_seq[] = {
    U8X8_START_TRANSFER(),
    U8X8_C(0x32),
    U8X8_A(0x66), U8X8_A(0x40), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x66), U8X8_A(0x80), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x66), U8X8_A(0x40), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x66), U8X8_A(0x80), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A4(0x0f, 0x0f, 0x0f, 0x0f), U8X8_A(0x00),
    U8X8_A4(0x1f, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static const uint8_t u8x8_d_ssd1619a_refresh_part_init_seq[] = {
    U8X8_START_TRANSFER(),
    U8X8_C(0x32),
    U8X8_A(0x50), U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0xa0), U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x50), U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0xa0), U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A(0x00), U8X8_A(0x00), U8X8_A(0x00),
    U8X8_A4(0x1f, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_A4(0x00, 0x00, 0x00, 0x00), U8X8_A(0x00),
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static const uint8_t u8x8_d_ssd1619a_powersave0_seq[] = {
    U8X8_START_TRANSFER(),
    U8X8_CA(0x22, 0xc0),
    U8X8_C(0x20),
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static const uint8_t u8x8_d_ssd1619a_powersave1_seq[] = {
    U8X8_START_TRANSFER(),
    U8X8_CA(0x22, 0x03),
    U8X8_C(0x20),
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static const uint8_t u8x8_d_ssd1619a_to_display_seq[] = {
    U8X8_START_TRANSFER(),
    U8X8_CA(0x22, 0x04),
    U8X8_C(0x20),
    U8X8_END_TRANSFER(),
    U8X8_END()
};

static void __set_window(u8x8_t *u8x8, uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye)
{
    u8x8_cad_SendCmd(u8x8, 0x44);
    u8x8_cad_SendArg(u8x8, xs);
    u8x8_cad_SendArg(u8x8, xe);

    u8x8_cad_SendCmd(u8x8, 0x45);
    u8x8_cad_SendArg(u8x8, ys & 0xff);
    u8x8_cad_SendArg(u8x8, (ys >> 8) & 0xff);
    u8x8_cad_SendArg(u8x8, ye & 0xff);
    u8x8_cad_SendArg(u8x8, (ye >> 8) & 0xff);

    u8x8_cad_SendCmd(u8x8, 0x4e);
    u8x8_cad_SendArg(u8x8, xs);

    u8x8_cad_SendCmd(u8x8, 0x4f);
    u8x8_cad_SendArg(u8x8, ys & 0xff);
    u8x8_cad_SendArg(u8x8, (ys >> 8) & 0xff);
}

static void u8x8_d_ssd1619a_draw_tile(u8x8_t *u8x8, uint8_t arg_int, void *arg_ptr)
{
    uint16_t xs, xe, ys, ye, c, i;
    uint8_t *ptr;

    u8x8_cad_StartTransfer(u8x8);

    ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
    c = ((u8x8_tile_t *)arg_ptr)->cnt;
    xs = ((u8x8_tile_t *)arg_ptr)->x_pos;
    xe = xs + (c * arg_int) - 1;
    ys = ((u8x8_tile_t *)arg_ptr)->y_pos * 8;
    ye = ys + 7;
    if (ye > 299)
        ye = 299;

    __set_window(u8x8, xs, xe, ys, ye);
    u8x8_cad_SendCmd(u8x8, 0x24);
    for (i = 0; i <= (ye&0x7); i++)
        u8x8_cad_SendData(u8x8, c,
                ptr + u8x8->display_info->tile_width * i);

    u8x8_cad_EndTransfer(u8x8);
}

static const u8x8_display_info_t u8x8_ssd1619a_400x300_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 120,
  /* pre_chip_disable_wait_ns = */ 60,
  /* reset_pulse_width_ms = */ 20,
  /* post_reset_wait_ms = */ 10,
  /* sda_setup_time_ns = */ 50,
  /* sck_pulse_width_ns = */ 100,
  /* sck_clock_hz = */ 20000000UL,
  /* spi_mode = */ 0,
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 40,
  /* write_pulse_width_ns = */ 150,
  /* tile_width = */ 50,	/* 50*8 = 400 */
  /* tile_hight = */ 38,	/* 38*8 = 304 */
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 400,
  /* pixel_height = */ 300
};

uint8_t u8x8_d_ssd1619a_400x300(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static uint8_t refresh_mode = 0;
    switch(msg)
    {
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
        u8x8_d_helper_display_setup_memory(u8x8, &u8x8_ssd1619a_400x300_display_info);
        break;

    case U8X8_MSG_DISPLAY_INIT:
        u8x8_d_helper_display_init(u8x8);
        u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1619a_400x300_HINK_E042A13_A0_swrst_seq);
        /* GPIO_E is used as BUSY pin. */
        while (u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_GPIO_E, 0, NULL) != 0)
            u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_MILLI, 10);
        u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1619a_400x300_HINK_E042A13_A0_init_seq);
        /* GPIO_E is used as BUSY pin. */
        while (u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_GPIO_E, 0, NULL) != 0)
            u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_MILLI, 10);
        u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1619a_refresh_full_init_seq);
        u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1619a_powersave0_seq);
        break;

    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
        if (arg_int == 0)
            u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1619a_powersave0_seq);
        else
            u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1619a_powersave1_seq);
        break;
    case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
        break;
    case U8X8_MSG_DISPLAY_DRAW_TILE:
        u8x8_d_ssd1619a_draw_tile(u8x8, arg_int, arg_ptr);
        break;
    case U8X8_MSG_DISPLAY_REFRESH:
        if (arg_int != refresh_mode) {
            if (arg_int)
                u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1619a_refresh_part_init_seq);
            else
                u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1619a_refresh_full_init_seq);
            refresh_mode = arg_int;
        }
        u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1619a_to_display_seq);
        /* GPIO_E is used as BUSY pin. */
        while (u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_GPIO_E, 0, NULL) != 0)
            u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_MILLI, 10);
        break;
    default:
        return 0;
    }

    return 1;
}

