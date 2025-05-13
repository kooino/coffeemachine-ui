#include <iostream>
#include <fstream>
#include <string>

std::string læsFraFil(const std::string& filnavn) {
    std::ifstream fil(filnavn);
    if (!fil.is_open()) {
        std::cerr << "❌ Kunne ikke åbne filen: " << filnavn << std::endl;
        return "";
    }

    std::string indhold;
    std::getline(fil, indhold);
    fil.close();
    return indhold;
}

int main() {
    std::string uid = læsFraFil("/home/pi/uid.txt");  // Tilpas sti hvis nødvendigt

    if (uid.empty()) {
        std::cout << "Ingen UID fundet eller fil er tom." << std::endl;
    } else {
        std::cout << "✅ UID læst fra fil: " << uid << std::endl;
    }

    return 0;
}
