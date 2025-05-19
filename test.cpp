#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>

int main() {
    const char *device = "/dev/i2c-1";   // I2C bus (typisk i2c-1 på Pi)
    int addr = 0x08;                     // Arduino slaveadresse
    char buffer[32] = {0};               // Buffer til at modtage data

    // Åbn I2C-bus
    int file = open(device, O_RDWR);
    if (file < 0) {
        std::cerr << "Kan ikke åbne I2C enhed." << std::endl;
        return 1;
    }

    // Sæt slaveadresse
    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        std::cerr << "Kan ikke sætte slaveadresse." << std::endl;
        close(file);
        return 1;
    }

    // Læs op til 32 bytes fra Arduino
    int bytesRead = read(file, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        std::cerr << "Fejl ved læsning fra Arduino." << std::endl;
    } else {
        std::cout << "Modtaget fra Arduino: " << buffer << std::endl;
    }

    close(file);
    return 0;
}
