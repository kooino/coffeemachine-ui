#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

int main() {
    const char* device = "/dev/i2c-1";
    int address = 0x08;  // Arduino-adresse

    int file = open(device, O_RDWR);
    if (file < 0) {
        perror("Open I2C");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, address) < 0) {
        perror("Set I2C Address");
        close(file);
        return 1;
    }

    char buffer[32] = {0};
    ssize_t bytesRead = read(file, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::cout << "Modtaget UID: " << buffer << std::endl;
    } else {
        std::cerr << "Kunne ikke læse fra Arduino" << std::endl;
    }

    close(file);
    return 0;
}
