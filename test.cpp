#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cctype>

// Hjælpefunktion til at trimme ikke-tal fra slutningen
std::string trimToDigits(const std::string& src) {
    size_t start = 0;
    while (start < src.size() && !isdigit(src[start])) start++;
    size_t end = start;
    while (end < src.size() && isdigit(src[end])) end++;
    return src.substr(start, end - start);
}

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
        std::string uid(buffer);
        std::cout << "✅ UID modtaget fra Arduino: " << uid << std::endl;

        // Trim UID til kun at indeholde tal
        std::string trimmed = trimToDigits(uid);

        try {
            unsigned long uidNum = std::stoul(trimmed);
            std::cout << "UID som tal: " << uidNum << std::endl;
        } catch (...) {
            std::cerr << "❌ Kunne ikke konvertere UID til tal. Trimmet streng var: '" << trimmed << "'\n";
        }

    } else {
        std::cerr << "❌ Fejl ved læsning fra Arduino.\n";
    }

    close(file);
    return 0;
}
