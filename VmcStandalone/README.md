# VmcStandalone

MainSoftware'den bağımsız çalışan VMC uygulaması. PSU, server ya da DPDK
yönetimi yapmaz; adımlar sırayla çalışır, ilk başarısız adımda durur.

| Adım | Ne yapar |
|------|----------|
| 1 | Cumulus switch'i (10.1.33.3) VMC'ye göre konfigüre eder |
| 2 | `vmc_test`'i ATE server'a (10.1.33.2) gönderip orada çalıştırır |

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

## Derleme

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
./vmcStandalone -v                 # her SSH komutunu ve çıktısını göster
./vmcStandalone -i /yol/interfaces # başka bir interfaces dosyası gönder
```

Adım 2'de `build/bin/vmc_test/` klasörü server'da `/home/user/Desktop/vmc_test/`'e
kopyalanır ve `sudo ./vmc_test` ile **interaktif** çalıştırılır: çıktısı canlı
görünür, klavye girişi ve Ctrl+C programa ulaşır. `vmc_test` 0 dışında bir kodla
çıkarsa adım başarısız sayılır.

Çıkış kodu: `0` başarılı, `1` bir adım başarısız, `2` hatalı argüman.

Bağlantı ayarları: switch `include/CumulusConfig.h`, server `include/ServerConfig.h`
(build script'i de server ayarlarını bu dosyadan okur).

## vmc_test (server uygulaması)

`vmc_test/` – ATE server'da derlenip çalışacak VMC test uygulaması
(eskiden `dpdk_vmc`'nin yaptığı iş). DPDK kullanmaz, düz raw socket kullanır.
Şu an boş iskelet; içerik eklenecek. Normalde elle derlemeye gerek yok
(`vmcStandalone` build'i server'da derler); server'da doğrudan denemek için:

```
vmc_test/
├── Makefile
├── include/Version.h
└── src/main.c
```

`src/` altındaki tüm `.c` dosyaları (alt klasörler dahil) Makefile tarafından
otomatik derlenir; yeni dosya eklemek için Makefile'a dokunmaya gerek yok.

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
| `Log.h` | `ErrorPrinter` + `DEBUG_LOG` (burada `--verbose` ile çalışma anında açılır) |
