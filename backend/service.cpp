#include "service.h"
#include <fstream>

std::string rydBestillinger() {
    std::ofstream fil("bestillinger.txt", std::ios::trunc);
    fil.close();
    return "{\"status\":\"Bestillinger ryddet\"}";
}
