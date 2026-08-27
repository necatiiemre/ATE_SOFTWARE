#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Struct tanimi
// Not: Bit alanlarinin yerlesimi derleyiciye baglidir.
// Çogu modern derleyicide (GCC/Clang) bu yapi birlestirilerek tek bir byte olusturur.
typedef struct {
    uint8_t val5 : 5; // 5 bit
    uint8_t val3 : 3; // 3 bit
} BitFieldStruct;

// Byte sirasini tersine çeviren fonksiyon (Endian simülasyonu için)
uint8_t swap_endian(uint8_t b) {
    // Tek bir byte için endian dönüsümü (swap) aslinda deger degismez 
    // ama mantigi anlamak için byte düzeyinde islem yapiyoruz.
    // Eger struct 16-bit veya 32-bit olsaydi fark çok daha belirgin olurdu.
    return b; 
}

// Eger struct daha büyük olsaydi (örnegin 16 bit), gerçek bir swap islemi gerekirdi:
uint16_t swap_uint16(uint16_t val) {
    return ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
}

int main() {
    uint8_t x = 0b00110000;

    BitFieldStruct xStruct = {0};
    // xStruct.val3 = 0b000;
    // xStruct.val5 = 0b000001;
    memcpy(&xStruct, &x, sizeof(uint8_t));
    printf("x (8 bit): %08X (Binary: 000)\n", x);
    printf("xStruct (8 bit): %08X (Binary: 000)\n", xStruct);
    printf("val3 (3 bit): %03X (Binary: 001)\n", xStruct.val3);
    printf("val5 (5 bit): %05X (Binary: 10000)\n", xStruct.val5);
    //printf("Bellekteki Ham Byte (Hex): 0x%02X\n", buffer[0]);

    // 1. Senaryo: Deger atama
    BitFieldStruct myStruct;
    myStruct.val3 = 0b101; // 5 (Decimal)
    myStruct.val5 = 0b11011; // 27 (Decimal)`


    // Bellekteki ham görüntüyü görmek için bir buffer olusturalim
    printf("val3 (3 bit): %03X (Binary: 001)\n", xStruct.val3);
    printf("val5 (5 bit): %05X (Binary: 10000)\n", xStruct.val5);

    printf("--- Veri Hazirligi ---\n");
    printf("val3 (3 bit): %03X (Binary: 101)\n", myStruct.val3);
    printf("val5 (5 bit): %05X (Binary: 11011)\n", myStruct.val5);
    printf("Bellekteki Ham Byte (Hex): 0x%02X\n", buffer[0]);

    // Bitlerin yerlesimi analizi
    // Little-endian sistemlerde bitler genellikle LSB'den baslar:
    // [val5 (5 bit)] [val3 (3 bit)]
    // 11011 | 101  => 11011101 (Binary) => 0xDD (Hex)
    
    printf("\n--- Bit Yerlesim Analizi ---\n");
    printf("Eger Little-Endian bit dizilimi varsa:\n");
    printf("Bit dizilimi: [val5: 11011][val3: 101] -> 11011101 (0xDD)\n");

    // 2. Senaryo: Memcpy ve Endian Farkliligi Simülasyonu
    // Not: Tek bir byte (8 bit) söz konusu oldugunda, byte'in kendisi 
    // endian dönüsümünden etkilenmez. Endianness "byte sirasi" ile ilgilidir.
    // Ancak struct 2 byte (16 bit) olsaydi durum degisirdi.
    
    printf("\n--- Endianness Etkisi ---\n");
    if (sizeof(BitFieldStruct) == 1) {
        printf("UYARI: Struct boyutu 1 byte. Tek byte için Endian dönüsümü (byte swap) sonucu degistirmez.\n");
        printf("Ancak bitlerin iç siralamasi (Bit-Endianness) derleyiciye göre degisir.\n");
    } else {
        printf("Struct boyutu %zu byte. Endian dönüsümü byte sirasini degistirecektir.\n", sizeof(BitFieldStruct));
    }

    // 3. Senaryo: Eger struct 16 bit olsaydi (Gerçek fark burada görülür)
    typedef struct {
        uint16_t val3 : 3;
        uint16_t val5 : 5;
        uint16_t padding : 8; 
    } WideBitFieldStruct;

    WideBitFieldStruct wideStruct;
    wideStruct.val3 = 0b101;
    wideStruct.val5 = 0b11011;
    wideStruct.padding = 0;

    uint16_t wideBuffer;
    memcpy(&wideBuffer, &wideStruct, sizeof(WideBitFieldStruct));

    printf("\n--- Genisletilmis (16-bit) Senaryo ---\n");
    printf("Orijinal 16-bit Deger (Hex): 0x%04X\n", wideBuffer);
    
    uint16_t swappedWide = swap_uint16(wideBuffer);
    printf("Endian Swap Sonrasi (Hex):   0x%04X\n", swappedWide);
    printf("Gözlem: Byte'lar yer degistirdigi için bitlerin konumu tamamen degisti!\n");

    return 0;
}