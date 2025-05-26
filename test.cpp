#include <nfc/nfc.h>
#include <iostream>
#include <iomanip>

int main() {
    nfc_context *context;
    nfc_device *pnd;
    nfc_target nt;

    nfc_init(&context);
    if (context == nullptr) {
        std::cerr << "Libnfc init fejl" << std::endl;
        return 1;
    }

    pnd = nfc_open(context, nullptr);
    if (pnd == nullptr) {
        std::cerr << "Ingen NFC-enhed fundet" << std::endl;
        nfc_exit(context);
        return 1;
    }

    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "Init fejl" << std::endl;
        nfc_close(pnd);
        nfc_exit(context);
        return 1;
    }

    std::cout << "Scan kort med PN532..." << std::endl;

    const nfc_modulation mod = { NMT_ISO14443A, NBR_106 };

    while (true) {
        if (nfc_initiator_poll_target(pnd, &mod, 1, 2, 2, &nt) > 0) {
            std::cout << "UID: ";
            for (size_t i = 0; i < nt.nti.nai.szUidLen; i++) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << (int)nt.nti.nai.abtUid[i] << " ";
            }
            std::cout << std::endl;
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
    return 0;
}
