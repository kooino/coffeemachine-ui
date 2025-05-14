#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>

#define I2C_ADDR 0x24           // Arduino I2C slave adresse
#define I2C_DEV "/dev/i2c-1"    // I2C enhed på Raspberry Pi

int main() {
    int file = open(I2C_DEV, O_RDWR);
    if (file < 0) {
        perror("❌ Kunne ikke åbne I2C-enhed");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("❌ Kunne ikke sætte I2C-adresse");
        close(file);
        return 1;
    }

    char buffer[16] = {0};  // plads til UID som tekst
    ssize_t bytes = read(file, buffer, sizeof(buffer) - 1);

    if (bytes > 0) {
        buffer[bytes] = '\0';  // null-terminate
        std::cout << buffer << std::endl;  // kun UID vises
    } else {
        std::cerr << "❌ Ingen data modtaget. Prøv igen." << std::endl;
    }

    close(file);
    return 0;
}
