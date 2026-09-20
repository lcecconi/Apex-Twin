#include "ST7305_U8g2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include <cstring>
#include <cstdlib>

#define SPI_CLK 24000000
#define ST7305_TILE_WIDTH 38
#define ST7305_TILE_HEIGHT 50
#define ST7305_BUFFER_ROW_BYTES (ST7305_TILE_WIDTH * 8)

static ST7305_U8g2 *g_lcd_instance = nullptr;

static u8x8_display_info_t st7305_display_info = {
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  /* post_chip_enable_wait_ns = */ 0,
  /* pre_chip_disable_wait_ns = */ 0,
  /* reset_pulse_width_ms = */ 20,
  /* post_reset_wait_ms = */ 50,
  /* sda_setup_time_ns = */ 0,
  /* sck_pulse_width_ns = */ 0,
  /* sck_clock_hz = */ SPI_CLK,
  /* spi_mode = */ 0,
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 0,
  /* write_pulse_width_ns = */ 0,
  /* tile_width = */ ST7305_TILE_WIDTH,
  /* tile_height = */ ST7305_TILE_HEIGHT,
  /* default_x_offset = */ 0,
  /* flip_mode_x_offset = */ 0,
  /* pixel_width = */ 300,
  /* pixel_height = */ 400
};

ST7305_U8g2::ST7305_U8g2(int sck, int mosi, int dc, int cs, int rst)
  : _sck(sck), _mosi(mosi), _dc(dc), _cs(cs), _rst(rst)
{
  g_lcd_instance = this;
}

ST7305_U8g2::~ST7305_U8g2()
{
  if (_spi_dev) {
    spi_bus_remove_device(_spi_dev);
    spi_bus_free(SPI2_HOST);
    _spi_dev = nullptr;
  }
  if (_my_buf) {
    free(_my_buf);
  }
  if (g_lcd_instance == this) {
    g_lcd_instance = nullptr;
  }
}

void ST7305_U8g2::_cmd(uint8_t cmd)
{
  gpio_set_level((gpio_num_t)_dc, 0);
  gpio_set_level((gpio_num_t)_cs, 0);
  
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 8;
  t.tx_buffer = &cmd;
  spi_device_polling_transmit(_spi_dev, &t);

  gpio_set_level((gpio_num_t)_cs, 1);
}

void ST7305_U8g2::_data(const uint8_t *data, size_t len)
{
  if (len == 0) return;
  gpio_set_level((gpio_num_t)_dc, 1);
  gpio_set_level((gpio_num_t)_cs, 0);

  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = len * 8;
  t.tx_buffer = data;
  spi_device_polling_transmit(_spi_dev, &t);

  gpio_set_level((gpio_num_t)_cs, 1);
}

void ST7305_U8g2::_cmd_data(uint8_t cmd, const uint8_t *data, size_t len)
{
  _cmd(cmd);
  if (len > 0) {
    _data(data, len);
  }
}

void ST7305_U8g2::reset()
{
  gpio_set_level((gpio_num_t)_rst, 1);
  vTaskDelay(pdMS_TO_TICKS(50));
  gpio_set_level((gpio_num_t)_rst, 0);
  vTaskDelay(pdMS_TO_TICKS(20));
  gpio_set_level((gpio_num_t)_rst, 1);
  vTaskDelay(pdMS_TO_TICKS(50));
}

uint8_t ST7305_U8g2::u8x8_byte_custom(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  (void)u8x8;
  (void)arg_int;
  (void)arg_ptr;

  switch (msg) {
    case U8X8_MSG_BYTE_INIT:
    case U8X8_MSG_BYTE_START_TRANSFER:
    case U8X8_MSG_BYTE_END_TRANSFER:
    case U8X8_MSG_BYTE_SET_DC:
    case U8X8_MSG_BYTE_SEND:
    default:
      return 1;
  }
}

uint8_t ST7305_U8g2::u8x8_d_st7305_custom(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  (void)arg_int;

  if (!g_lcd_instance) {
    return 0;
  }

  switch (msg) {
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &st7305_display_info);
      break;

    case U8X8_MSG_DISPLAY_INIT:
      g_lcd_instance->fullInit();
      break;

    case U8X8_MSG_DISPLAY_DRAW_TILE: {
      u8x8_tile_t *tile = (u8x8_tile_t *)arg_ptr;
      uint8_t cnt = tile->cnt;
      uint8_t y_pos = tile->y_pos;
      uint8_t x_pos = tile->x_pos;

      int first_col = x_pos * 8;
      int last_col = (x_pos + cnt) * 8 - 1;
      if (last_col >= 300) {
        last_col = 299;
      }

      int addr_start = 0x12 + first_col / 12;
      int addr_end = 0x12 + last_col / 12;
      int send_start = (addr_start - 0x12) * 3;
      int send_cnt = (addr_end - addr_start + 1) * 3;

      int addr_first_col = (addr_start - 0x12) * 12;
      int addr_last_col = (addr_end - 0x12) * 12 + 11;
      if (addr_last_col >= 300) {
        addr_last_col = 299;
      }

      uint8_t *row_base = tile->tile_ptr - ((uint16_t)x_pos * 8U);

      uint8_t col_bounds[] = {(uint8_t)(0x3C - addr_end), (uint8_t)(0x3C - addr_start)};
      g_lcd_instance->_cmd_data(0x2A, col_bounds, sizeof(col_bounds));

      uint8_t row_bounds[] = {(uint8_t)(y_pos * 4), (uint8_t)(y_pos * 4 + 3)};
      g_lcd_instance->_cmd_data(0x2B, row_bounds, sizeof(row_bounds));

      static const uint8_t st_lut[4][4] = {
        {0x00, 0x80, 0x40, 0xC0},
        {0x00, 0x20, 0x10, 0x30},
        {0x00, 0x08, 0x04, 0x0C},
        {0x00, 0x02, 0x01, 0x03},
      };

      uint8_t all_rows[300] = {0};
      for (int sr = 0; sr < 4; sr++) {
        int shift = sr * 2;
        int base_off = sr * send_cnt;
        int idx = base_off + (addr_first_col >> 2) - send_start;

        for (int col = addr_first_col; col <= addr_last_col; col += 4, idx++) {
          all_rows[idx] = st_lut[0][(row_base[col] >> shift) & 3]
                        | st_lut[1][(row_base[col + 1] >> shift) & 3]
                        | st_lut[2][(row_base[col + 2] >> shift) & 3]
                        | st_lut[3][(row_base[col + 3] >> shift) & 3];
        }
      }

      g_lcd_instance->_cmd_data(0x2C, all_rows, (size_t)send_cnt * 4U);
      break;
    }

    default:
      return 0;
  }

  return 1;
}

void ST7305_U8g2::begin(uint8_t tile_buf_height, const u8g2_cb_t *rotation)
{
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << _dc) | (1ULL << _cs) | (1ULL << _rst);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  gpio_set_level((gpio_num_t)_cs, 1);
  gpio_set_level((gpio_num_t)_dc, 1);
  gpio_set_level((gpio_num_t)_rst, 1);

  spi_bus_config_t buscfg = {};
  buscfg.mosi_io_num = _mosi;
  buscfg.miso_io_num = -1;
  buscfg.sclk_io_num = _sck;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;
  buscfg.max_transfer_sz = 4096;

  spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

  spi_device_interface_config_t devcfg = {};
  devcfg.clock_speed_hz = SPI_CLK;
  devcfg.mode = 0;
  devcfg.spics_io_num = -1;
  devcfg.queue_size = 7;

  spi_bus_add_device(SPI2_HOST, &devcfg, &_spi_dev);

  u8g2_t *u = u8g2_wrapper.getU8g2();
  u8x8_Setup(u8g2_GetU8x8(u), u8x8_d_st7305_custom, u8x8_dummy_cb, u8x8_byte_custom, u8x8_dummy_cb);

  uint8_t tbh = tile_buf_height == 0 ? ST7305_TILE_HEIGHT : tile_buf_height;
  if (tbh > ST7305_TILE_HEIGHT) {
    tbh = ST7305_TILE_HEIGHT;
  }

  size_t buf_sz = ST7305_BUFFER_ROW_BYTES * tbh;
  _my_buf = (uint8_t *)malloc(buf_sz);
  if (!_my_buf) {
    return;
  }
  memset(_my_buf, 0, buf_sz);

  u8g2_SetupBuffer(u, _my_buf, tbh, u8g2_ll_hvline_vertical_top_lsb, rotation);
  u8g2_InitDisplay(u);
  u8g2_SetPowerSave(u, 0);
}

void ST7305_U8g2::fullInit()
{
  reset();

  const uint8_t d6[] = {0x17, 0x02};
  const uint8_t d1[] = {0x01};
  const uint8_t c0[] = {0x11, 0x04};
  const uint8_t c1[] = {0x69, 0x69, 0x69, 0x69};
  const uint8_t c2[] = {0x19, 0x19, 0x19, 0x19};
  const uint8_t c4[] = {0x4B, 0x4B, 0x4B, 0x4B};
  const uint8_t d8[] = {0x80, 0xE9};
  const uint8_t b2[] = {0x02};
  const uint8_t b3[] = {0xE5, 0xF6, 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
  const uint8_t b4[] = {0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
  const uint8_t g_timing[] = {0x32, 0x03, 0x1F};
  const uint8_t b7[] = {0x13};
  const uint8_t b0[] = {0x64};
  const uint8_t c9[] = {0x00};
  const uint8_t m36[] = {0x48};
  const uint8_t m3a[] = {0x11};
  const uint8_t b9[] = {0x20};
  const uint8_t b8[] = {0x29};
  const uint8_t win_a[] = {0x12, 0x2A};
  const uint8_t win_b[] = {0x00, 0xC7};
  const uint8_t m35[] = {0x00};
  const uint8_t d0[] = {0xFF};

  _cmd_data(0xD6, d6, sizeof(d6));
  _cmd_data(0xD1, d1, sizeof(d1));
  _cmd_data(0xC0, c0, sizeof(c0));
  _cmd_data(0xC1, c1, sizeof(c1));
  _cmd_data(0xC2, c2, sizeof(c2));
  _cmd_data(0xC4, c4, sizeof(c4));
  _cmd_data(0xC5, c2, sizeof(c2));
  _cmd_data(0xD8, d8, sizeof(d8));
  _cmd_data(0xB2, b2, sizeof(b2));
  _cmd_data(0xB3, b3, sizeof(b3));
  _cmd_data(0xB4, b4, sizeof(b4));
  _cmd_data(0x62, g_timing, sizeof(g_timing));
  _cmd_data(0xB7, b7, sizeof(b7));
  _cmd_data(0xB0, b0, sizeof(b0));
  _cmd(0x11);
  vTaskDelay(pdMS_TO_TICKS(120));
  _cmd_data(0xC9, c9, sizeof(c9));
  _cmd_data(0x36, m36, sizeof(m36));
  _cmd_data(0x3A, m3a, sizeof(m3a));
  _cmd_data(0xB9, b9, sizeof(b9));
  _cmd_data(0xB8, b8, sizeof(b8));
  _cmd(0x20); // 0x20 = INVOFF (High-contrast Black on Silver)
  _cmd_data(0x2A, win_a, sizeof(win_a));
  _cmd_data(0x2B, win_b, sizeof(win_b));
  _cmd_data(0x35, m35, sizeof(m35));
  _cmd_data(0xD0, d0, sizeof(d0));
  _cmd(0x38);
  _cmd(0x29);
}

void ST7305_U8g2::setInvert(bool invert)
{
  _cmd(invert ? 0x20 : 0x21);
}
