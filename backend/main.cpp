#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <ctime>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <nfc/nfc.h>
#include <fcntl.h>

#define PORT 5000

std::mutex dataMutex;
std::string senesteUID = "";
bool kortOK = false;
std::string gemtValg = "";

std::vector<std::string> godkendteUIDs = {
    "165267797",
    "123456789"
};

bool checkKort(const std::string& uid) {
    for (const auto& godkendt : godkendteUIDs)
        if (uid == godkendt) return true;
    return false;
}

void skrivTilFil(const std::string& navn, const std::string& data) {
    std::ofstream fil(navn);
    if (fil.is_open()) {
        fil << data;
        fil.close();
    }
}

std::string læsFraFil(const std::string& navn) {
    std::ifstream fil(navn);
    std::string data;
    std::getline(fil, data);
    return data;
}

void logBestilling(const std::string& valg) {
    std::ofstream fil("bestillinger.txt", std::ios::app);
    auto nu = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    fil << "{ \"valg\": \"" << valg << "\", \"timestamp\": \"" << std::put_time(std::localtime(&nu), "%Y-%m-%d %H:%M:%S") << "\" },\n";
}

std::string hentBestillinger() {
    std::ifstream fil("bestillinger.txt");
    std::stringstream ss;
    ss << "[";
    std::string linje;
    while (std::getline(fil, linje)) {
        if (linje.back() == ',') linje.pop_back();
        ss << linje << ",";
    }
    std::string result = ss.str();
    if (result.back() == ',') result.pop_back();
    result += "]";
    return result;
}

void aktiverPumpe(int pin = 17, int varighedSek = 5) {
    std::ostringstream path;
    path << "/sys/class/gpio/gpio" << pin;

    // Eksporter GPIO hvis nødvendigt
    if (access(path.str().c_str(), F_OK) == -1) {
        std::ofstream eksport("/sys/class/gpio/export");
        eksport << pin;
        eksport.close();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Sæt som output
    std::ofstream dir(path.str() + "/direction");
    dir << "out";
    dir.close();

    // Tænd pumpen
    std::ofstream ud(path.str() + "/value");
    ud << "1";
    ud.close();

    std::cout << "🟢 Pumpe aktiveret (" << varighedSek << " sekunder)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(varighedSek));

    // Sluk pumpen
    std::ofstream udStop(path.str() + "/value");
    udStop << "0";
    udStop.close();

    std::cout << "🔴 Pumpe slukket." << std::endl;
}

void startNFC() {
    nfc_context *context;
    nfc_device *pnd;
    nfc_init(&context);
    if (!context) return;

    pnd = nfc_open(context, nullptr);
    if (!pnd) return;

    if (nfc_initiator_init(pnd) < 0) return;

    const nfc_modulation nm = { NMT_ISO14443A, NBR_106 };
    nfc_target nt;

    while (true) {
        if (nfc_initiator_select_passive_target(pnd, nm, nullptr, 0, &nt) > 0) {
            if (nt.nti.nai.szUidLen == 4) {
                uint32_t uid_int = 0;
                uid_int |= (nt.nti.nai.abtUid[0] << 24);
                uid_int |= (nt.nti.nai.abtUid[1] << 16);
                uid_int |= (nt.nti.nai.abtUid[2] << 8);
                uid_int |= nt.nti.nai.abtUid[3];

                std::string uidStr = std::to_string(uid_int);
                bool erGodkendt = checkKort(uidStr);

                {
                    std::lock_guard<std::mutex> lock(dataMutex);
                    senesteUID = uidStr;
                    kortOK = erGodkendt;
                }

                skrivTilFil("kort.txt", erGodkendt ? "1" : "0");
                std::cout << "📶 Kort læst: " << uidStr << (erGodkendt ? " ✅" : " ❌") << std::endl;
            }
            sleep(1);
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
}

int main() {
    std::thread nfcTråd(startNFC);
    nfcTråd.detach();

    int server_fd, ny_socket;
    struct sockaddr_in adresse;
    char buffer[30000] = {0};
    socklen_t addrlen = sizeof(adresse);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = INADDR_ANY;
    adresse.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&adresse, sizeof(adresse));
    listen(server_fd, 10);
    std::cout << "✅ Server kører på http://localhost:" << PORT << std::endl;

    while (true) {
        ny_socket = accept(server_fd, (struct sockaddr*)&adresse, &addrlen);
        read(ny_socket, buffer, sizeof(buffer));
        std::string request(buffer);
        std::string responseBody;

        if (request.find("POST /gem-valg") != std::string::npos) {
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                gemtValg = request.substr(bodyStart + 4);
                skrivTilFil("valg.txt", gemtValg);
                responseBody = "{\"status\":\"Valg gemt\"}";
            }
        }

        else if (request.find("POST /bestil") != std::string::npos) {
            std::string valg = læsFraFil("valg.txt");
            std::string kortStatus = læsFraFil("kort.txt");
            if (kortStatus == "1" && !valg.empty()) {
                logBestilling(valg);
                aktiverPumpe();  // Aktiver pumpen via GPIO
                skrivTilFil("kort.txt", "0");
                skrivTilFil("valg.txt", "");
                responseBody = "{\"status\":\"Bestilling gennemført\"}";
            } else {
                responseBody = "{\"error\":\"Kort ikke godkendt eller valg mangler\"}";
            }
        }

        else if (request.find("GET /seneste-uid") != std::string::npos) {
            std::string uid;
            bool valid;
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                uid = senesteUID;
                valid = kortOK;
            }
            std::ostringstream oss;
            oss << "{ \"uid\": \"" << uid << "\", \"valid\": " << (valid ? "true" : "false") << " }";
            responseBody = oss.str();
        }

        else if (request.find("POST /annuller") != std::string::npos) {
            std::lock_guard<std::mutex> lock(dataMutex);
            senesteUID = "";
            kortOK = false;
            skrivTilFil("kort.txt", "0");
            skrivTilFil("valg.txt", "");
            responseBody = "{\"status\":\"Annulleret\"}";
        }

        else {
            responseBody = "{\"message\":\"Kaffeautomat API\"}";
        }

        std::string httpResponse =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: " + std::to_string(responseBody.length()) + "\r\n\r\n" + responseBody;

        send(ny_socket, httpResponse.c_str(), httpResponse.size(), 0);
        close(ny_socket);
    }

    return 0;
}
