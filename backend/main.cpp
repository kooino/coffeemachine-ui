Idris
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
std::string gemtValg;
std::string senesteUID;
std::atomic<bool> scanningAktiv(true);

void sendI2CCommand(const std::string& cmd) {
    const char* filename = "/dev/i2c-1";
    int file = open(filename, O_RDWR);
    if (file < 0) {
        perror("❌ I2C open (pumpe)");
        return;
    }
    if (ioctl(file, I2C_SLAVE, I2C_ADDR_PUMPE) < 0) {
        perror("❌ I2C ioctl (pumpe)");
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
    if (file < 0) {
        perror("❌ I2C open (motor)");
        return;
    }
    if (ioctl(file, I2C_SLAVE, I2C_ADDR_MOTOR) < 0) {
        perror("❌ I2C ioctl (motor)");
        close(file);
        return;
    }
    char buf[1] = { mode };
    if (write(file, buf, 1) != 1) {
        perror("❌ Write motor");
    } else {
        std::cout << "✅ Sendt til motor: '" << mode << "' (" << (int)mode << ")" << std::endl;
    }
    close(file);
    usleep(100000);
}

bool checkKort(const std::string& uid) {
    if (uid == "3552077462") return false;
    std::vector<std::string> godkendte = { "165267797", "123456789" };
    return std::find(godkendte.begin(), godkendte.end(), uid) != godkendte.end();
}

std::string filtrerUID(const std::string& input) {
    std::string r;
    for (char c : input) if (isdigit(c)) r += c;
    return r;
}

void scanningThread() {
    nfc_context* context = nullptr;
    nfc_device* pnd = nullptr;
    nfc_init(&context);
    if (!context) return;
    pnd = nfc_open(context, nullptr);
    if (!pnd || nfc_initiator_init(pnd) < 0) return;
    const nfc_modulation mod[1] = {{NMT_ISO14443A, NBR_106}};
    nfc_target target;

    while (scanningAktiv) {
        int res = nfc_initiator_poll_target(pnd, mod, 1, 2, 2, &target);
        if (res > 0) {
            uint32_t uidNum = 0;
            for (size_t i = 0; i < target.nti.nai.szUidLen; i++)
                uidNum = (uidNum << 😎 | target.nti.nai.abtUid[i];
            {
                std::lock_guard<std::mutex> lock(uidMutex);
                senesteUID = std::to_string(uidNum);
            }
            while (nfc_initiator_target_is_present(pnd, nullptr) == 0 && scanningAktiv)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            std::lock_guard<std::mutex> lock(uidMutex);
            senesteUID = "";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    nfc_close(pnd);
    nfc_exit(context);
}

void skrivTilFil(const std::string& fil, const std::string& data) {
    std::ofstream f(fil); if (f) f << data;
}

std::string læsFraFil(const std::string& fil) {
    std::ifstream f(fil); std::ostringstream ss;
    if (f) ss << f.rdbuf();
    return ss.str();
}

void logBestilling(const std::string& valg) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream f("bestillinger.txt", std::ios::app);
    if (f) {
        auto now = std::chrono::system_clock::now();
        std::time_t tid = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&tid);
        f << "{ \"valg\": \"" << valg << "\", \"timestamp\": \""
          << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") << "\" },\n";
    }
}

std::string hentBestillinger() {
    std::ifstream f("bestillinger.txt");
    std::string line, all;
    while (getline(f, line)) {
        if (!line.empty() && line.back() == ',') line.pop_back();
        all += line + ",";
    }
    if (!all.empty() && all.back() == ',') all.pop_back();
    return "[" + all + "]";
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {AF_INET, htons(PORT), INADDR_ANY};
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);
    std::cout << "✅ Backend kører på http://localhost:" << PORT << "\n";

    std::thread t(scanningThread);

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);
        char buffer[30000] = {0};
        read(client, buffer, sizeof(buffer));
        std::string req(buffer), res;

        if (req.find("POST /gem-valg") != std::string::npos) {
            size_t pos = req.find("\r\n\r\n");
            if (pos != std::string::npos) {
                gemtValg = req.substr(pos + 4);
                skrivTilFil("valg.txt", gemtValg);

                if (gemtValg == "Te") {
                    sendI2CCommand("mode:1");
                    sendMotorCommand('1');
                } else if (gemtValg == "Lille kaffe") {
                    sendI2CCommand("mode:2");
                    sendMotorCommand('2');
                } else if (gemtValg == "Stor kaffe") {
                    sendI2CCommand("mode:3");
                    sendMotorCommand('3');
                }

                res = "{\"status\"😕"Valg gemt\"}";
            }
        }
        else if (req.find("GET /tjek-kort") != std::string::npos) {
            std::string uid;
            {
                std::lock_guard<std::mutex> lock(uidMutex);
                uid = senesteUID;
            }
            uid = filtrerUID(uid);
            bool ok = !uid.empty() && checkKort(uid);
            skrivTilFil("kort.txt", ok ? "1" : "0");
            res = ok ? "{\"kortOK\": true}" : uid.empty() ? "{\"kortOK\": false}" : "{\"kortOK\": false, \"error\": \"Forkert kort\"}";
        }
        else if (req.find("POST /bestil") != std::string::npos) {
            std::string kort = læsFraFil("kort.txt");
            std::string valg = læsFraFil("valg.txt");
            if (kort == "1" && !valg.empty()) {
                logBestilling(valg);
                sendI2CCommand("s");
                skrivTilFil("kort.txt", "0");
                skrivTilFil("valg.txt", "");
                res = "{\"status\"😕"OK\"}";
            } else {
                res = "{\"error\"😕"Ugyldig anmodning\"}";
            }
        }
        else if (req.find("POST /annuller") != std::string::npos) {
            skrivTilFil("kort.txt", "0");
            skrivTilFil("valg.txt", "");
            res = "{\"status\"😕"Annulleret\"}";
        }
        else if (req.find("GET /bestillinger") != std::string::npos) {
            res = hentBestillinger();
        }
        else {
            res = "{\"message\"😕"Kaffeautomat API\"}";
        }

        std::string http = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + std::to_string(res.size()) + "\r\n\r\n" + res;
        send(client, http.c_str(), http.size(), 0);
        close(client);
    }

    scanningAktiv = false;
    t.join();
    return 0;
}
