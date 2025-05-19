#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>

int main() {
    const char* device = "/dev/i2c-1";
    const int arduinoAddress = 0x08;
    const int maxAttempts = 5;
    const int readDelay = 100000; // 100ms i mikrosekunder

    int file = open(device, O_RDWR);
    if (file < 0) {
        std::cerr << "❌ Fejl ved åbning af I2C-enhed\n";
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, arduinoAddress) < 0) {
        std::cerr << "❌ Fejl ved indstilling af slaveadresse\n";
        close(file);
        return 1;
    }

    // Vent på at Arduino er klar
    usleep(200000);

    char buffer[32] = {0};
    int bytesRead = 0;
    int attempts = 0;

    // Læs med gentagelser
    while (attempts < maxAttempts && bytesRead <= 0) {
        bytesRead = read(file, buffer, sizeof(buffer)-1); // Reserver plads til null-terminering
        if (bytesRead > 0) break;
        usleep(readDelay);
        attempts++;
    }

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0'; // Null-terminer strengen
        std::string uid(buffer);
        
        // Vis resultater
        std::cout << "✅ Modtaget UID (" << bytesRead << " bytes):\n";
        std::cout << "Tekst: " << uid << "\n";
        std::cout << "Hex: ";
        for(int i = 0; i < bytesRead; i++) {
            printf("%02X ", static_cast<unsigned char>(buffer[i]));
        }
        std::cout << "\n";
    } else {
        std::cerr << "❌ Ingen data modtaget efter " << maxAttempts << " forsøg\n";
    }

    close(file);
    return 0;
}
