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

/** The address of the EEPROM on the I2C bus. */
#define EEPROM_ADDR 0x50

/** Mask for reading the EEPROM. */
#define eeread(addr) (addr | 0x1)

/** Mask for writing to the EEPROM. */
#define eewrite(addr) (addr & 0xFE)

/** Name of I2C device descriptor to look for the EEPROM on. */
static char *i2c_device = NULL;
/** The file path of the file containing the information to be written to the EEPROM. */
static char *file_path = NULL;

int main(int argc, char **argv) {

    /* Fetch command line arguments. */
    int c;
    while ((c = getopt(argc, argv, ":w:")) != -1) {
        switch (c) {
        case 'w':
            file_path = optarg;
            break;
        case ':':
            fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unkown option -%c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fputs("Please see the 'use' page for usage.", stderr);
            return EXIT_FAILURE;
        }
    }

    /* Check for positional arguments. */
    if (optind >= argc) {
        fprintf(stderr, "I2C bus device descriptor is required.\n");
        exit(EXIT_FAILURE);
    }
    i2c_device = argv[optind];

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

    // Read mode
    if (file_path == NULL) {
        printf("READ MODE\n");
        return 0;
    }

    /* Try to open file. */
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file '%s'\n", file_path);
        exit(EXIT_FAILURE);
    }

    // Set up the I2C packet for writing a single byte of data
    i2c_send_t header = {.stop = 1, .len = 2, .slave = {.fmt = I2C_ADDRFMT_7BIT, .addr = eewrite(EEPROM_ADDR)}};
    uint8_t write_command[sizeof(header) + 2]; // Save space for byte address and data byte
    memcpy(write_command, &header, sizeof(header));
    uint8_t data;
    uint8_t addr = 0;

    // Write all data to the EEPROM
    while (!feof(file)) {
        fread(&data, sizeof(uint8_t), 1, file);
        write_command[sizeof(header)] = addr;     // Write to the next address
        write_command[sizeof(header) + 1] = data; // Write the next byte of data
        errno_t error = devctl(bus, DCMD_I2C_SEND, write_command, sizeof(write_command), NULL);
        if (error != EOK) {
            fprintf(stderr, "Error writing byte %02x: %s\n", data, strerror(error));
            fclose(file);
            exit(EXIT_FAILURE);
        } else {
            printf("Byte %02x written.\n", data);
        }
        addr++;
    }

    fclose(file);
    return 0;
}
