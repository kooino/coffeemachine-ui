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
#include <nfc/nfc.h>

#define PORT 5000

std::mutex filMutex;
std::string valgFil = "valg.txt";
std::string kortFil = "kort.txt";

std::string hentUIDFraPN532() {
    nfc_context *context;
    nfc_device *pnd;
    nfc_init(&context);
    if (!context) return "";

    pnd = nfc_open(context, nullptr);
    if (!pnd) {
        nfc_exit(context);
        return "";
    }

    if (nfc_initiator_init(pnd) < 0) {
        nfc_close(pnd);
        nfc_exit(context);
        return "";
    }

    const nfc_modulation nmMifare = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_106
    };

    nfc_target nt;
    std::string result = "";

    if (nfc_initiator_select_passive_target(pnd, nmMifare, nullptr, 0, &nt) > 0) {
        if (nt.nti.nai.szUidLen == 4) {
            uint32_t uid_int = 0;
            uid_int |= (nt.nti.nai.abtUid[0] << 24);
            uid_int |= (nt.nti.nai.abtUid[1] << 16);
            uid_int |= (nt.nti.nai.abtUid[2] << 8);
            uid_int |= nt.nti.nai.abtUid[3];

            result = std::to_string(uid_int);
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
    return result;
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

std::string laesFraFil(const std::string& filnavn) {
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
            std::string uid = hentUIDFraPN532();
            responseBody = "{\"uid\":\"" + uid + "\"}";
        }
        else if (request.find("POST /gem-valg") != std::string::npos) {
            size_t bodyPos = request.find("\r\n\r\n");
            if (bodyPos != std::string::npos) {
                std::string valg = request.substr(bodyPos + 4);
                skrivTilFil(valgFil, valg);
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
            std::string kortStatus = laesFraFil(kortFil);
            std::string valg = laesFraFil(valgFil);

            if (kortStatus == "1" && !valg.empty()) {
                logBestilling(valg);
                skrivTilFil(kortFil, "0");
                skrivTilFil(valgFil, "");
                responseBody = "{\"status\":\"OK\"}";
            } else {
                responseBody = "{\"error\":\"Ugyldig anmodning\"}";
            }
        } else {
            responseBody = "{\"message\":\"Kaffeautomat backend k\u00f8rer\"}";
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
