#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>
#include <chrono>
#include <thread>

int main() {
    const char *device = "/dev/i2c-1";   // I2C-bus
    int addr = 0x08;                     // Arduino-slaveadresse
    char buffer[32];                     // Buffer til data

    int file = open(device, O_RDWR);
    if (file < 0) {
        std::cerr << "Kan ikke åbne I2C-enhed." << std::endl;
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        std::cerr << "Kan ikke sætte slaveadresse." << std::endl;
        close(file);
        return 1;
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer)); // Nulstil buffer

        int bytesRead = read(file, buffer, sizeof(buffer));
        if (bytesRead > 0) {
            std::cout << "Modtaget fra Arduino: ";
            for (int i = 0; i < bytesRead; i++) {
                std::cout << buffer[i];
            }
            std::cout << std::endl;
        } else {
            std::cerr << "Fejl ved læsning fra Arduino." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    close(file);
    return 0;
}
