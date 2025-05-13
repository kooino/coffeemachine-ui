#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iomanip>
#include <chrono>
#include <thread>

#define I2C_DEV "/dev/i2c-1"
#define PN532_ADDR 0x24  // Tjek evt. din adresse med i2cdetect

void sendCommand(int fd, const uint8_t* data, size_t len) {
    uint8_t buffer[64];
    buffer[0] = 0x00;  // I2C framing for PN532
    memcpy(&buffer[1], data, len);
    write(fd, buffer, len + 1);
}

bool waitForAck(int fd) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    uint8_t ack[6];
    read(fd, ack, 6);
    return (ack[0] == 0x00 && ack[1] == 0x00 && ack[2] == 0xFF &&
            ack[3] == 0x00 && ack[4] == 0xFF && ack[5] == 0x00);
}

int main() {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        std::cerr << "❌ Kunne ikke åbne I2C-enhed." << std::endl;
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, PN532_ADDR) < 0) {
        std::cerr << "❌ Kunne ikke kontakte PN532 på I2C-adresse." << std::endl;
        close(fd);
        return 1;
    }

    std::cout << "✅ PN532 test - venter på kort..." << std::endl;

    // SAMConfiguration (aktiver læsning)
    uint8_t samCfg[] = { 0x00, 0x00, 0xFF, 0x05, 0xFB, 0xD4, 0x14, 0x01, 0x14, 0x01, 0x00 };
    sendCommand(fd, samCfg, sizeof(samCfg));
    waitForAck(fd);

    while (true) {
        // InListPassiveTarget (1 kort, 106 kbps, ISO14443A)
        uint8_t inList[] = { 0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x4A, 0x01, 0x00, 0xE1 };
        sendCommand(fd, inList, sizeof(inList));
        if (!waitForAck(fd)) {
            std::cerr << "❌ Ingen ACK fra PN532." << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        uint8_t response[32];
        int len = read(fd, response, sizeof(response));
        if (len > 0 && response[6] == 0xD5 && response[7] == 0x4B) {
            int uidLen = response[12];
            std::cout << "✅ Kort fundet! UID: ";
            for (int i = 0; i < uidLen; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(response[13 + i]);
            }
            std::cout << std::dec << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    close(fd);
    return 0;
}
