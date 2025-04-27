#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ssd1683.h"

uint8_t reverse(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void reset(SSD1683 display)
{
    gpio_put(display.reset, 0);
    sleep_ms(10);
    gpio_put(display.reset, 1);
    sleep_ms(10);
}

void wait_not_busy(SSD1683 display)
{
    sleep_ms(20);
    do
    {
        tight_loop_contents();
    } while (gpio_get(display.busy) != 0);
}

uint8_t write_data(SSD1683 display, uint8_t data)
{
    uint8_t buf[1];
    buf[0] = data;
    gpio_put(display.cs, 0);
    uint8_t bytes = spi_write_blocking(display.spi, buf, 1);
    gpio_put(display.cs, 1);
}

int write_command(SSD1683 display, int data)
{
    uint8_t buf[1];
    buf[0] = data;
    gpio_put(display.cs, 0);
    gpio_put(display.dc, 0);
    uint8_t bytes = spi_write_blocking(display.spi, buf, 1);
    gpio_put(display.cs, 1);
    gpio_put(display.dc, 1);
}

int update(SSD1683 display, uint8_t update_control)
{
    write_command(display, 0x21); // Display update control
    // 0x40 = bypass red RAM
    // 0x04 = bypass black RAM
    // 0x00 = both normal
    write_data(display, 0x40);
    write_data(display, 0x00);
    write_command(display, 0x1A); // Write to temp. sensor
    write_data(display, 0x6e);

    wait_not_busy(display);

    write_command(display, 0x22);
    write_data(display, update_control);
    write_command(display, 0x20); // Master active

    wait_not_busy(display);

    return 0;
}

int set_ram_area(SSD1683 display, uint16_t width, uint16_t height, uint16_t x, uint16_t y)
{
    write_command(display, 0x11); // Set RAM entry mode
    write_data(display, 0x03);    // X/Y increment
    write_command(display, 0x44); // Set RAM size
    write_data(display, x / 8);
    write_data(display, (x + width - 1) / 8);
    write_command(display, 0x45);
    write_data(display, y % 256);
    write_data(display, y / 256);
    write_data(display, (y + height - 1) % 256);
    write_data(display, (y + height - 1) / 256);
    write_command(display, 0x4e);
    write_data(display, x / 8);
    write_command(display, 0x4f);
    write_data(display, y % 256);
    write_data(display, y / 256);
    wait_not_busy(display);

    return 0;
}

uint32_t write_ram(SSD1683 display, uint16_t width, uint16_t height, uint16_t x, uint16_t y, uint8_t target, uint8_t *data)
{
    const uint16_t size = width * height / 8;
    set_ram_area(display, width, height, x, y);

    write_command(display, target);

    gpio_put(display.cs, 0);
    gpio_put(display.dc, 1);

    sleep_ms(10);
    printf("%d\n", size);

    for (int i = 0; i < size; i++)
    {
        data[i] = ~reverse(data[i]);
    }

    uint16_t written = spi_write_blocking(display.spi, data, size);

    gpio_put(display.cs, 1);

    return written;
}

uint32_t write_partial_ram(SSD1683 display, uint16_t width, uint16_t height, uint16_t x, uint16_t y, uint8_t target, uint8_t *data)
{
    return write_ram(display, width, height, x, y, target, data);
}

uint32_t write_full_ram(SSD1683 display, uint8_t target, uint8_t *data)
{
    return write_ram(display, display.width, display.height, 0, 0, target, data);
}

int init_display(SSD1683 display, uint16_t width, uint16_t height)
{
    reset(display);

    wait_not_busy(display);
    write_command(display, 0x12); // Reset
    wait_not_busy(display);

    write_command(display, 0x01); // Set gate driver output
    write_data(display, (height - 1) % 256);
    write_data(display, (height - 1) / 256);
    write_data(display, 0x00);

    write_command(display, 0x3C); // Set border
    write_data(display, 0x01);

    write_command(display, 0x18); // Read temperature
    write_data(display, 0x80);

    set_ram_area(display, display.width, display.height, 0, 0);

    wait_not_busy(display);

    return 0;
}