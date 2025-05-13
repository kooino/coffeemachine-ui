#include <nfc/nfc.h>
#include <iostream>
#include <iomanip>

int main() {
    nfc_context* context;
    nfc_device* pnd;
    nfc_target nt;

    nfc_init(&context);
    if (context == nullptr) {
        std::cerr << "❌ Kan ikke initialisere libnfc!" << std::endl;
        return EXIT_FAILURE;
    }

    pnd = nfc_open(context, NULL);
    if (pnd == nullptr) {
        std::cerr << "❌ Kan ikke åbne NFC-enhed." << std::endl;
        nfc_exit(context);
        return EXIT_FAILURE;
    }

    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "❌ Kan ikke sætte NFC i initiator mode." << std::endl;
        nfc_close(pnd);
        nfc_exit(context);
        return EXIT_FAILURE;
    }

    std::cout << "✅ NFC-læser klar. Vent på kort..." << std::endl;

    const nfc_modulation nm = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_106,
    };

    while (true) {
        if (nfc_initiator_select_passive_target(pnd, nm, nullptr, 0, &nt) > 0) {
            std::cout << "✅ Kort fundet! UID: ";
            for (size_t i = 0; i < nt.nti.nai.szUidLen; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(nt.nti.nai.abtUid[i]);
            }
            std::cout << std::dec << std::endl;

            // Konverter til decimal UID hvis ønsket:
            uint32_t uid_decimal = 0;
            for (size_t i = 0; i < nt.nti.nai.szUidLen; ++i) {
                uid_decimal <<= 8;
                uid_decimal |= nt.nti.nai.abtUid[i];
            }
            std::cout << "🔢 UID (decimal): " << uid_decimal << std::endl;

            break;  // Fjern denne hvis du vil fortsætte med at læse flere kort
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
    return 0;
}
