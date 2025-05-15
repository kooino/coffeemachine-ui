#include <iostream>
#include <iomanip>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <string>
#include <nfc/nfc.h>

// Liste over godkendte kort i decimal-format
std::vector<std::string> godkendteUIDs = {
    "165267797",  // Dit eget kort
    "123456789"
};

// Skriver tekst til fil (fx "1" eller "0")
void skrivTilFil(const std::string& filnavn, const std::string& indhold) {
    std::ofstream fil(filnavn);
    if (fil.is_open()) {
        fil << indhold;
        fil.close();
    }
}

// Tjekker om et UID findes i listen
bool checkKort(const std::string& uid) {
    for (const auto& godkendt : godkendteUIDs) {
        if (uid == godkendt) return true;
    }
    return false;
}

int main() {
    nfc_context *context;
    nfc_device *pnd;

    nfc_init(&context);
    if (!context) {
        std::cerr << "❌ Kunne ikke initialisere NFC" << std::endl;
        return 1;
    }

    pnd = nfc_open(context, nullptr);
    if (!pnd) {
        std::cerr << "❌ Kunne ikke åbne NFC-enhed" << std::endl;
        nfc_exit(context);
        return 1;
    }

    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "❌ Fejl ved initialisering" << std::endl;
        nfc_close(pnd);
        nfc_exit(context);
        return 1;
    }

    std::cout << "📡 NFC klar – læg kortet på læseren..." << std::endl;

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
                bool godkendt = checkKort(uidStr);

                std::cout << "🔎 Læst UID: " << uidStr << " → ";
                if (godkendt) {
                    std::cout << "✅ Godkendt" << std::endl;
                    skrivTilFil("kort.txt", "1");
                } else {
                    std::cout << "❌ Ikke godkendt" << std::endl;
                    skrivTilFil("kort.txt", "0");
                }
            } else {
                std::cout << "⚠️ UID er ikke 4 bytes – springer over." << std::endl;
            }

            sleep(1); // Vent lidt før næste læsning
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
    return 0;
}
