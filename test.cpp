#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int main() {
    const char* filename = "/dev/i2c-1";
    int addr = 0x08;
    char buf[32] = {0};

    int file = open(filename, O_RDWR);
    if (file < 0) {
        perror("Open");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        perror("Ioctl");
        return 1;
    }

    ssize_t bytesRead = read(file, buf, sizeof(buf)-1);
    if (bytesRead > 0) {
        buf[bytesRead] = '\0';
        std::cout << "Modtaget UID: " << buf << std::endl;
    } else {
        std::cerr << "Ingen data læst" << std::endl;
    }

    close(file);
    return 0;
}
