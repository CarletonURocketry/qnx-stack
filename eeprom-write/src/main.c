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

errno_t eeprom_read(uint8_t addr, int bus, void *buf, size_t n);
size_t eeprom_write(uint8_t addr, int bus, void const *buf, size_t n);

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

        uint8_t buf[100];
        if (eeprom_read(0, bus, buf, sizeof(buf)) != EOK) {
            fprintf(stderr, "Could not read from EEPROM.\n");
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < sizeof(buf); i++) {
            putchar(buf[i]);
        }

        return 0;
    }

    /* Try to open file. */
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file '%s'\n", file_path);
        exit(EXIT_FAILURE);
    }

    // Write all data to the EEPROM
    uint8_t buf[10];
    uint8_t addr = 0;
    while (!feof(file)) {
        size_t nread = fread(&buf, sizeof(uint8_t), sizeof(buf), file);
        eeprom_write(addr, bus, buf, nread);
        addr += nread;
    }

    fclose(file);
    return 0;
}

errno_t eeprom_read(uint8_t addr, int bus, void *buf, size_t n) {
    // Dummy write to start from address
    i2c_send_t dummy_header = {
        .stop = 0,
        .len = 1,
        .slave = {.fmt = I2C_ADDRFMT_7BIT, .addr = eewrite(EEPROM_ADDR)},
    };

    uint8_t dummy_write[sizeof(dummy_header) + 1];
    memcpy(dummy_write, &dummy_header, sizeof(dummy_header));
    dummy_write[sizeof(dummy_header)] = addr; // Address 0
    errno_t err = devctl(bus, DCMD_I2C_SEND, dummy_write, sizeof(dummy_write), NULL);
    if (err != EOK) return err;

    // Start sequential read
    i2c_sendrecv_t read_header = {
        .stop = 1,
        .send_len = 0,
        .recv_len = n,
        .slave = {.fmt = I2C_ADDRFMT_7BIT, .addr = eewrite(EEPROM_ADDR)},
    };
    uint8_t buffer[sizeof(read_header) + n];
    memcpy(buffer, &read_header, sizeof(read_header));
    err = devctl(bus, DCMD_I2C_SENDRECV, buffer, sizeof(buffer), NULL);
    if (err != EOK) return err;
    memcpy(buf, &buffer[sizeof(read_header)], n);
    return EOK;
}

size_t eeprom_write(uint8_t addr, int bus, void const *buf, size_t n) {

    // Set up the I2C packet for writing a single byte of data
    i2c_send_t header = {.stop = 1, .len = 2, .slave = {.fmt = I2C_ADDRFMT_7BIT, .addr = eewrite(EEPROM_ADDR)}};
    uint8_t write_command[sizeof(header) + 2]; // Save space for byte address and data
    memcpy(write_command, &header, sizeof(header));

    for (size_t i = 0; i < n; i++) {
        write_command[sizeof(header)] = addr;                            // Write to the selected address
        write_command[sizeof(header) + 1] = ((uint8_t const *)(buf))[i]; // Write data
        errno_t error = devctl(bus, DCMD_I2C_SEND, write_command, sizeof(write_command), NULL);
        if (error != EOK) return i;
        usleep(5000); // Some delay (5ms is the datasheet defined max)
        addr++;
    }
    return n;
}
