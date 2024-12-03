#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <gif_lib.h>
#include <unistd.h> // For usleep

#define SPI_CS_PIN        RPI_V2_GPIO_P1_22  // Chip select pin
#define SPI_CLK_PIN       RPI_V2_GPIO_P1_23  // Clock pin
#define SPI_MISO_PIN      RPI_V2_GPIO_P1_21  // MISO pin (unused)
#define SPI_MOSI_PIN      RPI_V2_GPIO_P1_19  // MOSI pin

#define ST7789_WIDTH       240
#define ST7789_HEIGHT      240
#define ST7789_RESET_PIN   27  // Pin for RESET
#define ST7789_DC_PIN      24  // Pin for DC

// Function to initialize the SPI interface and display
void init_display() {
    if (!bcm2835_init()) {
        printf("BCM2835 initialization failed. Exiting...\n");
        exit(1);
    }

    bcm2835_gpio_fsel(SPI_CS_PIN, BCM2835_GPIO_FSEL_OUTP);    // CS pin
    bcm2835_gpio_fsel(SPI_CLK_PIN, BCM2835_GPIO_FSEL_ALT0);   // Clock
    bcm2835_gpio_fsel(SPI_MISO_PIN, BCM2835_GPIO_FSEL_INPT);  // MISO pin (not used)
    bcm2835_gpio_fsel(SPI_MOSI_PIN, BCM2835_GPIO_FSEL_ALT0);  // MOSI pin

    // Set up RESET and DC pins
    bcm2835_gpio_fsel(ST7789_RESET_PIN, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(ST7789_DC_PIN, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_spi_begin();
    bcm2835_spi_set_speed_hz(10000000); // 10 MHz speed
    bcm2835_spi_set_bit_order(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_set_data_mode(BCM2835_SPI_MODE0);
}

// Function to send a command to the display
void st7789_send_command(uint8_t cmd) {
    bcm2835_gpio_clr(ST7789_DC_PIN);  // Set DC to command mode
    bcm2835_spi_transfer(cmd);        // Send command
}

// Function to send data to the display
void st7789_send_data(uint8_t data) {
    bcm2835_gpio_set(ST7789_DC_PIN);  // Set DC to data mode
    bcm2835_spi_transfer(data);       // Send data
}

// Function to initialize the ST7789 display
void st7789_init() {
    bcm2835_gpio_set(ST7789_RESET_PIN);  // Reset high
    bcm2835_delay(200);
    bcm2835_gpio_clr(ST7789_RESET_PIN);  // Reset low
    bcm2835_delay(200);
    bcm2835_gpio_set(ST7789_RESET_PIN);  // Reset high

    // Send initialization commands (ST7789 initialization sequence)
    st7789_send_command(0x01);  // Software reset
    bcm2835_delay(150);
    st7789_send_command(0x11);  // Sleep out
    bcm2835_delay(255);
    st7789_send_command(0x3A);  // Interface Pixel Format
    st7789_send_data(0x55);     // 16-bit color
    st7789_send_command(0x36);  // Memory Data Access Control
    st7789_send_data(0x00);     // Set orientation
    st7789_send_command(0x29);  // Display on
}

// Function to convert a GIF color to RGB565
uint16_t gif_to_rgb565(int r, int g, int b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// Function to display a single GIF frame
void display_gif_frame(GifByteType *frame_data, int width, int height, GifFileType *gif_file) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int color_idx = frame_data[(y * width) + x];
            GifColorType color = gif_file->SColorMap->Colors[color_idx];

            // Convert to RGB565
            uint16_t color_565 = gif_to_rgb565(color.Red, color.Green, color.Blue);

            st7789_send_data(color_565 >> 8);  // Send high byte
            st7789_send_data(color_565 & 0xFF);  // Send low byte
        }
    }
}

// Function to load and display the frames of a GIF
void display_gif(const char *filename) {
    int error;
    GifFileType *gif_file = DGifOpenFileName(filename, &error);
    if (gif_file == NULL) {
        printf("Error opening GIF file.\n");
        return;
    }

    // Read the GIF header
    if (DGifSlurp(gif_file) == GIF_ERROR) {
        printf("Error decoding GIF file.\n");
        DGifCloseFile(gif_file, &error);
        return;
    }

    // Loop through all frames and display them
    for (int i = 0; i < gif_file->ImageCount; i++) {
        GifImageDesc *frame = &gif_file->Image[i];
        display_gif_frame(frame->RasterBits, frame->Width, frame->Height, gif_file);

        // Introduce a delay between frames for animation effect
        usleep(100000);  // Delay for 100ms between frames
    }

    // Clean up and close the GIF file
    DGifCloseFile(gif_file, &error);
}

int main() {
    init_display();
    st7789_init();
    display_gif("image.gif");  // Provide the path to your GIF file
    bcm2835_close();
    return 0;
}
