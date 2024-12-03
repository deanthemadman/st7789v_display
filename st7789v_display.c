#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include "stb_image.h"

// ST7789V Commands
#define ST7789_CMD_SWRESET 0x01
#define ST7789_CMD_SLPOUT  0x11
#define ST7789_CMD_DISPON  0x29
#define ST7789_CMD_COLMOD  0x3A
#define ST7789_CMD_MADCTL  0x36
#define ST7789_CMD_CASET   0x2A
#define ST7789_CMD_RASET   0x2B
#define ST7789_CMD_RAMWR   0x2C

// GPIO Pins
#define PIN_CS    RPI_GPIO_P1_24
#define PIN_RST   RPI_GPIO_P1_22
#define PIN_DC    RPI_GPIO_P1_18
#define PIN_SCK   RPI_GPIO_P1_23
#define PIN_SDI   RPI_GPIO_P1_19

void st7789_send_command(uint8_t cmd) {
    bcm2835_gpio_write(PIN_DC, LOW);  // DC low for command
    bcm2835_spi_transfer(cmd);
}

void st7789_send_data(uint8_t data) {
    bcm2835_gpio_write(PIN_DC, HIGH); // DC high for data
    bcm2835_spi_transfer(data);
}

void st7789_init() {
    bcm2835_gpio_write(PIN_RST, LOW);
    bcm2835_delay(100);
    bcm2835_gpio_write(PIN_RST, HIGH);
    bcm2835_delay(100);

    st7789_send_command(ST7789_CMD_SWRESET);
    bcm2835_delay(150);
    
    st7789_send_command(ST7789_CMD_SLPOUT);
    bcm2835_delay(255);

    st7789_send_command(ST7789_CMD_COLMOD);
    st7789_send_data(0x55); // 16-bit color

    st7789_send_command(ST7789_CMD_MADCTL);
    st7789_send_data(0x00); // Orientation

    st7789_send_command(ST7789_CMD_DISPON);
    bcm2835_delay(100);
}

void st7789_draw_image(uint8_t *image, int width, int height) {
    // Set column and row range
    st7789_send_command(ST7789_CMD_CASET);
    st7789_send_data(0x00);
    st7789_send_data(0x00);
    st7789_send_data(0x00);
    st7789_send_data((width - 1) & 0xFF);

    st7789_send_command(ST7789_CMD_RASET);
    st7789_send_data(0x00);
    st7789_send_data(0x00);
    st7789_send_data((height - 1) >> 8);
    st7789_send_data((height - 1) & 0xFF);

    st7789_send_command(ST7789_CMD_RAMWR);

    // Send pixel data
    for (int i = 0; i < width * height; i++) {
        uint8_t r = image[3 * i];
        uint8_t g = image[3 * i + 1];
        uint8_t b = image[3 * i + 2];
        uint16_t color = (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3);
        st7789_send_data(color >> 8);
        st7789_send_data(color & 0xFF);
    }
}

int main() {
    if (!bcm2835_init()) {
        printf("bcm2835 init failed. Are you running as root?\n");
        return 1;
    }

    bcm2835_spi_begin();
    bcm2835_spi_set_bit_order(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_set_data_mode(BCM2835_SPI_MODE0);
    bcm2835_spi_set_clock_divider(BCM2835_SPI_CLOCK_DIVIDER_65536); // Slow speed for safety
    bcm2835_gpio_fsel(PIN_CS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_RST, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_DC, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SCK, BCM2835_GPIO_FSEL_ALT0);
    bcm2835_gpio_fsel(PIN_SDI, BCM2835_GPIO_FSEL_ALT0);

    // Load the image (e.g., BMP, PNG, JPEG)
    int width, height, channels;
    uint8_t *image = stbi_load("image.bmp", &width, &height, &channels, 3);

    if (image == NULL) {
        printf("Failed to load image\n");
        return 1;
    }

    st7789_init();
    st7789_draw_image(image, width, height);

    // Cleanup
    stbi_image_free(image);
    bcm2835_spi_end();
    bcm2835_close();

    return 0;
}
