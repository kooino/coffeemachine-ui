#include <nfc/nfc.h>
#include <iostream>
#include <iomanip>

int main() {
    nfc_device *pnd = nullptr;
    nfc_context *context;

    nfc_init(&context);
    if (context == nullptr) {
        std::cerr << "Kunne ikke initialisere libnfc" << std::endl;
        return 1;
    }

    pnd = nfc_open(context, nullptr);
    if (pnd == nullptr) {
        std::cerr << "Kunne ikke åbne NFC enhed" << std::endl;
        nfc_exit(context);
        return 1;
    }

    if (nfc_initiator_init(pnd) < 0) {
        nfc_close(pnd);
        nfc_exit(context);
        std::cerr << "Kunne ikke initialisere som initiator" << std::endl;
        return 1;
    }

    std::cout << "Venter på RFID/NFC kort..." << std::endl;

    nfc_target nt;
    while (true) {
        if (nfc_initiator_select_passive_target(pnd, nmMifare, nullptr, 0, &nt) > 0) {
            std::cout << "Kort fundet! UID: ";
            for (int i = 0; i < nt.nti.nai.szUidLen; i++) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << (int)nt.nti.nai.abtUid[i] << " ";
            }
            std::cout << std::endl;

            // Vent lidt før næste læsning
            sleep(1);
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
    return 0;
}
