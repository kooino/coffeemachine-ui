#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>

int main() {
    const char* device = "/dev/i2c-1";
    const int arduinoAddress = 0x08;
    char buffer[32] = {0};

    int file = open(device, O_RDWR);
    if (file < 0) {
        std::cerr << "❌ Kan ikke åbne I2C-enhed.\n";
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, arduinoAddress) < 0) {
        std::cerr << "❌ Kan ikke sætte I2C-slaveadresse.\n";
        close(file);
        return 1;
    }

    int bytesRead = read(file, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';

        // HEX DUMP
        std::cout << "Rådata (hex): ";
        for (int i = 0; i < bytesRead; i++) {
            printf("%02X ", (unsigned char)buffer[i]);
        }
        std::cout << std::endl;

        std::string uid(buffer);
        std::cout << "✅ UID modtaget fra Arduino som tekst: '" << uid << "'" << std::endl;
    } else {
        std::cerr << "❌ Fejl ved læsning fra Arduino.\n";
    }

    close(file);
    return 0;
}
