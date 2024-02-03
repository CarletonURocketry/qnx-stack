#include <devctl.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <hw/i2c.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** The speed of the I2C bus in kilobits per second. */
#define BUS_SPEED 400000

/** Name of I2C device descriptor to look for the EEPROM on. */
static char *i2c_device = NULL;
/** The address of the EEPROM on the I2C bus. */
static uint8_t eeprom_addr = 0;
/** The file path of the file containing the information to be written to the EEPROM. */
static char *file_path = NULL;

int main(int argc, char **argv) {

    /* Check for positional arguments. */
    if (argc != 4) {
        if (argc < 2) fprintf(stderr, "The device descriptor of the I2C bus is required.\n");
        if (argc < 3) fprintf(stderr, "The address of the EEPROM is required.\n");
        if (argc < 4) fprintf(stderr, "The path to the file containing the write data is required.\n");
        exit(EXIT_FAILURE);
    }
    i2c_device = argv[1];
    eeprom_addr = strtoul(argv[2], NULL, 16); // Parse address as unsigned hex
    file_path = argv[3];

    /* Try to open file. */
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file '%s'\n", file_path);
        exit(EXIT_FAILURE);
    }

    /* Check that EEPROM address is within 7 bit address space. */
    if (eeprom_addr >= 128) {
        fprintf(stderr, "EEPROM address of 0x%02x is not within the 7 bit I2C addressing space.\n", eeprom_addr);
        exit(EXIT_FAILURE);
    }

    /* Open I2C. */
    int bus = open(i2c_device, O_RDWR);
    if (bus < 0) {
        fprintf(stderr, "Could not open I2C bus with error %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Set I2C bus speed. */
    uint32_t speed = BUS_SPEED;
    errno_t bus_speed = devctl(bus, DCMD_I2C_SET_BUS_SPEED, &speed, sizeof(speed), NULL);
    if (bus_speed != EOK) {
        fprintf(stderr, "Failed to set bus speed to %u with error %s\n", speed, strerror(bus_speed));
        exit(EXIT_FAILURE);
    }

    // Set up the I2C packet for writing a single byte of data
    i2c_send_t header = {.stop = 1, .len = 1, .slave = {.fmt = I2C_ADDRFMT_7BIT, .addr = eeprom_addr}};
    uint8_t write_command[sizeof(header) + 1];
    uint8_t data;

    // Write all data to the EEPROM
    while (!feof(file)) {
        fread(&data, sizeof(uint8_t), 1, file);
        write_command[sizeof(header)] = data; // Write the next byte of data
    }

    return 0;
}
