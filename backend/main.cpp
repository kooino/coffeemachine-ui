#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define PORT 5000
#define I2C_ADDR 0x24  // Match Arduino I2C-slaveadresse

std::string hentUIDFraArduino() {
    const char* filename = "/dev/i2c-1";
    int file = open(filename, O_RDWR);
    if (file < 0) {
        perror("❌ Kunne ikke åbne I2C-enhed");
        return "";
    }

    if (ioctl(file, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("❌ Kunne ikke sætte I2C-adresse");
        close(file);
        return "";
    }

    char buffer[32] = {0};
    ssize_t bytes = read(file, buffer, sizeof(buffer) - 1);
    close(file);

    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::string uid(buffer);
        std::cout << "✅ UID modtaget fra Arduino: " << uid << std::endl;
        return uid;
    }

    std::cout << "⚠️ Intet UID modtaget." << std::endl;
    return "";
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
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        read(new_socket, buffer, sizeof(buffer));
        std::string request(buffer);
        std::string responseBody;

        if (request.find("GET /seneste-uid") != std::string::npos) {
            std::string uid = hentUIDFraArduino();
            responseBody = "{\"uid\":\"" + uid + "\"}";
        } else {
            responseBody = "{\"message\":\"NFC testserver\"}";
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
