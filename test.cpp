#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define PN532_I2C_ADDR 0x24
#define I2C_DEV "/dev/i2c-1"

bool sendCommand(int file, const uint8_t* cmd, size_t len) {
    return write(file, cmd, len) == (ssize_t)len;
}

int main() {
    int file = open(I2C_DEV, O_RDWR);
    if (file < 0) {
        perror("❌ Kunne ikke åbne I2C");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, PN532_I2C_ADDR) < 0) {
        perror("❌ Kunne ikke sætte I2C-adresse");
        close(file);
        return 1;
    }

    // Send kommando: InListPassiveTarget (scan kort)
    uint8_t cmd[] = {
        0x00,
        0x00, 0xFF, 0x04, 0xFC,
        0xD4, 0x4A, 0x01, 0x00,
        0xE1, 0x00
    };

    for (int attempt = 0; attempt < 10; ++attempt) {
        sendCommand(file, cmd, sizeof(cmd));
        usleep(150000);  // Vent på svar

        uint8_t buffer[64];
        ssize_t bytes = read(file, buffer, sizeof(buffer));
        if (bytes <= 0) continue;

        // Find PN532 svar: D5 4B
        for (int i = 0; i < bytes - 6; ++i) {
            if (buffer[i] == 0xD5 && buffer[i + 1] == 0x4B) {
                uint8_t uidLength = buffer[i + 5];
                if (uidLength == 4) {
                    uint32_t uid = 0;
                    for (int j = 0; j < 4; ++j) {
                        uid <<= 8;
                        uid |= buffer[i + 6 + j];
                    }
                    std::cout << uid << std::endl;
                    close(file);
                    return 0;
                }
            }
        }

        usleep(200000);
    }

    std::cerr << "❌ Intet kort læst." << std::endl;
    close(file);
    return 1;
}
