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

    // Læs op til 31 bytes, så der er plads til null-terminering
    int bytesRead = read(file, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0'; // Sikrer null-terminering
        std::string uid(buffer);
        std::cout << "✅ UID modtaget fra Arduino: " << uid << std::endl;

        // Hvis du vil bruge tallet som integer:
        try {
            unsigned long uidNum = std::stoul(uid);
            std::cout << "UID som tal: " << uidNum << std::endl;
        } catch (...) {
            std::cerr << "❌ Kunne ikke konvertere UID til tal.\n";
        }

    } else {
        std::cerr << "❌ Fejl ved læsning fra Arduino.\n";
    }

    close(file);
    return 0;
}
