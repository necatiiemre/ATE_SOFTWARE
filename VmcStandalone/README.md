# VmcStandalone

MainSoftware'den bağımsız çalışan VMC uygulaması. PSU, server ya da DPDK
yönetimi yapmaz; adımlar sırayla çalışır, ilk başarısız adımda durur.

| Adım | Ne yapar |
|------|----------|
| 1 | Cumulus switch'i (10.1.33.3) VMC'ye göre konfigüre eder |
| 2 | `vmc_test`'i ATE server'a (10.1.33.2) gönderip orada çalıştırır |

## Hızlı başlangıç

```bash
cd VmcStandalone
./prepare_release.sh        # her şeyi derler, binary'leri Execute/ klasörüne toplar
cd Execute
./vmcStandalone             # Adım 1 -> Adım 2
```

`prepare_release.sh` şunları yapar:

1. PC uygulaması `vmcStandalone`'u derler (cmake + make, `build/` klasöründe)
2. `vmc_test`'i server'a gönderip **orada** derler ve binary'yi geri çeker
3. Çalışmak için gereken her şeyi `Execute/` klasörüne kopyalar:

```
Execute/
├── vmcStandalone                      ← bunu çalıştır
├── CumulusInterfaces/VMC/interfaces
├── vmc_test/vmc_test (+ VERSION.txt)
├── VERSION.txt
└── LOGS/VMC/<tarih_saat>/             ← çalıştırdıkça oluşan loglar
```

| Komut | Ne yapar |
|-------|----------|
| `./prepare_release.sh` | Derle + `Execute/`'a topla (değişmeyen şey yeniden derlenmez) |
| `./prepare_release.sh --clean` | `build/`'i silip her şeyi sıfırdan derler (vmc_test dahil) |
| `./prepare_release.sh --skip-vmc-test` | Server'a bağlanmadan sadece PC uygulamasını derler; `Execute/`'taki mevcut vmc_test korunur |

- `Execute/LOGS/` hiçbir zaman silinmez; tekrar hazırlamak eski logları bozmaz.
- Build başarısız olursa `Execute/` değiştirilmez (eski binary'ler yerinde kalır).
- `Execute/` klasörü olduğu gibi başka bir PC'ye kopyalanıp oradan çalıştırılabilir
  (o PC'de `sshpass`, `ssh`, `scp`, `tar` kurulu olmalı).
- `build/` ve `Execute/` git'e eklenmez (`.gitignore`).

## Adım 1 – Cumulus VMC konfigürasyonu

MainSoftware'deki `Vmc::configureSequence()` içindeki Cumulus kısmının
(`CumulusHelper::deployNetworkInterfaces` + `CumulusHelper::configureSequenceVmc`)
birebir aynısıdır, yalnızca VMC yolu alınmıştır (DTN/CMC/HSN yok):

1. SSH bağlantı testi
2. `CumulusInterfaces/VMC/interfaces` → `/tmp/interfaces` (scp) → `sudo mv` ile
   `/etc/network/interfaces`, ardından `sudo ifreload -a`
3. `bridge -j vlan show` okunur; 16 VMC girdisi zaten varsa VLAN adımı atlanır
4. Yoksa tek SSH oturumunda `bridge vlan add dev <port> vid <vlan> untagged`:

   | Port | VLAN (untagged) |
   |------|-----------------|
   | swp9s0–swp9s3   | 97–100  |
   | swp10s0–swp10s3 | 101–104 |
   | swp11s0–swp11s3 | 105–108 |
   | swp12s0–swp12s3 | 109–112 |

5. Durum tekrar okunup 16 girdinin hepsi doğrulanır (eksikler listelenir)

Bağlantı bilgileri: `include/CumulusConfig.h` (MainSoftware ile aynı).

## Derleme (elle)

Normalde `prepare_release.sh` yeterli. Elle derlemek istersen:

```bash
cd VmcStandalone
mkdir -p build && cd build
cmake ..
make
```

`make` iki şey yapar:

1. PC uygulaması `vmcStandalone`'u derler.
2. **`vmc_test`'i server'da derler** (prepare_release.sh'in dpdk_vmc için yaptığının
   aynısı): `vmc_test/` kaynakları server'da `/home/user/Desktop/vmc_test_build`'e
   kopyalanır, orada `make` çalışır, binary geri çekilir.

Sonuç – taşınabilir `build/bin/` klasörü:

```
build/bin/
├── vmcStandalone                       ← çalıştırılacak program
├── CumulusInterfaces/VMC/interfaces    ← Adım 1'de switch'e gönderilir
└── vmc_test/
    ├── vmc_test                        ← server'da derlenmiş binary (Adım 2)
    └── VERSION.txt
```

- `vmc_test` sadece `vmc_test/` içinde bir şey (ya da `ServerConfig.h`) değiştiyse
  yeniden derlenir; değişiklik yoksa `make` server'a hiç bağlanmaz.
- Elle yeniden derletmek: `make vmc_test`
- Server kapalıyken sadece PC uygulamasını derlemek: `cmake -DVMC_TEST_SERVER_BUILD=OFF ..`
- Server build script'i tek başına da çalışır: `scripts/build_vmc_test_on_server.sh`

Gerekli araçlar (PC): `cmake`, `g++`, `sshpass`, `ssh`, `scp`, `tar`.
Server'da: `gcc`, `make`.

## Interfaces dosyası

Uygulama repodaki diğer hiçbir şeye bağlı değildir; switch'e gönderilen dosya
bu projenin içindedir:

```
VmcStandalone/CumulusInterfaces/VMC/interfaces
```

Değişiklik **bu dosyada** yapılır. Derleme sırasında `build/bin/CumulusInterfaces/VMC/`
altına kopyalanır (dosya değişince bir sonraki `cmake --build` kopyayı otomatik
günceller), böylece `build/bin/` klasörü tek başına başka bir PC'ye taşınabilir.

Çalışırken dosya şu sırayla aranır, ilk bulunan kullanılır:

1. `-i / --interfaces <yol>` verildiyse o dosya
2. `<exe klasörü>/CumulusInterfaces/VMC/interfaces` (normalde `build/bin/` içindeki kopya)
3. `<exe klasörü>/../../CumulusInterfaces/VMC/interfaces` (`VmcStandalone/` içindeki asıl dosya)
4. `./CumulusInterfaces/VMC/interfaces` (çalıştırılan klasör)

`VmcStandalone/` dışındaki hiçbir yola bakılmaz.

Gerekli araçlar: `sshpass`, `ssh`, `scp`.

## Kullanım

```bash
cd build/bin
./vmcStandalone                    # Adım 1 (Cumulus) -> Adım 2 (vmc_test server'da)
./vmcStandalone -- --arg1 deger    # "--" sonrası her şey vmc_test'e argüman olarak gider
./vmcStandalone --skip-cumulus     # sadece Adım 2
./vmcStandalone --skip-vmc-test    # sadece Adım 1
./vmcStandalone --verify-only      # sadece switch'teki VMC VLAN durumunu kontrol et
./vmcStandalone -v                 # her SSH komutunu ve çıktısını ekranda da göster
./vmcStandalone --log-dir /yol     # logları başka bir klasöre yaz
./vmcStandalone -i /yol/interfaces # başka bir interfaces dosyası gönder
```

Adım 2'de `build/bin/vmc_test/` klasörü server'da `/home/user/Desktop/vmc_test/`'e
kopyalanır ve `sudo ./vmc_test` ile **interaktif** çalıştırılır: çıktısı canlı
görünür, klavye girişi ve Ctrl+C programa ulaşır. `vmc_test` 0 dışında bir kodla
çıkarsa adım başarısız sayılır.

Çıkış kodu: `0` başarılı, `1` bir adım başarısız, `2` hatalı argüman.

Bağlantı ayarları: switch `include/CumulusConfig.h`, server `include/ServerConfig.h`
(build script'i de server ayarlarını bu dosyadan okur).

## Log kaydı

Her çalıştırma kendi klasörünü açar (varsayılan: programın yanındaki `LOGS/VMC/`):

```
build/bin/LOGS/VMC/2026-11-04_14-16-59/
├── VMC_Standalone_Log.log        ← ana log
├── cumulus_interfaces_deployed   ← switch'e gönderilen interfaces dosyasının birebir kopyası
└── cumulus_bridge_vlan.json      ← doğrulama sonrası switch'in 'bridge -j vlan show' çıktısı
```

`VMC_Standalone_Log.log` içeriği:

- **Başlık:** başlangıç zamanı, yazılım ve vmc_test versiyonları, PC adı, komut satırı,
  switch/server adresleri
- **Ekranda görünen her şey**, her satır zaman damgasıyla (`[14:16:59.263] ...`) – server'da
  çalışan `vmc_test`'in canlı çıktısı dahil
- **Ayrıntılar** (`|` ile başlayan satırlar, ekranda sadece `-v` ile görünür): her SSH
  komutu, çıkış kodu ve çıktısı, scp/mv sonuçları, switch'in VMC port tablosu
- **Özet:** adım adım PASS / FAIL / NOT RUN, süreler, sonuç, bitiş zamanı

Şifreler log'a ve ekrana `****` olarak yazılır; renk kodları dosyada temizlenir.
Program yarıda kesilirse (Ctrl+C / kill) log'un sonuna bunu belirten bir satır düşülür.

> Not: `build/` klasörü silinirse içindeki `LOGS/` da gider. Kalıcı bir yer için
> `--log-dir` kullanın ya da `build/bin/`'i başka bir yere kopyalayıp oradan çalıştırın.

## vmc_test (server uygulaması)

`vmc_test/` – ATE server'da derlenip çalışan VMC test uygulaması (eskiden `dpdk_vmc`'nin
yaptığı iş). DPDK kullanmaz; `AF_PACKET` raw socket ile hazır Ethernet frame'i gönderir.

```
vmc_test/
├── Makefile
├── include/PacketDefs.h      ← paket formatı sabitleri (MAC/IP önekleri, A/B, port, TTL, sınırlar)
├── src/RawSocket.c           ← raw socket aç / gönder / kapat
├── src/Packet.c              ← buildPacket(), A/B sequence sayaçları
├── src/TestScenarios.c       ← gönderim senaryoları (tryPacketSend, ileride diğerleri)
└── src/main.c                ← argümanlar + çalıştırılan senaryo çağrıları
```

### Paket formatı (dpdk_vmc / dpdk_cmc ile aynı başlıklar)

```
ETH 14 | 802.1Q 4 | IPv4 20 | UDP 8 | DATA N byte | SEQ 1 byte
```

| Alan | Değer |
|------|-------|
| DST MAC | `03:00:00:00:VV:VV` (son 2 byte = VL-IDX) |
| SRC MAC | `02:00:00:00:00:20` Net A / `02:00:00:00:00:40` Net B (dpdk_cmc kuralı) |
| VLAN | TPID 0x8100, priority 0, VID = VLAN ID |
| IPv4 | src `10.0.0.0`, dst `224.224.VV.VV`, TTL 1, TOS 0, checksum hesaplanır |
| UDP | port 100 → 100, checksum 0 |
| Payload | N byte veri, **N+1. byte sequence**; verinin her byte'ı = sequence değeri (PRBS yok) |

- Payload 100 → frame 147 byte; payload 200 → 201. byte sequence, frame 247 byte.
- Payload sınırı: 1..1466 (DPDK'daki en büyük payload: 1466 + 1 seq).
- Sequence Net A ve Net B için **ayrı sayaç**: `0, 1, 2, … 255, 1, 2, …` (0 sadece ilk pakette).

### tryPacketSend

```c
int tryPacketSend(RawSocket *sock, Network net, uint16_t vlanId, uint16_t vlIdx,
                  uint16_t payloadLen, int count, unsigned delayMs);
```

Hepsi parametre: network (`NET_A`/`NET_B`), VLAN ID, VL-IDX, payload uzunluğu, paket sayısı,
paketler arası bekleme (ms). Gönderilen paket sayısını döner. Şu anki VMC deneme senaryosu
(`src/main.c`):

```c
tryPacketSend(&sock, NET_A, 100, 70, 100, 10, 10);   // 10 x Net A, VLAN 100, VL-IDX 70
tryPacketSend(&sock, NET_B, 100, 71, 100, 10, 10);   // 10 x Net B, VLAN 100, VL-IDX 71
```

Yeni senaryolar `src/TestScenarios.c`'ye ayrı fonksiyon olarak eklenir.

### Argümanlar

```bash
sudo ./vmc_test                       # varsayılan interface: ens1f0np0
sudo ./vmc_test --interface ens1f1np1
sudo ./vmc_test --hexdump             # gönderilen her frame'i hex olarak bas
```

vmcStandalone üzerinden: `./vmcStandalone -- --interface ens1f1np1 --hexdump`

Bir paket gönderilemezse `vmc_test` 1 ile çıkar ve vmcStandalone Adım 2'yi FAIL sayar.

### Server'da elle derleme

Normalde gerekmez (`prepare_release.sh` server'da derler):

```bash
cd vmc_test
make          # derle -> ./vmc_test
make debug    # debug build -> ./vmc_test-debug
make run      # sudo ile çalıştır (raw socket root ister)
make clean
```

## Yeni adım ekleme

`src/main.cpp` içindeki `steps` listesine ekleyin:

```cpp
steps.push_back({"Adim adi", [&] { return birFonksiyon(); }});
```

## Kod yapısı

| Dosya | Kaynak (MainSoftware) |
|-------|-----------------------|
| `SystemCommand` | `SystemCommandExecutor` (sadeleştirilmiş) |
| `SshClient` | `SSHDeployer` – `testConnection`, `execute`, `copyFileToPath`, `copyDirectory`, `executeInteractive` |
| `CumulusVmc` | `CumulusHelper` – sadece VMC yolu |
| `VmcTestRunner` | `Vmc::runDpdkVmcInteractive` (dpdk_vmc yerine vmc_test) |
| `scripts/build_vmc_test_on_server.sh` | `prepare_release.sh` → `prepare_dpdk_vmc` |
| `Log.h` | `ErrorPrinter` + `DEBUG_LOG` (ayrıntılar her zaman log dosyasına, `--verbose` ile ekrana da) |
| `Logger` | `ReportManager`'ın log klasörü/başlık kısmı + ekran çıktısının dosyaya kaydı |
