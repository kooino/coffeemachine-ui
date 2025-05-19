#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>

int main() {
    const char* device = "/dev/i2c-1";  // I2C bus på Raspberry Pi
    int address = 0x08;                 // Arduino I2C-slaveadresse

    // Åbn I2C-enheden
    int file = open(device, O_RDWR);
    if (file < 0) {
        std::cerr << "❌ Kan ikke åbne I2C-enhed: " << device << std::endl;
        return 1;
    }

    // Sæt slaveadressen
    if (ioctl(file, I2C_SLAVE, address) < 0) {
        std::cerr << "❌ Kan ikke sætte slaveadresse." << std::endl;
        close(file);
        return 1;
    }

    // Læs besked fra Arduino
    char buffer[32] = {0};
    int bytesRead = read(file, buffer, sizeof(buffer));

    if (bytesRead > 0) {
        std::cout << "✅ Modtaget fra Arduino: ";
        std::cout.write(buffer, bytesRead);
        std::cout << std::endl;
    } else {
        std::cerr << "❌ Fejl ved læsning fra Arduino." << std::endl;
    }

    close(file);
    return 0;
}
