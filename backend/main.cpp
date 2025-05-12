// Full backend with I2C UID reading and API
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
std::string gemtValg;

bool checkKort(const std::string& uid) {
    std::vector<std::string> godkendteUIDs = { "165267797", "123456789" };
    return std::find(godkendteUIDs.begin(), godkendteUIDs.end(), uid) != godkendteUIDs.end();
}

void skrivTilFil(const std::string& filnavn, const std::string& data) {
    std::ofstream out(filnavn);
    out << data;
}

std::string læsFraFil(const std::string& filnavn) {
    std::ifstream in(filnavn);
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

void logBestilling(const std::string& valg) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream log("bestillinger.txt", std::ios::app);
    auto now = std::chrono::system_clock::now();
    std::time_t tid = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&tid);
    log << "{ \"valg\": \"" << valg << "\", \"timestamp\": \""
        << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") << "\" },\n";
}

std::string hentBestillinger() {
    return læsFraFil("bestillinger.txt");
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
    usleep(100000);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);
    std::cout << "Backend lytter på port " << PORT << std::endl;

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);
        char buffer[4096] = {0};
        read(client, buffer, sizeof(buffer));
        std::string req(buffer);
        std::string res;

        if (req.find("POST /gem-valg") != std::string::npos) {
            size_t pos = req.find("\r\n\r\n");
            if (pos != std::string::npos) {
                gemtValg = req.substr(pos + 4);
                skrivTilFil("valg.txt", gemtValg);
                if (gemtValg == "Te") sendI2CCommand("mode:1");
                else if (gemtValg == "Lille kaffe") sendI2CCommand("mode:2");
                else if (gemtValg == "Stor kaffe") sendI2CCommand("mode:3");
                res = "{\"status\":\"Valg gemt\"}";
            }
        }
        else if (req.find("POST /bestil") != std::string::npos) {
            std::string kort = læsFraFil("kort.txt");
            std::string valg = læsFraFil("valg.txt");
            if (kort == "1" && !valg.empty()) {
                logBestilling(valg);
                sendI2CCommand("s");
                skrivTilFil("kort.txt", "0");
                skrivTilFil("valg.txt", "");
                res = "{\"status\":\"OK\"}";
            } else res = "{\"error\":\"Ugyldig anmodning\"}";
        }
        else if (req.find("GET /tjek-kort") != std::string::npos) {
            size_t pos = req.find("uid=");
            std::string uid = "";
            if (pos != std::string::npos) {
                size_t end = req.find_first_of("& \\r\\n", pos);
                uid = req.substr(pos + 4, end - (pos + 4));
            }
            bool ok = checkKort(uid);
            skrivTilFil("kort.txt", ok ? "1" : "0");
            res = "{\"kortOK\":" + std::string(ok ? "true" : "false") + "}";
        }
        else if (req.find("GET /seneste-uid") != std::string::npos) {
            const char* filename = "/dev/i2c-1";
            int file = open(filename, O_RDWR);
            std::string uid = "";

            if (file >= 0 && ioctl(file, I2C_SLAVE, I2C_ADDR) >= 0) {
                char buffer[32] = {0};
                if (read(file, buffer, sizeof(buffer)) > 0) {
                    uid = buffer;
                } else {
                    uid = "";
                }
                close(file);
            }

            res = "{\"uid\":\"" + uid + "\"}";
        }
        else if (req.find("GET /bestillinger") != std::string::npos) {
            res = hentBestillinger();
        }
        else {
            res = "{\"message\":\"Kaffeautomat API\"}";
        }

        std::string response =
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + std::to_string(res.size()) + "\r\n\r\n" + res;
        send(client, response.c_str(), response.size(), 0);
        close(client);
    }
}
