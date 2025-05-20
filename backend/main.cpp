
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
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

std::string gemtValg;
std::mutex uidMutex;
std::string senesteUID;
std::atomic<bool> scanningAktiv(true);

// === I2C SEND ===
void sendPumpCommand(char mode) {
    int file = open("/dev/i2c-1", O_RDWR);
    if (file < 0) { perror("I2C open (pump)"); return; }
    if (ioctl(file, I2C_SLAVE, I2C_ADDR_PUMPE) < 0) {
        perror("I2C ioctl (pump)"); close(file); return;
    }
    std::string cmd = "mode:";
    cmd += mode;
    write(file, cmd.c_str(), cmd.size());
    close(file);
    usleep(100000);
}

void sendMotorCommand(char mode) {
    int file = open("/dev/i2c-1", O_RDWR);
    if (file < 0) { perror("I2C open (motor)"); return; }
    if (ioctl(file, I2C_SLAVE, I2C_ADDR_MOTOR) < 0) {
        perror("I2C ioctl (motor)"); close(file); return;
    }
    write(file, &mode, 1);
    close(file);
    usleep(100000);
}

bool checkKort(const std::string& uid) {
    if (uid == "3552077462") return false;
    std::vector<std::string> godkendte = {"165267797", "123456789"};
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
    if (!context) { std::cerr << "libnfc fejl\n"; return; }

    pnd = nfc_open(context, nullptr);
    if (!pnd || nfc_initiator_init(pnd) < 0) {
        std::cerr << "NFC fejl\n";
        return;
    }

    const nfc_modulation mod[1] = { {NMT_ISO14443A, NBR_106} };
    nfc_target target;

    while (scanningAktiv) {
        int res = nfc_initiator_poll_target(pnd, mod, 1, 2, 2, &target);
        if (res > 0) {
            uint32_t uidNum = 0;
            for (size_t i = 0; i < target.nti.nai.szUidLen; i++)
                uidNum = (uidNum << 8) | target.nti.nai.abtUid[i];

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

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {AF_INET, htons(PORT), INADDR_ANY};
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);
    std::cout << "Backend kører på http://localhost:" << PORT << "\n";

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
                    sendPumpCommand('1');
                    sendMotorCommand('1');
                } else if (gemtValg == "Lille kaffe") {
                    sendPumpCommand('2');
                    sendMotorCommand('2');
                } else if (gemtValg == "Stor kaffe") {
                    sendPumpCommand('3');
                    sendMotorCommand('3');
                }

                res = "{\"status\":\"Valg gemt\"}";
            }
        } else if (req.find("GET /tjek-kort") != std::string::npos) {
            std::string uid = filtrerUID(senesteUID);
            bool ok = !uid.empty() && checkKort(uid);
            skrivTilFil("kort.txt", ok ? "1" : "0");

            if (uid.empty()) res = "{\"kortOK\": false}";
            else if (!ok) res = "{\"kortOK\": false, \"error\": \"Forkert kort\"}";
            else res = "{\"kortOK\": true}";

        } else if (req.find("POST /bestil") != std::string::npos) {
            std::string kort = læsFraFil("kort.txt");
            if (kort == "1" && !gemtValg.empty()) {
                sendPumpCommand('s');
                skrivTilFil("kort.txt", "0");
                skrivTilFil("valg.txt", "");
                res = "{\"status\":\"OK\"}";
            } else res = "{\"error\":\"Ugyldig anmodning\"}";
        } else if (req.find("POST /annuller") != std::string::npos) {
            skrivTilFil("kort.txt", "0");
            skrivTilFil("valg.txt", "");
            res = "{\"status\":\"Annulleret\"}";
        } else {
            res = "{\"message\":\"Kaffeautomat API\"}";
        }

        std::string http =
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: "
            + std::to_string(res.size()) + "\r\n\r\n" + res;

        send(client, http.c_str(), http.size(), 0);
        close(client);
    }

    scanningAktiv = false;
    t.join();
    return 0;
}

