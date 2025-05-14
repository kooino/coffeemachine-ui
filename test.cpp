#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>

#define PN532_I2C_ADDR 0x24  // Eller brug 0x48, 0x24, 0x7B afhængigt af PN532-konfiguration
#define I2C_DEV "/dev/i2c-1"

bool sendCommand(int file, const uint8_t* cmd, size_t len) {
    if (write(file, cmd, len) != (ssize_t)len) {
        perror("Fejl ved skrivning til PN532");
        return false;
    }
    return true;
}

int main() {
    int file = open(I2C_DEV, O_RDWR);
    if (file < 0) {
        perror("Kunne ikke åbne I2C");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, PN532_I2C_ADDR) < 0) {
        perror("Kunne ikke kontakte PN532 på bus");
        close(file);
        return 1;
    }

    // Enkel kommando til at hente firmwareversion
    uint8_t getFirmwareVersion[] = {
        0x00,                   // I2C preamble
        0x00, 0xFF, 0x02, 0xFE, // Length, length checksum
        0xD4, 0x02,             // GetFirmware command
        0x2A,                   // Checksum
        0x00                    // Postamble
    };

    std::cout << "Sender GetFirmwareVersion...\n";

    if (!sendCommand(file, getFirmwareVersion, sizeof(getFirmwareVersion))) {
        close(file);
        return 1;
    }

    // Vent på svar
    usleep(100000);

    uint8_t buffer[32];
    ssize_t bytes = read(file, buffer, sizeof(buffer));

    if (bytes <= 0) {
        perror("Ingen svar fra PN532");
        close(file);
        return 1;
    }

    std::cout << "Svar modtaget (" << bytes << " bytes):\n";
    for (int i = 0; i < bytes; ++i) {
        printf("0x%02X ", buffer[i]);
    }
    std::cout << std::endl;

    close(file);
    return 0;
}
