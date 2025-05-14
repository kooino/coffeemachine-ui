// ✅ Fuld backendkode til kaffeautomat med RFID og I2C
// PN532 læser UID, validerer kort, gemmer valg, sender I2C-kommandoer

#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <vector>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <nfc/nfc.h>

#define PORT 5000
#define I2C_ADDR 0x08

std::mutex logMutex, uidMutex;
std::atomic<bool> nfcRunning{true};
std::string latestUID, gemtValg;

bool checkKort(const std::string& uid) {
    std::vector<std::string> godkendteUIDs = {"0458477EAE6D80", "04A8B26EAE6D80"};
    return std::find(godkendteUIDs.begin(), godkendteUIDs.end(), uid) != godkendteUIDs.end();
}

void logBestilling(const std::string& valg) {
    std::lock_guard<std::mutex> lock(logMutex);
    int fd = open("bestillinger.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        auto now = std::chrono::system_clock::now();
        std::time_t tid = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&tid);
        std::ostringstream oss;
        char tidsbuffer[20];
        std::strftime(tidsbuffer, sizeof(tidsbuffer), "%Y-%m-%d %H:%M:%S", now_tm);
        oss << "{ \"valg\": \"" << valg << "\", \"timestamp\": \"" << tidsbuffer << "\" },\n";
        write(fd, oss.str().c_str(), oss.str().length());
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
    std::vector<std::string> entries;
    std::istringstream stream(buffer);
    std::string line;

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
    const char* filename = "/dev/i2c-1";
    int file = open(filename, O_RDWR);
    if (file < 0) {
        perror("❌ I2C-enhed fejl");
        return;
    }

    if (ioctl(file, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("❌ I2C-slave fejl");
        close(file);
        return;
    }

    if (write(file, cmd.c_str(), cmd.length()) != (ssize_t)cmd.length()) {
        perror("❌ I2C-skrivefejl");
    } else {
        std::cout << "✅ I2C-kommando: " << cmd << std::endl;
    }

    close(file);
    usleep(100000);
}

void readNFC() {
    nfc_context* context;
    nfc_init(&context);
    nfc_device* device = nfc_open(context, nullptr);

    if (!device) {
        std::cerr << "❌ NFC-enhed ikke tilgængelig" << std::endl;
        return;
    }

    nfc_initiator_init(device);
    const nfc_modulation nm = {.nmt = NMT_ISO14443A, .nbr = NBR_106};

    while (nfcRunning) {
        nfc_target target;
        int res = nfc_initiator_select_passive_target(device, nm, nullptr, 0, &target);

        if (res > 0) {
            std::string uid;
            for (size_t i = 0; i < target.nti.nai.szUidLen; i++) {
                char buf[3];
                sprintf(buf, "%02X", target.nti.nai.abtUid[i]);
                uid += buf;
            }

            {
                std::lock_guard<std::mutex> lock(uidMutex);
                latestUID = uid;
            }

            nfc_initiator_deselect_target(device);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    nfc_close(device);
    nfc_exit(context);
}

int main() {
    std::thread nfcThread(readNFC);
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socketfejl");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bindfejl");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Lyttefejl");
        exit(EXIT_FAILURE);
    }

    std::cout << "✅ Server kører på port " << PORT << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
            perror("Acceptfejl");
            continue;
        }

        char buffer[30000] = {0};
        read(new_socket, buffer, sizeof(buffer));
        std::string request(buffer);
        std::string responseBody;

        if (request.find("POST /gem-valg") != std::string::npos) {
            size_t bodyPos = request.find("\r\n\r\n");
            if (bodyPos != std::string::npos) {
                gemtValg = request.substr(bodyPos + 4);

                if (gemtValg == "Te") sendI2CCommand("mode:1");
                else if (gemtValg == "Lille kaffe") sendI2CCommand("mode:2");
                else if (gemtValg == "Stor kaffe") sendI2CCommand("mode:3");

                responseBody = "{\"status\":\"Valg gemt\"}";
            }
        }
        else if (request.find("POST /bestil") != std::string::npos) {
            std::lock_guard<std::mutex> lock(uidMutex);
            if (checkKort(latestUID) && !gemtValg.empty()) {
                logBestilling(gemtValg);
                sendI2CCommand("s");
                latestUID.clear();
                gemtValg.clear();
                responseBody = "{\"status\":\"Bestilling gennemført\"}";
            } else {
                responseBody = "{\"error\":\"Ugyldig bestilling\"}";
            }
        }
        else if (request.find("GET /seneste-uid") != std::string::npos) {
            std::lock_guard<std::mutex> lock(uidMutex);
            responseBody = "{\"uid\":\"" + latestUID + "\",\"valid\":" + (checkKort(latestUID) ? "true" : "false") + "}";
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
            "Content-Length: " + std::to_string(responseBody.size()) + "\r\n\r\n" +
            responseBody;

        send(new_socket, httpResponse.c_str(), httpResponse.size(), 0);
        close(new_socket);
    }

    nfcRunning = false;
    if (nfcThread.joinable()) nfcThread.join();
    return 0;
}
