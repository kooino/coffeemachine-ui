// Backend med serial RFID UID-integration
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 5000

void lytSerial() {
    FILE* ser = fopen("/dev/ttyACM0", "r");  // Justér hvis din Arduino sidder et andet sted
    if (!ser) {
        perror("Serial fejl");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), ser)) {
        std::string str(line);
        if (str.find("UID:") != std::string::npos) {
            std::string uid = str.substr(4);
            uid.erase(uid.find_last_not_of(" \r\n") + 1);

            std::ofstream out("seneste_uid.txt");
            out << uid;
            out.close();

            std::cout << "Modtaget UID: " << uid << std::endl;
        }
    }
    fclose(ser);
}

std::string læsFil(const std::string& filnavn) {
    std::ifstream in(filnavn);
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

int main() {
    std::thread t(lytSerial);
    t.detach();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);
    std::cout << "Server kører på port " << PORT << std::endl;

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);
        char buffer[4096] = {0};
        read(client, buffer, sizeof(buffer));
        std::string req(buffer);
        std::string res;

        if (req.find("GET /seneste-uid") != std::string::npos) {
            std::string uid = læsFil("seneste_uid.txt");
            res = "{\"uid\":\"" + uid + "\"}";
        } else {
            res = "{\"message\":\"Brug /seneste-uid\"}";
        }

        std::string response =
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + std::to_string(res.size()) + "\r\n\r\n" + res;

        send(client, response.c_str(), response.size(), 0);
        close(client);
    }

    return 0;
}
