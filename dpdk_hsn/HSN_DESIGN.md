# HSN Test — Tasarım Notu (çalışma belgesi)

DTN uygulamasından türetilen HSN varyantının tasarım kararları.
Durum etiketleri: **[KESIN]** karar verildi · **[TEYIT]** doğrulanacak · **[BEKLE]** bilgi bekliyor.

## 1. Topoloji

- **Server:** 12× ConnectX-6 Dx 100G (DPDK Port 0–11). PCI: 21/41/64/81/a1/e1 (×.0/.1).
  NUMA: P0-1=2, P2-3=0, P4-5=1, P6-7=7, P8-9=6, P10-11=5. **[KESIN]**
- **Switch:** Mellanox Spectrum (32 kafes).
  - Server tarafı: **swp13–swp24** (12× tekli 100G)
  - Cihaz 100G: **swp1–swp4** (4× tekli 100G)
  - Cihaz 10G: **swp25–swp31** (7 kafes × 4×10G breakout = 28, 26 kullanılır) **[KESIN]**
  - Breakout ayrıca `/etc/cumulus/ports.conf`'ta tanımlanmalı (25..31 = 4x10G). **[KESIN]**
- **Cihaz:** 4×100G (D0–D3) + 26×10G (E0–E25) = 30 port. Çift yönlü, **split TX/RX** (DTN mantığı). **[KESIN]**

## 2. Paket yapısı (HSN — DTN'den farklı)

Cihazın gördüğü frame: `[DST MAC 6B][SRC MAC 6B][payload]` — **IP/UDP yok, anlamlı EtherType yok.** **[KESIN]**
- **DST MAC** = `03:00:00:00:VV:VV` → VV:VV = 16-bit VL-ID (AFDX standardı). **[TEYIT]**
- **SRC MAC** = `02:00:00:00:00:PP` → PP = gönderen server portu.
- **VLAN tag** (`0x8100`) yalnızca **server→switch** segmentinde (switch routing için); switch cihaza çıkarırken söker (egress untagged). **[KESIN]**
- **Payload** = `[Sequence 8B][PRBS-31 data ...]`. **[KESIN]**
  - Boyut değişken; maks ≤ **9000B** (jumbo). MTU: switch 9216, NIC ~9216. **[KESIN]**
  - **TX boyut deseni (KESIN):** 128–4096 (2'nin katları) **eşit-bit**, 5000–9000 jumbo 1'er.
    | payload | 128 | 256 | 512 | 1024 | 2048 | 4096 | 5000 | 6000 | 7000 | 8000 | 9000 |
    |---|---|---|---|---|---|---|---|---|---|---|---|
    | adet | 256 | 128 | 64 | 32 | 16 | 8 | 1 | 1 | 1 | 1 | 1 |

    128–4096 her biri 32768 payload byte (=eşit %bit ≈14.1). Döngü = **509 paket**.
    **Model:** worker (port/queue) başına sürekli tekrar; boyut VL'den bağımsız ilerler (DTN IMIX mantığı). **[KESIN]**
- **Kod etkisi:** `build_packet_mbuf`'tan IP+UDP çıkar; payload offset = L2 sonrası; checksum yok; RX parse IP/UDP atlamayı bırakır; mbuf buffer jumbo'ya büyür; yeni (boyut→adet) pattern üreteci.

## 3. VLAN planı

- RX VLAN (server→cihaz): **100–129** (cihaz portu başına 1)
- TX VLAN (cihaz→server): **200–229** (cihaz portu başına 1)
- Cihaz-tarafı swp: `bridge-vids <RX_VLAN> untagged` + `bridge-pvid <TX_VLAN>`. **[KESIN]**
- Server-tarafı swp (swp13–24): hangi server portu hangi VLAN'ları taşır → port haritasından. RX yönü [KESIN], TX yönü **[BEKLE]** (cihaz forward haritası).

## 4. VL-ID planı

- Aralık limiti: **3–4095**. Toplam **2048 VL**, yön = **TX (server→cihaz)** benzersiz sanal linkler. **[KESIN]**
- Dağılım: **100G portu = 304 VL**, **10G portu = 32 VL** → 4×304 + 26×32 = **2048** (tam). Kullanılan VL-ID aralığı **3–2050**. **[KESIN]**
  - Cihaz portu başına ardışık blok: D0=3–306, D1=307–610, D2=611–914, D3=915–1218, E0=1219–1250, … E25=2019–2050.
- RX yönü (cihaz→server): DTN gibi ortak taban aralığı, VLAN'la kapsamlanır. **[KESIN]**
- VL-ID pakette: DST MAC son 2 byte. Her VL'nin kendi sequence sayacı.
- **VL hızı: hız limiti YOK — FULL BURST.** Her port line-rate'te VL'lerini round-robin süpürür; rate-limiter kullanılmayacak (AFDX BAG yok). **[KESIN]**

## 5. Split TX/RX eşleştirmesi

**Kural:** Bir server portundan gönderilen VL, aynı porttan geri ALINMAMALI (TX srv ≠ RX srv). Tüm 12 port doğrulandı ✅.

- **Cihaz davranışı: çapraz-eş loopback.** Cihaz portları fiziksel çapraz-kablolu çiftler; bir porta giren VL çapraz-eşinden çıkar (girdiği porttan dönmez). 100G↔100G, 10G↔10G:
  - 100G çiftleri: **D0↔D1**, **D2↔D3**
  - 10G çiftleri: **E0↔E1, E2↔E3, … E24↔E25** (13 bitişik çift)
- **Cihaz çifti → server çifti** (aynı NIC/NUMA, çapraz split):

  | Cihaz çifti | Server çifti | NUMA |
  |---|---|---|
  | D0↔D1 | P0↔P1 | 2 |
  | D2↔D3 | P2↔P3 | 0 |
  | E0↔E1, E2↔E3, E4↔E5, E6↔E7 | P4↔P5 | 1 |
  | E8↔E9, E10↔E11, E12↔E13 | P6↔P7 | 7 |
  | E14↔E15, E16↔E17, E18↔E19 | P8↔P9 | 6 |
  | E20↔E21, E22↔E23, E24↔E25 | P10↔P11 | 5 |

- Örnek D0: P0 → VL 3–306 → D0 girer → çapraz kablo → D1 çıkar → P1 alır. P0 gönderdiğini P1'den alır (aynı porttan değil) ✅.
- Yük: P0–P3 = 100G link full-duplex (100G TX + 100G RX) ; P4–P11 = 60–80G FD (hafif). Split çiftleri aynı NUMA'da → locality. **[KESIN]**

## 6. Kod değişiklik listesi (dpdk → dpdk_hsn)

1. `Config.h`: `HSN_PORT_MAP`, `PORT_VLAN_CONFIG` (30 port), VLAN/VL-ID aralıkları, port sayıları.
2. `Config.h`/Makefile: `STATS_MODE_DTN` → `STATS_MODE_HSN`, NUM_TX/RX_CORES ayarı (100G için).
3. Paket kurulumu: IP/UDP kaldır, Ethernet-only, jumbo, (boyut→adet) pattern.
4. `interfaces` VLAN katmanı + `AteCumulusConfig.c` egress-untagged sekansı (swp1-4 + swp25-31).
5. `Helpers.c`: 30 satırlık HSN stats tablosu.
6. Eklentiler — **PTP, HealthMonitor, EmbeddedLatency KALDIRILDI** (dosyalar silindi, `ate_mode_enabled` → yeni `AteMode.c/.h`'ye taşındı). Raw bakır + PsuTelemetry şimdilik duruyor. Yeni bir latency testi sonra eklenecek. **[KESIN]**

## 7. Bekleyen bilgiler

Karara bağlananlar (DTN ile aynı): ~~loopback/forward~~ → **loopback [KESIN]** · ~~VL hızı/BAG~~ → **BAG'siz eşit bölüşüm [KESIN]** · ~~VL-ID DST MAC~~ → **[KESIN]**

Hâlâ bekleyenler:
- [ ] Payload gerçek maks boyutu (<9000) → MTU kesinleşir
- [ ] (boyut, adet) pattern tablosu → TX pattern üreteci
- [ ] >4096 boyutlular azalıyor mu
- [x] ~~Her cihaz portunda kaç VL~~ → **304 (100G) / 32 (10G), toplam 2048** [KESIN]
- [x] ~~Port-to-port map~~ → **çapraz-eş, section 5'te tam** [KESIN]
- [x] ~~Hangi eklentiler kalacak~~ → PTP/Health/EmbeddedLatency **kaldırıldı**; raw bakır + PsuTelemetry duruyor. Yeni latency testi sonra.
- [ ] Yeni latency testi tasarımı (EmbeddedLatency yerine)

## 8. VLAN + VL-ID + port tam spec (Config.h kaynağı)

Her cihaz portu: ingress VLAN (server→cihaz) 100–129, dönüş VLAN = çapraz-eşinin pvid'i (200–229).
Cihaz swp'si: `bridge-vids {100+idx} untagged` + `bridge-pvid {200+idx}`.

| dev | swp  | VL-ID | ingVLAN | dönVLAN |  TX→ |  RX← | çapraz |
|-----|------|-------|---------|---------|------|------|--------|
| D0  | swp1 | 3–306 |   100   |   201   |  P0  |  P1  | D1 |
| D1 | swp2 | 307–610 | 101 | 200 | P1 | P0 | D0 |
| D2 | swp3 | 611–914 | 102 | 203 | P2 | P3 | D3 |
| D3 | swp4 | 915–1218 | 103 | 202 | P3 | P2 | D2 |
| E0 | swp25s0 | 1219–1250 | 104 | 205 | P4 | P5 | E1 |
| E1 | swp25s1 | 1251–1282 | 105 | 204 | P5 | P4 | E0 |
| … E2–E25 | swp25s2…swp31s1 | 1283–2050 | 106–129 | (eş pvid) | P4–P11 çapraz | | |

Server portu görünümü (kural ✅, gönderdiğini almaz):
`P0:TX[100]RX[200]` `P1:TX[101]RX[201]` … `P4:TX[104,106,108,110]RX[204,206,208,210]` … (10G portları 3–4 VLAN).
100G portu (P0–P3) = 1 VLAN×304 VL ; 10G portu (P4–P11) = 3–4 VLAN×32 VL.

## 9. Queue / Core / RSS mimarisi (performans kararı)

Heterojen yük (100G maks, 10G hafif) için:

- **TX:** portun VL'leri TX queue'lara bölünür (100G: 304 VL → N queue alt-aralık; 10G: cihaz-portu/VLAN başına queue).
- **RX:** **RSS (L2/ETH hash — DST MAC = VL-ID)** ile VL'ler RX queue'lara yayılır. Ethernet-only olduğu için L3/L4 RSS yok; `RTE_ETH_RSS_ETH`. Per-VLAN rte_flow steering'e gerek yok.
- **Per-cihaz-portu istatistik:** donanım per-queue yerine **yazılım per-VL sayaçları** (TX+RX; RSS her VL'i tek queue/core'a tutarlı yolladığı için sayaçlar core-local, atomik gerekmez). 30 satırlık HSN tablosu bunlardan.
- **Core (Makefile ile ayarlanır, gerçek çekirdek sayısına göre):**
  - 100G portu (P0–P3): **4 TX + 4 RX** core.
  - 10G portu (P4–P11): **2 TX + 2 RX** core (queue'ları paylaşarak).
  - Varsayılan toplam ~64 core. AMD Genoa çift-soket; gerçek sayıya göre `NUM_*_CORES_100G/10G` ile ayarlanır.
- PRBS + sequence doğrulaması her VL için (DTN ile aynı). **[KESIN — v1, ayarlanabilir]**
