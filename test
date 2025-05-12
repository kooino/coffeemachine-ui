
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>

#define I2C_DEVICE "/dev/i2c-1"
#define SLAVE_ADDR 0x08

int main() {
    int file;
    const char* message = "Hej fra RPi";

    if ((file = open(I2C_DEVICE, O_RDWR)) < 0) {
        perror("Fejl ved åbning af I2C-enhed");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, SLAVE_ADDR) < 0) {
        perror("Fejl ved opsætning af slave-adresse");
        close(file);
        return 1;
    }

    while (true) {
        if (write(file, message, strlen(message)) != (ssize_t)strlen(message)) {
            perror("Fejl ved skrivning til Arduino");
        } else {
            std::cout << "✅ Sendt: " << message << std::endl;
        }
        sleep(5);
    }

    close(file);
    return 0;
}
