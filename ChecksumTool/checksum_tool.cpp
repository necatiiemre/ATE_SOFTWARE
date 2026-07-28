// ============================================================================
// checksum_tool - Bagimsiz paket checksum / Q14 zaman kontrol araci
// ----------------------------------------------------------------------------
// Bir hex byte array verirsin; bu arac TUM ADIMLARI detayli yazarak:
//   1) Girdiyi parse eder ve baytlari 1-based numaralandirir
//   2) Data field'i (byte 5-68) tek tek toplar (her adimda calisan toplam)
//   3) Toplamin modulo 256'sini alir (carries ignored)
//   4) 2's complement ile checksum'i hesaplar
//   5) Pakette checksum bayti (byte 69) varsa dogrular (data+cksum == 0 mod 256)
//   6) Zaman alanini (byte 49-52, Q14 little-endian) HH:MM:SS'e cozer
//
// Bu app tamamen bagimsizdir; MainSoftware'e bagli degildir.
//
// Derleme:
//   g++ -std=c++17 -O2 -o checksum_tool checksum_tool.cpp
//   (veya) ./build.sh
//
// Kullanim:
//   ./checksum_tool CA E3 40 13 ... B9 07 EA        # arguman olarak
//   echo "CA E3 40 ... 07 EA" | ./checksum_tool     # stdin (pipe)
//   ./checksum_tool                                 # calistir, yapistir, Enter
//   ./checksum_tool --data 12 34 56                 # sadece verilen baytlari topla
//
// Girdi formati esnek: "CA E3 40", "0xCA,0xE3,0x40", "cae340" hepsi olur.
// ============================================================================

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// --- Paket duzeni (0-based offset'ler; ekranda 1-based byte no gosterilir) ---
// Checksum kurali her iki pakette de ayni: data field = byte 5 .. son
// checksum baytindan onceki byte; checksum = son byte.
static constexpr size_t CHECKSUM_BEGIN  = 4;     // data field baslangici, byte 5
static constexpr uint32_t Q14_SCALE     = 16384; // 2^14

// Zaman paketi (69 bayt)
static constexpr size_t TIME_PACKET_SIZE = 69;
static constexpr size_t TS_OFFSET        = 48;   // zaman alani, byte 49-52 (Q14 LE)

// Tarih paketi (55 bayt)
static constexpr size_t DATE_PACKET_SIZE = 55;
static constexpr size_t DATE_YEAR_OFFSET  = 28;  // byte 29-30 (LE)
static constexpr size_t DATE_MONTH_OFFSET = 30;  // byte 31-32 (LE)
static constexpr size_t DATE_DAY_OFFSET   = 32;  // byte 33-34 (LE)

static const char* LINE = "--------------------------------------------------------------";

// Esnek hex parse: bosluk/virgul ayirir, opsiyonel 0x atar, bitisik hex'i boler
static bool parseHex(const std::string& input, std::vector<uint8_t>& out, std::string& err)
{
    std::string norm = input;
    for (char& c : norm) {
        if (c == ',' || c == ';' || c == '\t' || c == '\n' || c == '\r') c = ' ';
    }
    std::istringstream iss(norm);
    std::string tok;
    while (iss >> tok) {
        if (tok.size() > 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X'))
            tok = tok.substr(2);
        if (tok.empty()) continue;
        if (tok.size() % 2 != 0) {
            err = "Tek haneli hex token: '" + tok + "' (her bayt 2 hane olmali)";
            return false;
        }
        for (size_t i = 0; i < tok.size(); i += 2) {
            std::string bs = tok.substr(i, 2);
            if (!std::isxdigit((unsigned char)bs[0]) || !std::isxdigit((unsigned char)bs[1])) {
                err = "Gecersiz hex: '" + bs + "'";
                return false;
            }
            out.push_back((uint8_t)std::stoul(bs, nullptr, 16));
        }
    }
    return true;
}

// Baytlari 1-based, offset etiketli grid halinde bas
static void dumpBytes(const std::vector<uint8_t>& b)
{
    for (size_t i = 0; i < b.size(); i++) {
        if (i % 8 == 0) printf("  byte %2zu: ", i + 1);   // 1-based
        printf("%02X ", b[i]);
        if ((i + 1) % 8 == 0 || i + 1 == b.size()) printf("\n");
    }
}

static void section(const char* title)
{
    printf("\n%s\n%s\n%s\n", LINE, title, LINE);
}

int main(int argc, char** argv)
{
    bool dataOnly = false;
    std::string input;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--data" || a == "-d") { dataOnly = true; continue; }
        if (a == "--help" || a == "-h") {
            printf("Kullanim: checksum_tool [--data] <hex baytlar...>\n");
            printf("  varsayilan : tam 69-bayt paket analizi (checksum + Q14 zaman)\n");
            printf("  --data     : verilen tum baytlari topla, 2's comp checksum ver\n");
            printf("  hex stdin'den de okunabilir (pipe ya da yapistir+Enter)\n");
            return 0;
        }
        input += a + " ";
    }
    if (input.empty()) {
        std::string line;
        std::getline(std::cin, line);
        input = line;
    }

    std::vector<uint8_t> b;
    std::string err;

    section("ADIM 0: GIRDIYI PARSE ET");
    if (!parseHex(input, b, err)) { fprintf(stderr, "HATA: %s\n", err.c_str()); return 1; }
    if (b.empty())                { fprintf(stderr, "HATA: hic bayt verilmedi.\n"); return 1; }
    printf("Toplam %zu bayt okundu:\n", b.size());
    dumpBytes(b);

    // ---------------- Genel amacli mod: tum baytlari topla ------------------
    if (dataOnly) {
        section("ADIM 1: TUM BAYTLARI TOPLA (--data modu)");
        uint32_t sum = 0;
        for (size_t i = 0; i < b.size(); i++) {
            uint32_t before = sum;
            sum += b[i];
            printf("  byte %2zu:  0x%02X (%3u)   toplam: %u + %u = %u (0x%X)\n",
                   i + 1, b[i], b[i], before, b[i], sum, sum);
        }
        section("ADIM 2: MODULO 256 (carries ignored)");
        printf("  toplam        = %u (0x%X)\n", sum, sum);
        printf("  toplam & 0xFF = 0x%02X\n", sum & 0xFF);
        section("ADIM 3: 2's COMPLEMENT");
        uint8_t s8 = sum & 0xFF;
        printf("  ~0x%02X        = 0x%02X\n", s8, (uint8_t)(~s8));
        printf("  + 1           = 0x%02X\n", (uint8_t)(~s8 + 1));
        printf("\n  >> CHECKSUM   = 0x%02X\n", (uint8_t)((~sum + 1) & 0xFF));
        return 0;
    }

    // ---------------- Tam paket modu ----------------------------------------
    // Checksum aralik/ofset paket boyutundan turetilir: data field byte 5'ten
    // son checksum baytindan onceye kadar; checksum son byte.
    const size_t N = b.size();
    const char* pktType =
        (N == TIME_PACKET_SIZE) ? "ZAMAN paketi (69 bayt)" :
        (N == DATE_PACKET_SIZE) ? "TARIH paketi (55 bayt)" : "BILINMEYEN boyut";
    const size_t CK_OFFSET = N - 1;   // son byte = checksum
    const size_t CK_END    = N - 1;   // data field bunun oncesine kadar

    if (N != TIME_PACKET_SIZE && N != DATE_PACKET_SIZE) {
        printf("\nUYARI: paket %zu bayt (beklenen 69=zaman ya da 55=tarih).\n", N);
        printf("       Yine de byte 5-%zu toplanacak (son byte checksum kabul edildi).\n", CK_END);
        printf("       Sadece bir data blogunu toplatmak istersen --data kullan.\n");
    } else {
        printf("\nPaket tipi: %s\n", pktType);
    }

    section("ADIM 1: HANGI BAYTLAR TOPLANIYOR?");
    printf("  DAHIL DEGIL : byte 1-4  (header)");
    for (size_t i = 0; i < 4 && i < N; i++) printf(" %02X", b[i]);
    printf("\n");
    printf("  DAHIL DEGIL : byte %zu   (checksum baytinin kendisi) -> %02X\n",
           CK_OFFSET + 1, b[CK_OFFSET]);
    printf("  TOPLANAN    : byte 5-%zu (data field)\n", CK_END);

    char step2[80];
    snprintf(step2, sizeof(step2), "ADIM 2: DATA FIELD'I TEK TEK TOPLA (byte 5-%zu)", CK_END);
    section(step2);
    uint32_t sum = 0;
    for (size_t i = CHECKSUM_BEGIN; i < CK_END && i < N; i++) {
        uint32_t before = sum;
        sum += b[i];
        printf("  byte %2zu:  0x%02X (%3u)   toplam: %u + %u = %u (0x%X)\n",
               i + 1, b[i], b[i], before, b[i], sum, sum);
    }

    section("ADIM 3: MODULO 256 (carries ignored)");
    printf("  toplam        = %u (0x%X)\n", sum, sum);
    printf("  toplam & 0xFF = 0x%02X   (dusuk 8 bit)\n", sum & 0xFF);

    section("ADIM 4: 2's COMPLEMENT -> CHECKSUM");
    uint8_t s8 = sum & 0xFF;
    uint8_t calc = (uint8_t)((~sum + 1) & 0xFF);
    printf("  ~0x%02X        = 0x%02X\n", s8, (uint8_t)(~s8));
    printf("  + 1           = 0x%02X\n", (uint8_t)(~s8 + 1));
    printf("\n  >> HESAPLANAN CHECKSUM = 0x%02X\n", calc);

    {
        section("ADIM 5: PAKETTEKI CHECKSUM ILE KARSILASTIR");
        uint8_t inPkt = b[CK_OFFSET];
        printf("  Pakette (byte %zu): 0x%02X\n", CK_OFFSET + 1, inPkt);
        printf("  Hesaplanan       : 0x%02X\n", calc);
        printf("  Sonuc            : %s\n", (inPkt == calc) ? ">>> [OK] eslesti" : ">>> [MISMATCH] eslesmedi");
        printf("\n  Dogrulama testi (data field + checksum):\n");
        uint32_t chk = (sum + inPkt) & 0xFF;
        printf("    (0x%02X + 0x%02X) & 0xFF = 0x%02X   -> %s\n",
               s8, inPkt, chk, (chk == 0) ? "0x00 (mesaj gecerli)" : "!= 0 (mesaj HATALI)");
    }

    // ADIM 6: alan cozumu paket tipine gore
    if (N == TIME_PACKET_SIZE) {
        section("ADIM 6: ZAMAN ALANINI COZ (byte 49-52, Q14 little-endian)");
        uint8_t t0 = b[TS_OFFSET+0], t1 = b[TS_OFFSET+1], t2 = b[TS_OFFSET+2], t3 = b[TS_OFFSET+3];
        printf("  Ham baytlar (49-52) : %02X %02X %02X %02X\n", t0, t1, t2, t3);
        printf("  Little-endian birlestir (LSB once):\n");
        printf("    0x%02X | (0x%02X<<8) | (0x%02X<<16) | (0x%02X<<24)\n", t0, t1, t2, t3);
        uint32_t q14 = (uint32_t)t0 | ((uint32_t)t1<<8) | ((uint32_t)t2<<16) | ((uint32_t)t3<<24);
        printf("    = %u (0x%08X)   [Q14 integer]\n", q14, q14);
        uint32_t sod = q14 / Q14_SCALE;
        uint32_t rem = q14 % Q14_SCALE;
        printf("  Q14 -> saniye: %u / 16384 = %u  (kalan %u)\n", q14, sod, rem);
        printf("  Gece yarisindan gecen saniye: %u\n", sod);
        printf("\n  >> ZAMAN = %02u:%02u:%02u\n", sod/3600, (sod%3600)/60, sod%60);
        if (rem != 0)
            printf("  UYARI: kesir kismi != 0 (%u/16384) - tam saniye bekleniyordu\n", rem);
    } else if (N == DATE_PACKET_SIZE) {
        section("ADIM 6: TARIH ALANINI COZ (byte 29-34, little-endian 16-bit)");
        uint32_t year  = (uint32_t)b[DATE_YEAR_OFFSET]  | ((uint32_t)b[DATE_YEAR_OFFSET+1]  << 8);
        uint32_t month = (uint32_t)b[DATE_MONTH_OFFSET] | ((uint32_t)b[DATE_MONTH_OFFSET+1] << 8);
        uint32_t day   = (uint32_t)b[DATE_DAY_OFFSET]   | ((uint32_t)b[DATE_DAY_OFFSET+1]   << 8);
        printf("  Yil   (byte 29-30): %02X %02X -> %u\n", b[DATE_YEAR_OFFSET],  b[DATE_YEAR_OFFSET+1],  year);
        printf("  Ay    (byte 31-32): %02X %02X -> %u\n", b[DATE_MONTH_OFFSET], b[DATE_MONTH_OFFSET+1], month);
        printf("  Gun   (byte 33-34): %02X %02X -> %u\n", b[DATE_DAY_OFFSET],   b[DATE_DAY_OFFSET+1],   day);
        printf("\n  >> TARIH = %04u/%02u/%02u\n", year, month, day);
    }

    printf("\n%s\n", LINE);
    return 0;
}
