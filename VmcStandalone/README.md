# VmcStandalone

MainSoftware'den bağımsız çalışan VMC uygulaması. PSU, server ya da DPDK
yönetimi yapmaz; adımlar sırayla çalışır, ilk başarısız adımda durur.

| Adım | Ne yapar |
|------|----------|
| 1 | Cumulus switch'i (10.1.33.3) VMC'ye göre konfigüre eder |

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
cmake -S . -B build
cmake --build build
```

Çıktı: `build/bin/vmcStandalone`. Derleme sırasında
`../CumulusInterfaces/VMC/interfaces` dosyası `build/bin/CumulusInterfaces/VMC/`
altına kopyalanır; yani `build/bin/` klasörü tek başına taşınabilir. Dosyanın
tek kaynağı yine repodaki `CumulusInterfaces/VMC/interfaces`'tir.

Gerekli araçlar: `sshpass`, `ssh`, `scp`.

## Kullanım

```bash
./build/bin/vmcStandalone                 # tüm adımlar
./build/bin/vmcStandalone --verify-only   # sadece switch'teki VMC VLAN durumunu kontrol et
./build/bin/vmcStandalone -v              # her SSH komutunu ve çıktısını göster
./build/bin/vmcStandalone -i /yol/interfaces
```

Çıkış kodu: `0` başarılı, `1` bir adım başarısız, `2` hatalı argüman.

## Yeni adım ekleme

`src/main.cpp` içindeki `steps` listesine ekleyin:

```cpp
steps.push_back({"Adim adi", [&] { return birFonksiyon(); }});
```

## Kod yapısı

| Dosya | Kaynak (MainSoftware) |
|-------|-----------------------|
| `SystemCommand` | `SystemCommandExecutor` (sadeleştirilmiş) |
| `SshClient` | `SSHDeployer` – sadece `testConnection`, `execute`, `copyFileToPath` |
| `CumulusVmc` | `CumulusHelper` – sadece VMC yolu |
| `Log.h` | `ErrorPrinter` + `DEBUG_LOG` (burada `--verbose` ile çalışma anında açılır) |
