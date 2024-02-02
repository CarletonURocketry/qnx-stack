/**
 * @file main.c
 * @brief The main function for the fetcher module, where program logic is used to create a console application.
 *
 * The main function for the fetcher module, where program logic is used to create a console application.
 */
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

/** Name of i2c device descriptor to read from, if one is provided. */
static char *i2c_device = NULL;

int main(int argc, char **argv) {

    /* Check for positional arguments. */
    if (argc != 2) {
        fprintf(stderr, "The device descriptor of the I2C bus is required.\n");
        exit(EXIT_FAILURE);
    }
    i2c_device = argv[1];

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

    // Set up acknowledgement message header
    i2c_send_t ack_header = {.stop = 1, .len = 1, .slave = {.fmt = I2C_ADDRFMT_7BIT, .addr = 0}};
    uint8_t ack_command[sizeof(ack_header) + 1];
    ack_command[sizeof(ack_header)] = 0x0; // Ack message contents

    // Generate all possible I2C addresses
    for (uint8_t i = 0; i < 128; i++) {
        ack_header.slave.addr = i;                            // Current address
        memcpy(ack_command, &ack_header, sizeof(ack_header)); // Copy in header
        errno_t error = devctl(bus, DCMD_I2C_SEND, ack_command, sizeof(ack_command), NULL);
        if (error == EOK) {
            printf("0x%02x\n", i);
        }
    }

    return 0;
}
