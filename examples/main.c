#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "image.h"
#include "ssd1683.h"

#define EPD_BUSY 7
#define EPD_DC 10
#define EPD_CS 15
#define EPD_TX 19
#define EPD_SCK 18
#define EPD_RST 8

spi_inst_t *spi = spi0;

int main()
{
    stdio_init_all();
    sleep_ms(5000);
    spi_init(spi, 400000);
    gpio_set_function(EPD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(EPD_TX, GPIO_FUNC_SPI);

    gpio_init(EPD_CS);
    gpio_set_dir(EPD_CS, GPIO_OUT);

    gpio_init(EPD_DC);
    gpio_set_dir(EPD_DC, GPIO_OUT);

    gpio_init(EPD_RST);
    gpio_set_dir(EPD_RST, GPIO_OUT);

    gpio_init(EPD_BUSY);
    gpio_set_dir(EPD_BUSY, GPIO_IN);

    SSD1683 display = {.width = 400, .height = 300, .spi = spi, .reset = EPD_RST, .cs = EPD_CS, .dc = EPD_DC, .busy = EPD_BUSY};

    init_display(display, 400, 300);

    write_full_ram(display, BW_RAM, image);
    update(display, FAST_FULL_UPDATE);
    sleep_ms(5000);

    write_partial_ram(display, 400, 50, 0, 100, BW_RAM, image);
    update(display, PARTIAL_UPDATE);
}
