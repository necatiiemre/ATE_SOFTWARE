# checksum_tool

SerialTimeForwarder paketlerini elle kontrol etmek için mini CLI aracı.
Bir hex byte array verirsin; **data field checksum**'ını (byte 5-68 toplamının
2's complement'i) hesaplar, pakette checksum baytı (byte 69) varsa karşılaştırır
ve **Q14 zaman alanını** (byte 49-52, little-endian) `HH:MM:SS` olarak çözer.

Aritmetik, `src/SerialTimeForwarder.cpp` içindeki gerçek kodun birebir aynısıdır.

## Derleme

```bash
g++ -std=c++17 -O2 -o checksum_tool checksum_tool.cpp
```

## Kullanım

```bash
# Tam paket (69 bayt): checksum + zaman analizi
./checksum_tool CA E3 40 13 ... B9 07 EA

# stdin'den (pipe ya da çalıştırıp yapıştır + Enter)
echo "CA E3 40 ... 07 EA" | ./checksum_tool

# Sadece verilen baytları topla (genel amaçlı sum + 2's complement)
./checksum_tool --data 12 34 56        # -> 0x64
```

Girdi formatı esnek: `CA E3 40`, `0xCA,0xE3,0x40`, `cae340` — hepsi kabul edilir.

## Checksum kuralı

```
sum      = (byte[5..68] toplamı) & 0xFF     # ilk 4 header + son checksum baytı hariç
checksum = (-sum) & 0xFF = (~sum + 1) & 0xFF
```

Doğrulama: `data field + checksum == 0 (mod 256)`.

## Örnek doğrulanmış değerler

| Saat | Zaman alanı (byte 49-52) | Checksum (byte 69) |
|---|---|---|
| 00:00:00 | `00 00 00 00` | `EA` |
| 01:00:00 | `00 00 84 03` | `63` |
| 21:00:00 | `00 00 D4 49` | `CD` |
