#include <nfc/nfc.h>
#include <iostream>
#include <iomanip>
#include <unistd.h>  // for sleep()

int main() {
    nfc_context *context;
    nfc_device *pnd;

    // Initialisér NFC-biblioteket
    nfc_init(&context);
    if (context == nullptr) {
        std::cerr << "Kunne ikke initialisere libnfc" << std::endl;
        return 1;
    }

    // Åbn NFC-enheden (via libnfc.conf)
    pnd = nfc_open(context, nullptr);
    if (pnd == nullptr) {
        std::cerr << "Kunne ikke åbne NFC-enhed" << std::endl;
        nfc_exit(context);
        return 1;
    }

    // Initialiser som initiator (læser)
    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "Fejl ved initiering som initiator" << std::endl;
        nfc_close(pnd);
        nfc_exit(context);
        return 1;
    }

    std::cout << "🔄 Venter på RFID-kort..." << std::endl;

    const nfc_modulation nmMifare = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_106
    };

    nfc_target nt;

    while (true) {
        if (nfc_initiator_select_passive_target(pnd, nmMifare, nullptr, 0, &nt) > 0) {
            std::cout << "✅ Kort fundet!" << std::endl;

            // HEX-format
            std::cout << "HEX: ";
            for (int i = 0; i < nt.nti.nai.szUidLen; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(nt.nti.nai.abtUid[i]) << " ";
            }
            std::cout << std::endl;

            // DEC byte for byte
            std::cout << "DEC (bytes): ";
            for (int i = 0; i < nt.nti.nai.szUidLen; ++i) {
                std::cout << std::dec << static_cast<int>(nt.nti.nai.abtUid[i]);
                if (i < nt.nti.nai.szUidLen - 1)
                    std::cout << "-";
            }
            std::cout << std::endl;

            // UID som samlet heltal (hvis det er 4 bytes)
            if (nt.nti.nai.szUidLen == 4) {
                uint32_t uid_int = 0;
                uid_int |= (nt.nti.nai.abtUid[0] << 24);
                uid_int |= (nt.nti.nai.abtUid[1] << 16);
                uid_int |= (nt.nti.nai.abtUid[2] << 8);
                uid_int |= nt.nti.nai.abtUid[3];
                std::cout << "DEC (samlet int): " << uid_int << std::endl;
            }

            std::cout << std::endl;
            sleep(1);  // Vent før næste læsning
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
    return 0;
}
