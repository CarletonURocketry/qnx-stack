# eeprom-write

### Matteo Golin

Utility for writing and reading the board ID EEPROM of CU InSpace PCBs.

## Quick Start

You can use the command `use eeprom-write` to see the manual for the program.

### Reading

To read the EEPROM, run `eeprom-write /dev/i2c1` with the PCB connected to the I2C bus. The device descriptor for the
I2C bus can replace `/dev/i2c1` if it is different.

### Writing

To write to the board ID EEPROM, run `eeprom-write -w contents.ext /dev/i2c1` were `contents.ext` is some file
containing the data to be written to the EEPROM. This can be a text file, binary file, etc. Keep in mind the EEPROM's
limit of **128 bytes**.

To clear the EEPROM's contents before writing, you can use the `-ew` flag instead of the `-w` flag.
