#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>

#define PN532_I2C_ADDR 0x24
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

    // ReadPassiveTarget command: læs MIFARE kort
    uint8_t readCardCommand[] = {
        0x00,                   // Preamble
        0x00, 0xFF, 0x04, 0xFC, // Length, checksum
        0xD4, 0x4A, 0x01, 0x00, // InListPassiveTarget
        0xE1,                   // Data checksum
        0x00                    // Postamble
    };

    std::cout << "Venter på NFC-kort...\n";

    while (true) {
        if (!sendCommand(file, readCardCommand, sizeof(readCardCommand))) {
            close(file);
            return 1;
        }

        usleep(100000); // Vent på svar

        uint8_t buffer[32];
        ssize_t bytes = read(file, buffer, sizeof(buffer));

        if (bytes <= 0) {
            std::cerr << "Ingen svar fra PN532\n";
            continue;
        }

        // Kig efter UID i svaret
        for (int i = 0; i < bytes - 6; ++i) {
            if (buffer[i] == 0xD5 && buffer[i + 1] == 0x4B) {
                uint8_t uidLength = buffer[i + 5];
                std::cout << "Kort læst. UID: ";
                for (int j = 0; j < uidLength; ++j) {
                    printf("%02X ", buffer[i + 6 + j]);
                }
                std::cout << std::endl;
                break;
            }
        }

        usleep(500000); // Søg hvert 0.5 sekund
    }

    close(file);
    return 0;
}
