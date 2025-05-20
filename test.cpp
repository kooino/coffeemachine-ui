#include <nfc/nfc.h>
#include <iostream>
#include <iomanip>

int main() {
    nfc_context *context;
    nfc_device *pnd;
    nfc_target nt;

    // Init libnfc
    nfc_init(&context);
    if (context == nullptr) {
        std::cerr << "Unable to init libnfc" << std::endl;
        return 1;
    }

    // Åbn den første tilgængelige NFC enhed
    pnd = nfc_open(context, nullptr);
    if (pnd == nullptr) {
        std::cerr << "ERROR: Unable to open NFC device." << std::endl;
        nfc_exit(context);
        return 1;
    }

    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "ERROR: Unable to initialize NFC device as initiator." << std::endl;
        nfc_close(pnd);
        nfc_exit(context);
        return 1;
    }

    std::cout << "NFC reader: " << nfc_device_get_name(pnd) << " initialized." << std::endl;
    std::cout << "Bring a tag close to the reader..." << std::endl;

    // Poll for a ISO14443A (MIFARE) tag
    const nfc_modulation nmModulations[1] = {
        { NMT_ISO14443A, NBR_106 }
    };

    while (true) {
        int res = nfc_initiator_poll_target(pnd, nmModulations, 1, 10, 2, &nt);
        if (res < 0) {
            std::cerr << "Error polling for target." << std::endl;
            break;
        }

        if (res > 0) {
            // Print UID
            std::cout << "Tag found, UID: ";
            for (size_t i = 0; i < nt.nti.nai.szUidLen; i++) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << (int)nt.nti.nai.abtUid[i] << " ";
            }
            std::cout << std::dec << std::endl;

            // Vent indtil tag fjernes for at scanne næste
            std::cout << "Fjern kortet for at scanne næste..." << std::endl;
            while (nfc_initiator_target_is_present(pnd, nullptr) == 0) {
                // vent
            }
            std::cout << "Klar til næste kort." << std::endl;
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
    return 0;
}
