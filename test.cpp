#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <fstream>
#include <unistd.h>
#include <nfc/nfc.h>

std::atomic<std::string> senesteUID = "";
std::atomic<bool> kortOK = false;

// Liste over godkendte UID'er som decimal heltal
std::vector<std::string> godkendteUIDs = {
    "165267797",  // Eksempel UID
    "123456789"
};

bool checkKort(const std::string& uid) {
    return std::find(godkendteUIDs.begin(), godkendteUIDs.end(), uid) != godkendteUIDs.end();
}

void skrivTilFil(const std::string& navn, const std::string& data) {
    std::ofstream fil(navn);
    if (fil.is_open()) {
        fil << data;
        fil.close();
    }
}

void startNFC() {
    nfc_context* context;
    nfc_device* pnd;

    nfc_init(&context);
    if (!context) {
        std::cerr << "❌ Kunne ikke initialisere NFC" << std::endl;
        return;
    }

    pnd = nfc_open(context, nullptr);
    if (!pnd) {
        std::cerr << "❌ Kunne ikke åbne NFC-enhed" << std::endl;
        nfc_exit(context);
        return;
    }

    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "❌ Initiering som initiator fejlede" << std::endl;
        nfc_close(pnd);
        nfc_exit(context);
        return;
    }

    std::cout << "📡 NFC-læser klar. Læg kortet på..." << std::endl;

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

                std::string uid_str = std::to_string(uid_int);
                senesteUID = uid_str;

                bool godkendt = checkKort(uid_str);
                kortOK = godkendt;

                skrivTilFil("kort.txt", godkendt ? "1" : "0");

                std::cout << "✅ UID: " << uid_str
                          << (godkendt ? " (Godkendt)" : " (Afvist)") << std::endl;
            } else {
                std::cout << "⚠️ UID er ikke 4 bytes – ignoreres." << std::endl;
            }
            sleep(1);
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
}

int main() {
    startNFC();  // Du kan også køre dette i en tråd hvis du har en server loop
    return 0;
}
