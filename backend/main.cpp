// BACKEND: main.cpp

#include <iostream>
#include <fstream>
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
#define I2C_ADDR 0x08

std::mutex logMutex;

bool checkKort(const std::string& uid) {
    if (uid == "3552077462") return false; // Blokeret UID
    std::vector<std::string> godkendteUIDs = { "165267797", "123456789" };
    return std::find(godkendteUIDs.begin(), godkendteUIDs.end(), uid) != godkendteUIDs.end();
}

void skrivTilFil(const std::string& filnavn, const std::string& data) {
    std::ofstream fil(filnavn);
    if (fil.is_open()) {
        fil << data;
        fil.close();
    }
}

std::string læsFraFil(const std::string& filnavn) {
    std::ifstream fil(filnavn);
    std::stringstream ss;
    if (fil.is_open()) {
        ss << fil.rdbuf();
        fil.close();
    }
    return ss.str();
}

void logBestilling(const std::string& valg) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream fil("bestillinger.txt", std::ios::app);
    if (fil.is_open()) {
        auto now = std::chrono::system_clock::now();
        std::time_t tid = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&tid);
        fil << "{ \"valg\": \"" << valg << "\", \"timestamp\": \""
            << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") << "\" },\n";
        fil.close();
    }
}

std::string hentBestillinger() {
    std::ifstream fil("bestillinger.txt");
    std::string linje, json = "[";
    bool først = true;
    while (std::getline(fil, linje)) {
        if (!først) json += ",";
        json += linje;
        først = false;
    }
    json += "]";
    return json;
}

std::string filtrerUID(const std::string& input) {
    std::string resultat;
    for (char c : input) {
        if (isdigit(c)) resultat += c;
    }
    return resultat;
}

void sendI2CCommand(const std::string& cmd) {
    const char* dev = "/dev/i2c-1";
    int file = open(dev, O_RDWR);
    if (file < 0 || ioctl(file, I2C_SLAVE, I2C_ADDR) < 0) return;
    write(file, cmd.c_str(), cmd.length());
    close(file);
    usleep(100000);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[30000] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        memset(buffer, 0, sizeof(buffer));
        read(new_socket, buffer, sizeof(buffer));
        std::string request(buffer);
        std::string responseBody;

        if (request.find("POST /gem-valg") != std::string::npos) {
            size_t bodyPos = request.find("\r\n\r\n");
            if (bodyPos != std::string::npos) {
                std::string valg = request.substr(bodyPos + 4);
                skrivTilFil("valg.txt", valg);
                if (valg == "Te") sendI2CCommand("mode:1");
                else if (valg == "Lille kaffe") sendI2CCommand("mode:2");
                else if (valg == "Stor kaffe") sendI2CCommand("mode:3");
                responseBody = "{\"status\":\"Valg gemt\"}";
            }
        } else if (request.find("GET /tjek-kort") != std::string::npos) {
            std::string uid = "";
            char raw[32] = {0};
            int file = open("/dev/i2c-1", O_RDWR);
            if (file >= 0 && ioctl(file, I2C_SLAVE, I2C_ADDR) >= 0) {
                int bytes = read(file, raw, sizeof(raw));
                if (bytes > 0) uid = filtrerUID(std::string(raw, bytes));
                close(file);
            }
            bool kortOK = checkKort(uid);
            if (kortOK) {
                skrivTilFil("kort.txt", "1");
                responseBody = "{\"kortOK\": true}";
            } else {
                skrivTilFil("kort.txt", "0");
                skrivTilFil("uid.txt", ""); // ryd UID ved ugyldig kort
                responseBody = "{\"kortOK\": false, \"error\": \"❌ Forkert kort\"}";
            }
        } else if (request.find("POST /bestil") != std::string::npos) {
            std::string kort = læsFraFil("kort.txt");
            std::string valg = læsFraFil("valg.txt");
            if (kort == "1" && !valg.empty()) {
                logBestilling(valg);
                sendI2CCommand("s");
                skrivTilFil("kort.txt", "0");
                skrivTilFil("valg.txt", "");
                responseBody = "{\"status\":\"OK\"}";
            } else {
                responseBody = "{\"error\":\"Ugyldig anmodning\"}";
            }
        } else if (request.find("POST /annuller") != std::string::npos) {
            skrivTilFil("kort.txt", "0");
            skrivTilFil("valg.txt", "");
            skrivTilFil("uid.txt", ""); // ryd UID ved annullering
            responseBody = "{\"status\":\"Annulleret\"}";
        } else if (request.find("GET /bestillinger") != std::string::npos) {
            responseBody = hentBestillinger();
        } else {
            responseBody = "{\"message\":\"Kaffeautomat API\"}";
        }

        std::string httpResponse =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: " + std::to_string(responseBody.size()) + "\r\n\r\n" +
            responseBody;

        send(new_socket, httpResponse.c_str(), httpResponse.size(), 0);
        close(new_socket);
    }
    return 0;
}
