#include <nfc/nfc.h>
#include <iostream>
#include <iomanip>
#include <unistd.h>  // For sleep()

int main() {
    nfc_context *context;
    nfc_device *pnd;

    // Initialiser libnfc
    nfc_init(&context);
    if (context == nullptr) {
        std::cerr << "Kunne ikke initialisere libnfc" << std::endl;
        return 1;
    }

    // Åbn enhed (brug konfiguration fra /etc/nfc/libnfc.conf)
    pnd = nfc_open(context, nullptr);
    if (pnd == nullptr) {
        std::cerr << "Kunne ikke åbne NFC-enhed" << std::endl;
        nfc_exit(context);
        return 1;
    }

    // Sæt enheden i initiator-mode (aktiv læser)
    if (nfc_initiator_init(pnd) < 0) {
        std::cerr << "Fejl ved initialisering" << std::endl;
        nfc_close(pnd);
        nfc_exit(context);
        return 1;
    }

    std::cout << "✅ Klar. Hold et RFID/NFC-kort op til læseren..." << std::endl;

    // Angiv protokol (MIFARE-type ISO14443A)
    const nfc_modulation nmMifare = {
        .nmt = NMT_ISO14443A,
        .nbr = NBR_106
    };

    nfc_target nt;

    while (true) {
        // Vent på kort
        if (nfc_initiator_select_passive_target(pnd, nmMifare, nullptr, 0, &nt) > 0) {
            std::cout << "📡 Kort fundet! UID: ";
            for (int i = 0; i < nt.nti.nai.szUidLen; i++) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << (int)nt.nti.nai.abtUid[i] << " ";
            }
            std::cout << std::dec << std::endl;
            sleep(1);  // Vent 1 sekund før næste læsning
        }
    }

    // Luk enhed
    nfc_close(pnd);
    nfc_exit(context);
    return 0;
}
