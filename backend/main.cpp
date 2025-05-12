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

std::mutex logMutex;
std::string gemtValg;

bool checkKort(const std::string& uid) {
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
    int fd = open(filnavn.c_str(), O_RDONLY);
    if (fd < 0) return "";
    char buffer[1024];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        return std::string(buffer);
    }
    return "";
}

void logBestilling(const std::string& valg) {
    std::lock_guard<std::mutex> lock(logMutex);
    int fd = open("bestillinger.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        auto now = std::chrono::system_clock::now();
        std::time_t tid = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&tid);

        std::ostringstream oss;
        oss << "{ \"valg\": \"" << valg << "\", \"timestamp\": \""
            << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") << "\" },\n";

        std::string logEntry = oss.str();
        write(fd, logEntry.c_str(), logEntry.length());
        close(fd);
    }
}

std::string hentBestillinger() {
    int fd = open("bestillinger.txt", O_RDONLY);
    if (fd < 0) return "[]";

    char buffer[8192];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (bytesRead <= 0) return "[]";

    buffer[bytesRead] = '\0';
    std::istringstream stream(buffer);
    std::string line;
    std::vector<std::string> entries;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == ',') line.pop_back();
        if (!line.empty()) entries.push_back(line);
    }

    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < entries.size(); ++i) {
        json << entries[i];
        if (i != entries.size() - 1) json << ",";
    }
    json << "]";
    return json.str();
}

void sendI2CCommand(const std::string& cmd) {
    int file;
    const char *filename = "/dev/i2c-1";
    int addr = 0x08;

    if ((file = open(filename, O_RDWR)) < 0) return;
    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        close(file);
        return;
    }

    write(file, cmd.c_str(), cmd.size());
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

    std::cout << "✅ Server kører på http://localhost:" << PORT << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
            perror("Accept fejl");
            exit(EXIT_FAILURE);
        }

        read(new_socket, buffer, 30000);
        std::string request(buffer);
        std::string responseBody;

        if (request.find("POST /gem-valg") != std::string::npos) {
            size_t bodyPos = request.find("\r\n\r\n");
            if (bodyPos != std::string::npos) {
                gemtValg = request.substr(bodyPos + 4);
                if (gemtValg.front() == '"' && gemtValg.back() == '"') {
                    gemtValg = gemtValg.substr(1, gemtValg.length() - 2);
                }
                skrivTilFil("valg.txt", gemtValg);

                // Send mode til Arduino
                if (gemtValg == "Te") {
                    sendI2CCommand("mode:1");
                } else if (gemtValg == "Lille kaffe") {
                    sendI2CCommand("mode:2");
                } else if (gemtValg == "Stor kaffe") {
                    sendI2CCommand("mode:3");
                }

                responseBody = "{\"status\":\"Valg gemt\"}";
            }
        }
        else if (request.find("GET /tjek-kort") != std::string::npos) {
            size_t uidStart = request.find("uid=");
            std::string uid = (uidStart != std::string::npos) ?
                request.substr(uidStart + 4, 9) : "";

            bool kortOK = checkKort(uid);
            skrivTilFil("kort.txt", kortOK ? "1" : "0");
            responseBody = "{\"kortOK\":" + std::string(kortOK ? "true" : "false") + "}";
        }
        else if (request.find("POST /bestil") != std::string::npos) {
            std::string kortStatus = læsFraFil("kort.txt");
            std::string valg = læsFraFil("valg.txt");

            if (kortStatus == "1" && !valg.empty()) {
                logBestilling(valg);
                sendI2CCommand("start");
                responseBody = "{\"status\":\"OK\"}";
                skrivTilFil("kort.txt", "0");
                skrivTilFil("valg.txt", "");
            } else {
                responseBody = "{\"error\":\"Ugyldig anmodning\"}";
            }
        }
        else if (request.find("GET /bestillinger") != std::string::npos) {
            responseBody = hentBestillinger();
        }
        else {
            responseBody = "{\"message\":\"Kaffeautomat API\"}";
        }

        std::string httpResponse =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: " + std::to_string(responseBody.length()) + "\r\n\r\n" +
            responseBody;

        send(new_socket, httpResponse.c_str(), httpResponse.length(), 0);
        close(new_socket);
    }

    return 0;
}
