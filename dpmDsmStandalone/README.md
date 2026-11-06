# dpmDsmStandalone

DPM ve DSM için bağımsız test uygulaması. `vmcStandalone`'un kopyasıdır; akış, paket
senaryoları, log ve build sistemi aynıdır. Farklar: açılışta **DPM / DSM seçimi**, Cumulus'ta
sadece **A/B hattı** (swp9s0 / swp9s1), paket ve PTP değerleri bu hatlara göre, ve unit'e göre
**PTP sekansı**.

## Hızlı başlangıç

```bash
cd dpmDsmStandalone
bash prepare_release.sh      # PC uygulaması + dpm_dsm_test (server'da derlenir) -> Execute/
cd Execute
./dpmDsmStandalone           # açılışta sorar:  1) DPM  2) DSM
./dpmDsmStandalone --unit dpm    # sormadan
```

Akış (VMC ile aynı):

1. Cumulus switch'i (10.1.33.3) seçilen unit'e göre konfigüre et
2. `dpm_dsm_test`'i ATE server'a (10.1.33.2) gönderip `sudo ./dpm_dsm_test --unit dpm|dsm` ile çalıştır:
   paket senaryoları sırayla, en sonda PTP

## DPM / DSM farkları

| | DPM | DSM |
|---|---|---|
| Cumulus | aynı (aşağıda) | aynı (aşağıda) |
| Paket senaryoları | aynı | aynı |
| PTP | sadece **slave** fazı | **grandmaster → master → slave** (VMC ile birebir) |
| Log klasörü | `LOGS/DPM/<tarih_saat>/DPM_Standalone_Log.log` | `LOGS/DSM/<tarih_saat>/DSM_Standalone_Log.log` |

PC tarafında unit'e özel her şey `include/UnitConfig.h`'de, server tarafında
`dpm_dsm_test/src/main.c`'de (`--unit`).

## Cumulus (A/B hattı)

Fiber switch'in 9. portunda: **swp9s0 = A hattı, swp9s1 = B hattı** (1G). Interfaces dosyası
VMC dosyasının aynısıdır (`CumulusInterfaces/DPM/interfaces`, `CumulusInterfaces/DSM/interfaces`);
A/B hattı orada CMC'deki gibi tanımlı:

```
server ens1f0np0 → swp13 (VLAN 97) → swp9s0 untagged → A hattı → geri → PVID 225 → swp13 → server
server ens1f0np0 → swp13 (VLAN 98) → swp9s1 untagged → B hattı → geri → PVID 226 → swp13 → server
```

VLAN adımı (MainSoftware'in CMC'si gibi) sadece iki girdi ekler/kontrol eder:
`bridge vlan add dev swp9s0 vid 97 untagged`, `bridge vlan add dev swp9s1 vid 98 untagged`.

## dpm_dsm_test (server uygulaması)

`vmc_test`'in kopyası; raw socket ile gönderir/alır. Bütün senaryolar `ens1f0np0`
üzerinden A/B hattında koşar: **Net A → VLAN 97, Net B → VLAN 98**. VL-IDX, paket sayıları,
beklemeler ve senaryo sırası `vmcStandalone` ile aynıdır (`dpm_dsm_test/src/main.c`).

### PTP (`casePtp`, en sonda)

Frame yapısı ve VL-IDX'ler VMC ile aynı; bacaklar A/B hattı:

| Bacak | Net | TX VLAN | Unit master (faz 1, 2) | Biz master (faz 2, 3) |
|-------|-----|---------|------------------------|-----------------------|
| A swp9s0 | A | 97 | Sync 1600 → Req 1601 → Resp 1602 | Sync 1500 → Req 1501 → Resp 1502 |
| B swp9s1 | B | 98 | Sync 1620 → Req 1621 → Resp 1622 | Sync 1520 → Req 1521 → Resp 1522 |

- **DSM:** 1. grandmaster (önce kick: Sync 1500/1520 → Req → Resp, sonra unit Sync'i
  gelince başlar) → 30 s bekle → 2. master → 30 s bekle → 3. slave; fazlar 30 s.
- **DPM:** sadece 3. slave fazı (30 s): biz Sync 1500/1520 → DPM Req 1501/1521 → biz Resp 1502/1522.
  Diğer fazlar özette `SKIPPED (not used for this unit)` görünür.
- Unit 225 / 226 ile cevap verir (sadece log'lanır); gelen frame'ler VL-IDX + mesaj tipinden tanınır.

Ayarlar (`dpm_dsm_test/src/main.c`): `PTP_LEGS[]` ve `runCasePtp()` – interface, süreler,
Sync aralığı, timeout, hangi fazların koşacağı (`runGrandmaster` / `runMaster` / `runSlave`).

## Seçenekler

```bash
./dpmDsmStandalone --unit dsm                 # unit (yoksa menü)
./dpmDsmStandalone --verify-only              # sadece switch'teki A/B VLAN durumunu kontrol et
./dpmDsmStandalone --skip-cumulus             # sadece Adım 2
./dpmDsmStandalone --skip-dpm-dsm-test        # sadece Adım 1
./dpmDsmStandalone -v                         # SSH komutlarını ekranda da göster
./dpmDsmStandalone -- --hexdump               # "--" sonrası dpm_dsm_test'e gider
```

Terminal yoksa (otomasyon) `--unit` verilmelidir. Çıkış kodu: `0` başarılı, `1` adım
başarısız, `2` hatalı argüman.

| `prepare_release.sh` | |
|---|---|
| `bash prepare_release.sh` | Derle + `Execute/`'a topla |
| `bash prepare_release.sh --clean` | Sıfırdan derle |
| `bash prepare_release.sh --skip-dpm-dsm-test` | Server'a bağlanmadan sadece PC uygulaması |

Server'da `dpm_dsm_test` `/home/user/Desktop/dpm_dsm_test_build`'de derlenir ve
`/home/user/Desktop/dpm_dsm_test`'e kurulur – `vmcStandalone`'un klasörlerinden ayrıdır.

## Log

Her çalıştırma `Execute/LOGS/<DPM|DSM>/<tarih_saat>/` klasörünü açar: ana log (başlık, ekrandaki
her şey zaman damgasıyla – server'daki `dpm_dsm_test` çıktısı dahil –, SSH ayrıntıları,
özet), switch'e gönderilen interfaces dosyasının kopyası ve switch'in `bridge vlan` durumu.
Şifreler `****` olarak yazılır.

## vmcStandalone'dan farklı olan dosyalar

`include/UnitConfig.h` (yeni), `src/main.cpp` (unit seçimi, log/başlık), `src/CumulusVmc.cpp/.h`
(A/B VLAN tablosu, mesajlar), `include/CumulusConfig.h`, `include/ServerConfig.h`,
`src/VmcTestRunner.cpp/.h`, `CMakeLists.txt`, `prepare_release.sh`,
`scripts/build_dpm_dsm_test_on_server.sh`, `CumulusInterfaces/DPM|DSM/interfaces`,
`dpm_dsm_test/Makefile`, `dpm_dsm_test/src/main.c` (A/B değerleri, `--unit`, PTP ayarı),
`dpm_dsm_test/src/Ptp.c`, `dpm_dsm_test/include/Ptp.h` (unit adı, faz seçimi).
Geri kalan her şey birebir aynıdır.
