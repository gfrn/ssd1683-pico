#include <stdio.h>

#include "pico/stdlib.h"
#include "font.h"
#include "hardware/spi.h"
#include "ssd1683.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

unsigned char reverse(unsigned char b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void reset(SSD1683 display)
{
    gpio_put(display.reset, 0);
    sleep_ms(40);
    gpio_put(display.reset, 1);
    sleep_ms(40);
}

void wait_not_busy(SSD1683 display)
{
    sleep_ms(20);
    do
    {
        tight_loop_contents();
    } while (gpio_get(display.busy) != 0);
}

int write_data(SSD1683 display, int data)
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
    printf("%d\n", written);

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

int overlay_image(SSD1683 display, const uint8_t *image, uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
    uint16_t end_offset = width % 8;
    uint16_t start_offset = 0;
    uint16_t actual_pos = 0;

    for (uint16_t i = 0; i < height; i++) {
        for (uint16_t j = 0; j < width/8; j++) {
            const uint16_t orig_pos = x/8 + j + display.width/8 * (i+y);
            display.framebuffer[orig_pos] = image[i*(width/8) + j] | display.framebuffer[orig_pos];
        }
    }

    return 0;
}

int rotate_and_overlay_image(SSD1683 display, const uint8_t *image, uint16_t width, uint16_t height, uint16_t x, uint16_t y, int8_t angle) {
    uint8_t* rotated_image = calloc(width/8*height, sizeof(uint8_t));

    double sinma = sin(-angle);
    double cosma = cos(-angle);

    int hwidth = width / 2;
    int hheight = height / 2;

    for(int xi = 0; xi < width; xi++) {
        int xt = xi - hwidth;
        for(int yi = 0; yi < height; yi++) {
            uint8_t color = image[xi/8 + yi*(width/8)] & 0x01 << xi%8;
            if (!color) {
                continue;
            }
            int yt = yi - hheight;

            int xs = (int)round((cosma * xt - sinma * yt) + hwidth);
            int ys = (int)round((sinma * xt + cosma * yt) + hheight);

            if(xs >= 0 && xs < width && ys >= 0 && ys < height) {
                rotated_image[xs/8 + ys*(width/8)] |= 0x01 << xs%8;
            }
        }
    }

    const int result = overlay_image(display, rotated_image, width, height, x, y);
    free(rotated_image);

    return result;
}

int overlay_text(SSD1683 display, const char *text, size_t text_size, uint16_t x, uint16_t y) {
    uint8_t* text_image = malloc(text_size*sizeof(char)*8);

    for (size_t i = 0; i < text_size; i++) {
        for(int j = 0; j < 8; j++) {
            text_image[i + j*text_size] = font[text[i]-0x20][j];
        }
    }

    const int result = overlay_image(display, text_image, text_size*8, 8, x, y);

    free(text_image);

    return result;
}
