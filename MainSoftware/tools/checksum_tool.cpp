// ============================================================================
// checksum_tool - SerialTimeForwarder paket checksum / Q14 zaman kontrol aracı
// ----------------------------------------------------------------------------
// Bir hex byte array verirsin, bu araç:
//   * data field'ı (byte 5-68) toplar, 2's complement checksum'ı hesaplar
//   * pakette checksum baytı (byte 69) varsa karşılaştırır (OK / MISMATCH)
//   * zaman alanını (byte 49-52, Q14 little-endian) HH:MM:SS olarak çözer
//
// Bu, MainSoftware/src/SerialTimeForwarder.cpp içindeki mantığın birebir aynısı.
//
// Derleme:
//   g++ -std=c++17 -O2 -o checksum_tool checksum_tool.cpp
//
// Kullanım:
//   ./checksum_tool CA E3 40 13 ... B9 07 EA        # argüman olarak
//   echo "CA E3 40 ... 07 EA" | ./checksum_tool     # stdin'den (pipe)
//   ./checksum_tool                                 # çalıştır, satırı yapıştır, Enter
//
//   ./checksum_tool --data 12 34 56                 # SADECE verilen baytları topla
//                                                   # (genel amaçlı: sum + 2's comp)
//
// Girdi formatı esnek:  "CA E3 40",  "0xCA,0xE3,0x40",  "cae340"  hepsi olur.
// ============================================================================

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// --- Paket düzeni (0-based offset'ler; yorumdaki byte no'lar 1-based) --------
static constexpr size_t PACKET_SIZE       = 69;  // tam sistem mesajı
static constexpr size_t TS_OFFSET         = 48;  // zaman alanı, byte 49-52 (Q14 LE)
static constexpr size_t CHECKSUM_OFFSET   = 68;  // checksum, byte 69
static constexpr size_t CHECKSUM_BEGIN    = 4;   // data field başlangıcı, byte 5
static constexpr size_t CHECKSUM_END      = 68;  // exclusive: byte 5-68 toplanır
static constexpr uint32_t Q14_SCALE       = 16384; // 2^14

// data field toplamı üzerinden 2's complement checksum
static uint8_t computeChecksum(const std::vector<uint8_t>& b, size_t begin, size_t end)
{
    uint32_t sum = 0;
    for (size_t i = begin; i < end && i < b.size(); i++) {
        sum += b[i];
    }
    return static_cast<uint8_t>((~sum + 1) & 0xFF);
}

// Esnek hex parse: boşluk/virgül ayırır, opsiyonel 0x atar, bitişik hex'i böler
static bool parseHex(const std::string& input, std::vector<uint8_t>& out, std::string& err)
{
    std::string norm = input;
    for (char& c : norm) {
        if (c == ',' || c == ';' || c == '\t' || c == '\n' || c == '\r') c = ' ';
    }

    std::istringstream iss(norm);
    std::string tok;
    while (iss >> tok) {
        // 0x / 0X önekini at
        if (tok.size() > 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X')) {
            tok = tok.substr(2);
        }
        if (tok.empty()) continue;

        if (tok.size() % 2 != 0) {
            err = "Tek haneli hex token: '" + tok + "' (her bayt 2 hane olmalı)";
            return false;
        }
        for (size_t i = 0; i < tok.size(); i += 2) {
            std::string byteStr = tok.substr(i, 2);
            if (!std::isxdigit(static_cast<unsigned char>(byteStr[0])) ||
                !std::isxdigit(static_cast<unsigned char>(byteStr[1]))) {
                err = "Geçersiz hex: '" + byteStr + "'";
                return false;
            }
            out.push_back(static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16)));
        }
    }
    return true;
}

static void printHex(const std::vector<uint8_t>& b, size_t hi)
{
    for (size_t i = 0; i < b.size(); i++) {
        printf("%02X ", b[i]);
        if ((i + 1) % 10 == 0) printf("\n");
    }
    if (b.size() % 10 != 0) printf("\n");
    (void)hi;
}

int main(int argc, char** argv)
{
    bool dataOnly = false;   // --data: sadece verilen baytları topla (genel amaçlı)
    std::string input;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--data" || a == "-d") { dataOnly = true; continue; }
        if (a == "--help" || a == "-h") {
            printf("Kullanim: checksum_tool [--data] <hex baytlar...>\n");
            printf("  varsayilan : tam 69-bayt paket analizi (checksum + Q14 zaman)\n");
            printf("  --data     : verilen tum baytlari topla, 2's comp checksum ver\n");
            printf("  hex de stdin'den okunabilir (pipe veya yapistir+Enter)\n");
            return 0;
        }
        input += a + " ";
    }

    // Argüman yoksa stdin'den oku
    if (input.empty()) {
        std::string line;
        std::getline(std::cin, line);
        input = line;
    }

    std::vector<uint8_t> bytes;
    std::string err;
    if (!parseHex(input, bytes, err)) {
        fprintf(stderr, "HATA: %s\n", err.c_str());
        return 1;
    }
    if (bytes.empty()) {
        fprintf(stderr, "HATA: hic bayt verilmedi.\n");
        return 1;
    }

    printf("Girdi: %zu bayt\n", bytes.size());
    printHex(bytes, 0);
    printf("\n");

    // --- Genel amaçlı mod: tüm baytları topla -------------------------------
    if (dataOnly) {
        uint32_t sum = 0;
        for (uint8_t v : bytes) sum += v;
        uint8_t ck = static_cast<uint8_t>((~sum + 1) & 0xFF);
        printf("[--data modu] Tum baytlar toplandi\n");
        printf("  Toplam        : %u (0x%X)\n", sum, sum);
        printf("  Toplam & 0xFF : 0x%02X\n", sum & 0xFF);
        printf("  Checksum      : 0x%02X   (2's complement)\n", ck);
        return 0;
    }

    // --- Tam paket modu -----------------------------------------------------
    if (bytes.size() != PACKET_SIZE) {
        printf("UYARI: paket %zu bayt (beklenen %zu). Yine de byte 5-68 uzerinden hesaplanacak.\n",
               bytes.size(), PACKET_SIZE);
        printf("       (Sadece bir data blogunu toplatmak istiyorsan --data kullan.)\n\n");
    }

    // Checksum
    uint32_t sum = 0;
    for (size_t i = CHECKSUM_BEGIN; i < CHECKSUM_END && i < bytes.size(); i++) sum += bytes[i];
    uint8_t calc = computeChecksum(bytes, CHECKSUM_BEGIN, CHECKSUM_END);

    printf("Checksum (data field = byte 5-68):\n");
    printf("  Toplam        : %u (0x%X)\n", sum, sum);
    printf("  Toplam & 0xFF : 0x%02X\n", sum & 0xFF);
    printf("  Hesaplanan    : 0x%02X   (2's complement)\n", calc);

    if (bytes.size() > CHECKSUM_OFFSET) {
        uint8_t inPacket = bytes[CHECKSUM_OFFSET];
        printf("  Pakette (byte 69): 0x%02X   -> %s\n",
               inPacket, (inPacket == calc) ? "[OK]" : "[MISMATCH]");
    }

    // Q14 zaman alanı (byte 49-52, little-endian)
    if (bytes.size() >= TS_OFFSET + 4) {
        uint32_t q14 = static_cast<uint32_t>(bytes[TS_OFFSET + 0]) |
                       (static_cast<uint32_t>(bytes[TS_OFFSET + 1]) << 8) |
                       (static_cast<uint32_t>(bytes[TS_OFFSET + 2]) << 16) |
                       (static_cast<uint32_t>(bytes[TS_OFFSET + 3]) << 24);
        uint32_t sod = q14 / Q14_SCALE;      // saniye (gece yarisindan)
        uint32_t rem = q14 % Q14_SCALE;      // kesir kismi (SyncServer'da 0 olmali)
        printf("\nZaman alani (byte 49-52, Q14 little-endian):\n");
        printf("  Ham (LE)      : %02X %02X %02X %02X\n",
               bytes[TS_OFFSET+0], bytes[TS_OFFSET+1], bytes[TS_OFFSET+2], bytes[TS_OFFSET+3]);
        printf("  Q14 integer   : %u (0x%08X)\n", q14, q14);
        printf("  Saniye        : %u  (gece yarisindan)\n", sod);
        printf("  Saat          : %02u:%02u:%02u\n", sod / 3600, (sod % 3600) / 60, sod % 60);
        if (rem != 0) printf("  UYARI: kesir kismi != 0 (%u/16384) — tam saniye bekleniyordu\n", rem);
    }

    return 0;
}
