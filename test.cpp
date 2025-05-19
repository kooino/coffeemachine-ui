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
        buffer[bytesRead] = '\0'; // Sikrer null-terminering
        std::string uid(buffer);

        // Fjerner eventuelle ikke-udskrivbare tegn fra starten/slutningen
        uid.erase(0, uid.find_first_of("0123456789"));
        uid.erase(uid.find_last_of("0123456789") + 1);

        std::cout << "✅ UID modtaget fra Arduino som tekst: " << uid << std::endl;
    } else {
        std::cerr << "❌ Fejl ved læsning fra Arduino.\n";
    }

    close(file);
    return 0;
}
