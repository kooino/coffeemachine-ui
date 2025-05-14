#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>

#define ARDUINO_I2C_ADDR 0x24
#define I2C_DEV "/dev/i2c-1"

int main() {
    int file = open(I2C_DEV, O_RDWR);
    if (file < 0) {
        perror("Kunne ikke åbne I2C");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, ARDUINO_I2C_ADDR) < 0) {
        perror("Kunne ikke sætte I2C adresse");
        close(file);
        return 1;
    }

    char buffer[16] = {0};
    ssize_t bytes = read(file, buffer, sizeof(buffer) - 1);

    if (bytes > 0) {
        buffer[bytes] = '\0';  // sikrer korrekt afslutning
        std::cout << buffer << std::endl;  // viser UID som tekst
    } else {
        std::cerr << "Ingen data modtaget fra Arduino." << std::endl;
    }

    close(file);
    return 0;
}
