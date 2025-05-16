#include <nfc/nfc.h>
#include <iostream>
#include <iomanip>
#include <unistd.h>

int main() {
    nfc_context* context;
    nfc_device* pnd;

    nfc_init(&context);
    if (!context) {
        std::cerr << "❌ Kan ikke initialisere libnfc\n";
        return 1;
    }

    pnd = nfc_open(context, "pn532_spi:/dev/spidev0.0");
    if (!pnd) {
        std::cerr << "❌ Kan ikke åbne NFC-enhed\n";
        nfc_exit(context);
        return 1;
    }

    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "❌ Kan ikke initialisere som initiator\n";
        nfc_close(pnd);
        nfc_exit(context);
        return 1;
    }

    std::cout << "✅ Venter på kort...\n";

    const nfc_modulation nm = { NMT_ISO14443A, NBR_106 };
    nfc_target nt;

    while (true) {
        if (nfc_initiator_select_passive_target(pnd, nm, nullptr, 0, &nt) > 0) {
            std::cout << "✅ Kort fundet! UID: ";
            for (int i = 0; i < nt.nti.nai.szUidLen; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
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
