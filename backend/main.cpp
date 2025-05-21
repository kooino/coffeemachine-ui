#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <vector>
#include <algorithm>
#include <mutex>
#include <iomanip>
#include <thread>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <nfc/nfc.h>

#define PORT 5000
#define I2C_ADDR_PUMPE 0x08
#define I2C_ADDR_MOTOR 0x30

std::mutex logMutex;
std::mutex uidMutex;

std::string senesteUID;
std::atomic<bool> scanningAktiv(true);

int storeKopper = 0;    // tæller for store kopper kaffe
int lilleKopper = 0;    // tæller for små kopper kaffe og te

// Funktion til at sende besked til service.txt
void skrivServiceBesked(const std::string& besked) {
    int fd = open("service.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        auto now = std::chrono::system_clock::now();
        std::time_t tid = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&tid);
        std::ostringstream oss;
        oss << "[" << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") << "] " << besked << "\n";
        write(fd, oss.str().c_str(), oss.str().length());
        close(fd);
    }
}

void sendI2CCommand(const std::string& cmd) {
    const char* filename = "/dev/i2c-1";
    int file = open(filename, O_RDWR);
    if (file < 0) return;
    if (ioctl(file, I2C_SLAVE, I2C_ADDR_PUMPE) < 0) {
        close(file);
        return;
    }
    write(file, cmd.c_str(), cmd.length());
    close(file);
    usleep(100000);
}

void sendMotorCommand(char mode) {
    const char* filename = "/dev/i2c-1";
    int file = open(filename, O_RDWR);
    if (file < 0) return;
    if (ioctl(file, I2C_SLAVE, I2C_ADDR_MOTOR) < 0) {
        close(file);
        return;
    }
    char buf[1] = { mode };
    write(file, buf, 1);
    close(file);
    usleep(100000);
}

bool checkKort(const std::string& uid) {
    if (uid == "3552077462") return false;
    std::vector<std::string> godkendteUIDs = { "165267797", "123456789" };
    return std::find(godkendteUIDs.begin(), godkendteUIDs.end(), uid) != godkendteUIDs.end();
}

std::string filtrerUID(const std::string& input) {
    std::string resultat;
    for (char c : input) {
        if (isdigit(c)) resultat += c;
    }
    return resultat;
}

void scanningThread() {
    nfc_context* context = nullptr;
    nfc_device* pnd = nullptr;
    nfc_init(&context);
    if (!context) return;
    pnd = nfc_open(context, nullptr);
    if (!pnd || nfc_initiator_init(pnd) < 0) {
        if (pnd) nfc_close(pnd);
        nfc_exit(context);
        return;
    }
    const nfc_modulation mod[1] = { { NMT_ISO14443A, NBR_106 } };
    nfc_target target;
    while (scanningAktiv) {
        int res = nfc_initiator_poll_target(pnd, mod, 1, 2, 2, &target);
        if (res > 0) {
            uint32_t uidNum = 0;
            for (size_t i = 0; i < target.nti.nai.szUidLen; i++) {
                uidNum = (uidNum << 8) | target.nti.nai.abtUid[i];
            }
            {
                std::lock_guard<std::mutex> lock(uidMutex);
                senesteUID = std::to_string(uidNum);
            }
            while (nfc_initiator_target_is_present(pnd, nullptr) == 0 && scanningAktiv) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else {
            std::lock_guard<std::mutex> lock(uidMutex);
            senesteUID.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    nfc_close(pnd);
    nfc_exit(context);
}

void logBestilling(const std::string& valg) {
    std::lock_guard<std::mutex> lock(logMutex);
    int fd = open("bestillinger.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd != -1) {
        auto now = std::chrono::system_clock::now();
        std::time_t tid = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&tid);
        std::ostringstream oss;
        oss << "{ \"valg\": \"" << valg << "\", \"timestamp\": \"" << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") << "\" },\n";
        write(fd, oss.str().c_str(), oss.str().length());
        close(fd);
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) exit(EXIT_FAILURE);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) exit(EXIT_FAILURE);
    if (listen(server_fd, 10) < 0) exit(EXIT_FAILURE);
    std::cout << "Backend server kører på http://localhost:" << PORT << std::endl;

    std::thread t(scanningThread);

    char buffer[30000] = {0};
    while (true) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (new_socket < 0) continue;
        memset(buffer, 0, sizeof(buffer));
        read(new_socket, buffer, sizeof(buffer));
        std::string request(buffer);
        std::string responseBody;

        if (request.find("POST /gem-valg") != std::string::npos) {
            size_t bodyPos = request.find("\r\n\r\n");
            if (bodyPos != std::string::npos) {
                std::string valg = request.substr(bodyPos + 4);
                logBestilling(valg);

                // Send I2C kommandoer til pumpe og motor baseret på valg
                if (valg == "Te") {
                    sendI2CCommand("mode:1");
                    sendMotorCommand('1');
                    lilleKopper++;
                } else if (valg == "Lille kaffe") {
                    sendI2CCommand("mode:2");
                    sendMotorCommand('2');
                    lilleKopper++;
                } else if (valg == "Stor kaffe") {
                    sendI2CCommand("mode:3");
                    sendMotorCommand('3');
                    storeKopper++;
                }

                // Check for service beskeder
                if (storeKopper >= 3) {
                    skrivServiceBesked("Mangel på kaffebønner: 3 store kopper lavet.");
                    storeKopper = 0; // reset tæller efter besked
                }
                if (lilleKopper >= 4) {
                    skrivServiceBesked("Mangel på vand: 4 små kopper lavet.");
                    lilleKopper = 0; // reset tæller efter besked
                }

                responseBody = "{\"status\":\"Valg gemt\"}";
            }
        } else if (request.find("GET /tjek-kort") != std::string::npos) {
            std::string uid;
            {
                std::lock_guard<std::mutex> lock(uidMutex);
                uid = senesteUID;
            }
            uid = filtrerUID(uid);
            bool kortOK = (!uid.empty() && checkKort(uid));
            if (uid.empty()) responseBody = "{\"kortOK\": false}";
            else if (!kortOK) responseBody = "{\"kortOK\": false, \"error\": \"Forkert kort\"}";
            else responseBody = "{\"kortOK\": true}";
        } else if (request.find("POST /bestil") != std::string::npos) {
            responseBody = "{\"status\":\"OK\"}";
        } else if (request.find("POST /annuller") != std::string::npos) {
            sendMotorCommand('a');  // stop motoren
            sendI2CCommand("a");     // stop pumpen
            responseBody = "{\"status\":\"Annulleret\"}";
        } else if (request.find("GET /bestillinger") != std::string::npos) {
            // Returner indhold af bestillinger.txt
            int fd = open("bestillinger.txt", O_RDONLY);
            if (fd >= 0) {
                char buf[8192];
                ssize_t bytesRead = read(fd, buf, sizeof(buf) - 1);
                close(fd);
                if (bytesRead > 0) {
                    buf[bytesRead] = '\0';
                    responseBody = std::string(buf);
                } else {
                    responseBody = "[]";
                }
            } else {
                responseBody = "[]";
            }
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

    scanningAktiv = false;
    t.join();
    return 0;
}
