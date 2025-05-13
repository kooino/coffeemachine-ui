#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
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
#define I2C_ADDR 0x24

std::mutex filMutex;
std::string valgFil = "valg.txt";
std::string kortFil = "kort.txt";

std::string hentUIDFraArduino() {
    const char* filename = "/dev/i2c-1";
    int file = open(filename, O_RDWR);
    if (file < 0) return "";

    if (ioctl(file, I2C_SLAVE, I2C_ADDR) < 0) {
        close(file);
        return "";
    }

    char buffer[32] = {0};
    ssize_t bytesRead = read(file, buffer, sizeof(buffer) - 1);
    close(file);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string uid(buffer);
        uid.erase(uid.find_last_not_of(" \n\r\t") + 1); // Trim trailing
        std::cout << "UID fra Arduino: [" << uid << "]" << std::endl;
        return uid;
    }
    return "";
}

bool checkGodkendtUID(const std::string& uid) {
    return uid == "165267797";
}

void skrivTilFil(const std::string& filnavn, const std::string& data) {
    std::lock_guard<std::mutex> lock(filMutex);
    std::ofstream fil(filnavn);
    if (fil.is_open()) {
        fil << data;
        fil.close();
    }
}

std::string læsFraFil(const std::string& filnavn) {
    std::ifstream fil(filnavn);
    if (!fil.is_open()) return "";
    std::stringstream buffer;
    buffer << fil.rdbuf();
    return buffer.str();
}

void logBestilling(const std::string& valg) {
    std::lock_guard<std::mutex> lock(filMutex);
    std::ofstream fil("bestillinger.txt", std::ios::app);
    if (fil.is_open()) {
        auto now = std::chrono::system_clock::now();
        std::time_t tid = std::chrono::system_clock::to_time_t(now);
        std::tm* t = std::localtime(&tid);
        fil << "{ \"valg\": \"" << valg << "\", \"tid\": \""
            << std::put_time(t, "%Y-%m-%d %H:%M:%S") << "\" },\n";
        fil.close();
    }
}

void sendI2CCommand(const std::string& cmd) {
    const char* filename = "/dev/i2c-1";
    int file = open(filename, O_RDWR);
    if (file < 0) return;

    if (ioctl(file, I2C_SLAVE, I2C_ADDR) < 0) {
        close(file);
        return;
    }

    write(file, cmd.c_str(), cmd.length());
    close(file);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[30000] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket fejl");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind fejl");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Lyt fejl");
        exit(EXIT_FAILURE);
    }

    std::cout << "✅ Backend kører på http://localhost:" << PORT << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
            perror("Accept fejl");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        read(new_socket, buffer, sizeof(buffer));
        std::string request(buffer);
        std::string responseBody;

        if (request.find("GET /seneste-uid") != std::string::npos) {
            std::string uid = hentUIDFraArduino();
            responseBody = "{\"uid\":\"" + uid + "\"}";
        }
        else if (request.find("POST /gem-valg") != std::string::npos) {
            size_t bodyPos = request.find("\r\n\r\n");
            if (bodyPos != std::string::npos) {
                std::string valg = request.substr(bodyPos + 4);
                skrivTilFil(valgFil, valg);

                if (valg == "Te") sendI2CCommand("mode:1");
                else if (valg == "Lille kaffe") sendI2CCommand("mode:2");
                else if (valg == "Stor kaffe") sendI2CCommand("mode:3");

                responseBody = "{\"status\":\"Valg gemt\"}";
            }
        }
        else if (request.find("GET /tjek-kort") != std::string::npos) {
            size_t pos = request.find("uid=");
            std::string uid = "";
            if (pos != std::string::npos) {
                size_t end = request.find_first_of("& \r\n", pos);
                uid = request.substr(pos + 4, end - (pos + 4));
            }

            bool godkendt = checkGodkendtUID(uid);
            skrivTilFil(kortFil, godkendt ? "1" : "0");
            responseBody = "{\"kortOK\":" + std::string(godkendt ? "true" : "false") + "}";
        }
        else if (request.find("POST /bestil") != std::string::npos) {
            std::string kortStatus = læsFraFil(kortFil);
            std::string valg = læsFraFil(valgFil);

            if (kortStatus == "1" && !valg.empty()) {
                logBestilling(valg);
                sendI2CCommand("s");
                skrivTilFil(kortFil, "0");
                skrivTilFil(valgFil, "");
                responseBody = "{\"status\":\"OK\"}";
            } else {
                responseBody = "{\"error\":\"Ugyldig anmodning\"}";
            }
        }
        else {
            responseBody = "{\"message\":\"Kaffeautomat backend kører\"}";
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
