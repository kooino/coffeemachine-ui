#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

int main() {
    const char* device = "/dev/i2c-1"; // I2C bus på Pi
    int address = 0x08;                // Arduino I2C-adresse

    int file = open(device, O_RDWR);
    if (file < 0) {
        perror("❌ Kunne ikke åbne /dev/i2c-1");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, address) < 0) {
        perror("❌ Kunne ikke kontakte I2C enhed (0x08)");
        close(file);
        return 1;
    }

    char buffer[32] = {0};
    ssize_t bytesRead = read(file, buffer, sizeof(buffer) - 1);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0'; // Gør det til en C-streng
        std::cout << "✅ UID modtaget fra Arduino: " << buffer << std::endl;
    } else {
        std::cerr << "⚠️  Ingen data modtaget – måske ikke noget kort scannet?" << std::endl;
    }

    close(file);
    return 0;
}
