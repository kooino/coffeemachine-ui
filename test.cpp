#include <nfc/nfc.h>
#include <iostream>
#include <iomanip>
#include <unistd.h>  // for sleep()

int main() {
    nfc_device *pnd = nullptr;
    nfc_context *context;

    nfc_init(&context);
    if (context == nullptr) {
        std::cerr << "Kunne ikke initialisere libnfc" << std::endl;
        return 1;
    }

    pnd = nfc_open(context, nullptr);  // libnfc.conf bruges til at finde enheden
    if (pnd == nullptr) {
        std::cerr << "Kunne ikke åbne NFC-enhed – tjek forbindelser og I2C-mode" << std::endl;
        nfc_exit(context);
        return 1;
    }

    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "Kunne ikke initialisere som initiator" << std::endl;
        nfc_close(pnd);
        nfc_exit(context);
        return 1;
    }

    std::cout << "✅ Klar. Hold et RFID/NFC-kort hen til læseren..." << std::endl;

    const nfc_modulation nmMifare = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_106
    };

    nfc_target nt;
    while (true) {
        if (nfc_initiator_select_passive_target(pnd, nmMifare, nullptr, 0, &nt) > 0) {
            std::cout << "📡 Kort fundet! UID: ";
            for (int i = 0; i < nt.nti.nai.szUidLen; ++i) {
                std::cout << std::hex << std::setfill('0') << std::setw(2)
                          << static_cast<int>(nt.nti.nai.abtUid[i]) << " ";
            }
            std::cout << std::dec << std::endl;
            sleep(1);
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
    return 0;
}
