#ifndef _inc_ssd1683
#define _inc_ssd1683

#include "pico/stdlib.h"

// RAM targets
#define BW_RAM 0x24
#define RED_RAM 0x26

// Update control values
#define FAST_FULL_UPDATE 0xd7
#define FULL_UPDATE 0xf7
#define PARTIAL_UPDATE 0xfc

/** @struct ssd_1683_display
 *  @brief structure representing a SSD1683-driven display
 */
typedef struct ssd_1683_display {
    spi_inst_t* spi; /** pointer to SPI instance */
    uint16_t width; /** width of display */
    uint16_t height; /** height of display */

    uint8_t reset; /** reset pin */
    uint8_t cs; /** CS pin */
    uint8_t dc; /** DC pin */
    uint8_t busy; /** busy pin */
} SSD1683;

/**
	@brief hard reset the display

	@param[in] display : display to target

    @return void
**/
void reset(SSD1683 display);

/**
	@brief trigger full update/refresh of the display

	@param[in] display : display to target
	@param[in] update_control : update type (fast full, partial, full, etc.)

    @return write status
    @retval 0 for success
**/
int update(SSD1683 display, uint8_t update_control);

/**
	@brief set target RAM area

	@param[in] display : display to target
	@param[in] width : width of window
	@param[in] height : height of window
	@param[in] x : x position of window
	@param[in] y : y position of window

    @return write status
    @retval 0 for success
**/
int set_ram_area(SSD1683 display, uint16_t width, uint16_t height, uint16_t x, uint16_t y);

/**
	@brief write data to display RAM

	@param[in] display : display to target
	@param[in] width : width of window
	@param[in] height : height of window
	@param[in] x : x position of window
	@param[in] y : y position of window
	@param[in] target : target RAM (black or red)
	@param[in] data : data to write

    @return write status
    @retval bytes written (ideally, width*height/8)
**/
uint32_t write_partial_ram(SSD1683 display, uint16_t width, uint16_t height, uint16_t x, uint16_t y, uint8_t target, uint8_t* data);

/**
	@brief write data to display RAM, targeting whole display

	@param[in] display : display to target
	@param[in] target : target RAM (black or red)
	@param[in] data : data to write

    @return write status
    @retval bytes written (ideally, width*height/8)
**/
uint32_t write_full_ram(SSD1683 display, uint8_t target, uint8_t* data);

/**
	@brief intialise display

	@param[in] display : display to target
	@param[in] width : width of display
	@param[in] height : height of display

    @return write status
    @retval 0 for success
**/
int init_display(SSD1683 display, uint16_t width, uint16_t height);

#endif