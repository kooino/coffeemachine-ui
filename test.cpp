#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>

#define I2C_DEV "/dev/i2c-1"
#define PN532_I2C_ADDR 0x24

bool writeCommand(int file, const uint8_t* data, size_t len) {
    return write(file, data, len) == (ssize_t)len;
}

bool readAck(int file) {
    uint8_t ack[6];
    usleep(1000);
    if (read(file, ack, sizeof(ack)) != 6) return false;

    // PN532 ACK frame: 00 00 FF 00 FF 00
    return ack[0] == 0x00 && ack[1] == 0x00 && ack[2] == 0xFF &&
           ack[3] == 0x00 && ack[4] == 0xFF && ack[5] == 0x00;
}

int main() {
    int file = open(I2C_DEV, O_RDWR);
    if (file < 0) {
        perror("Kunne ikke åbne I2C");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, PN532_I2C_ADDR) < 0) {
        perror("Kunne ikke sætte I2C adresse");
        close(file);
        return 1;
    }

    // 1. Send SAMConfiguration kommando
    const uint8_t samConfig[] = {
        0x00,
        0x00, 0xFF, 0x05, 0xFB,
        0xD4, 0x14, 0x01, 0x14, 0x01,
        0x6A, 0x00
    };
    writeCommand(file, samConfig, sizeof(samConfig));
    usleep(1000);
    if (!readAck(file)) {
        std::cerr << "SAMConfig ACK mangler\n";
        close(file);
        return 1;
    }
    usleep(2000);
    read(file, samConfig, 32); // Tøm eventuelt svar

    // 2. Send InListPassiveTarget kommando
    const uint8_t inList[] = {
        0x00,
        0x00, 0xFF, 0x04, 0xFC,
        0xD4, 0x4A, 0x01, 0x00,
        0xE1, 0x00
    };
    writeCommand(file, inList, sizeof(inList));
    usleep(1000);
    if (!readAck(file)) {
        std::cerr << "InListPassiveTarget ACK mangler\n";
        close(file);
        return 1;
    }

    // 3. Læs UID-svar
    usleep(200000);
    uint8_t response[64];
    ssize_t len = read(file, response, sizeof(response));
    if (len <= 0) {
        std::cerr << "Intet svar fra PN532\n";
        close(file);
        return 1;
    }

    // 4. Find D5 4B og læs UID
    for (int i = 0; i < len - 8; ++i) {
        if (response[i] == 0xD5 && response[i + 1] == 0x4B) {
            uint8_t uidLen = response[i + 5];
            if (uidLen == 4) {
                uint32_t uid = 0;
                for (int j = 0; j < 4; ++j) {
                    uid <<= 8;
                    uid |= response[i + 6 + j];
                }
                std::cout << uid << std::endl;
                close(file);
                return 0;
            }
        }
    }

    std::cerr << "Intet UID fundet\n";
    close(file);
    return 1;
}
