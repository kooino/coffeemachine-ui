#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <vector>
#include <algorithm>
#include <mutex>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define PORT 5000
#define I2C_ADDR_MOTOR 0x30
#define I2C_ADDR_RFID  0x08

std::mutex logMutex;
std::string gemtValg;

bool checkKort(const std::string& uid) {
    if (uid == "3552077462") return false; // Forkert kort
    std::vector<std::string> godkendteUIDs = { "165267797", "123456789" };
    return std::find(godkendteUIDs.begin(), godkendteUIDs.end(), uid) != godkendteUIDs.end();
}

void skrivTilFil(const std::string& filnavn, const std::string& data) {
    int fd = open(filnavn.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, data.c_str(), data.size());
        close(fd);
    }
}

std::string læsFraFil(const std::string& filnavn) {
