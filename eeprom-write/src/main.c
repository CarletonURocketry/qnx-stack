#include "../fetcher/src/drivers/m24c0x/m24c0x.h"
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** The speed of the I2C bus in kilobits per second. */
#define BUS_SPEED 400000

/** Name of I2C device descriptor to look for the EEPROM on. */
char *i2c_device = NULL;

/** The file path of the file containing the information to be written to the EEPROM. */
char *file_path = NULL;

/** Whether or not the EEPROM should be erased. */
bool erase = false;

/** Address of the EEPROM on the I2C bus. */
uint8_t eeprom_addr = 0x50;

/** Buffer to store the contents read from the EEPROM. */
uint8_t buf[M24C02_CAP + 1];

int main(int argc, char **argv) {

    /* Fetch command line arguments. */
    int c;
    while ((c = getopt(argc, argv, ":w:e")) != -1) {
        switch (c) {
        case 'e':
            erase = true;
            break;
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
    optind++;
    if (optind >= argc) {
        fprintf(stderr, "Address of the EEPROM is required.\n");
        exit(EXIT_FAILURE);
    }
    eeprom_addr = strtoul(argv[optind], NULL, 16);

    /* Open I2C. */
    int bus = open(i2c_device, O_RDWR);
    if (bus < 0) {
        fprintf(stderr, "Could not open I2C bus with error %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Set I2C bus speed. */
    uint32_t speed = BUS_SPEED;
    int err = devctl(bus, DCMD_I2C_SET_BUS_SPEED, &speed, sizeof(speed), NULL);
    if (err != EOK) {
        fprintf(stderr, "Failed to set bus speed to %u with error %s\n", speed, strerror(err));
        exit(EXIT_FAILURE);
    }

    // Construct the sensor location of the EEPROM
    SensorLocation loc = {
        .bus = bus,
        .addr = {.fmt = I2C_ADDRFMT_7BIT, .addr = eeprom_addr},
    };

    /* Erase the EEPROM. */
    if (erase) {
        m24c0x_erase(&loc, M24C02_CAP);
        if (file_path == NULL) return 0; // If we wanted to read, return early.
    }

    // Read mode
    if (file_path == NULL) {

        err = m24c0x_seq_read_rand(&loc, 0x00, buf, M24C02_CAP);
        if (err) {
            fprintf(stderr, "Could not read from EEPROM.\n");
            exit(EXIT_FAILURE);
        }

        buf[M24C02_CAP] = '\0'; // End buffer with null terminator to avoid overflow
        printf("%s", buf);

        return 0;
    }

    /* Try to open file. */
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file '%s'\n", file_path);
        exit(EXIT_FAILURE);
    }

    // Write the maximum amount of data to the EEPROM
    size_t nread = fread(&buf, sizeof(uint8_t), sizeof(buf), file);
    for (uint8_t addr = 0; addr < nread; addr++) {
        m24c0x_write_byte(&loc, addr, buf[addr]);
    }

    fclose(file);
    return 0;
}
