# Versiyonlama Rehberi

Bu doküman, ATE yazılım paketindeki her bileşenin sürümünü **nerede
bulacağınızı** ve **nereden değiştireceğinizi** açıklar.

## Özet

Her bileşenin sürümü tek bir kaynakta (single source of truth) tutulur.
Bu değer üç yere akar:

```
  <Bileşen>Version.h / .py   ──►  ① CMake (derleme)   ──►  test raporu başlığı
       (TEK KAYNAK)          ──►  ② prepare_release.sh ──►  binary yanında VERSION.txt
```

Sürümü değiştirmek istediğinizde **yalnızca ① numaralı kaynak dosyayı**
düzenlersiniz; diğer iki yer bu değeri otomatik okur.

---

## 1. Kaynak dosyalar (tek gerçek kaynak)

Her bileşenin sürümü, adı bileşene göre verilmiş bir dosyada, aynı adı
taşıyan bir değişkende tutulur.

| Bileşen | Kaynak dosya | Değişken |
|---|---|---|
| MainSoftware | `MainSoftware/include/MainSoftwareVersion.h` | `MainSoftwareVersion` |
| RemoteConfigSender | `RemoteConfigSender/include/RemoteConfigSenderVersion.h` | `RemoteConfigSenderVersion` |
| Test_Starters | `Test_Starters/include/TestStartersVersion.h` | `TestStartersVersion` |
| Flicker_Detection | `Flicker_Detection/include/FlickerDetectionVersion.h` | `FlickerDetectionVersion` |
| FirmwareUpdater | `FirmwareUpdater/FirmwareUpdaterVersion.py` | `FirmwareUpdaterVersion` |
| PdfReportGenerator | `PdfReportGenerator/PdfReportGeneratorVersion.py` | `PdfReportGeneratorVersion` |
| dpdk (DTN) | `dpdk/include/DtnIrswVersion.h` | `DtnIrswVersion` |
| dpdk_cmc | `dpdk_cmc/include/CmcVersion.h` | `CmcVersion` |
| dpdk_vmc | `dpdk_vmc/include/VmcVersion.h` | `VmcVersion` |
| dpdk_hsn | `dpdk_hsn/include/HsnIrswVersion.h` | `HsnIrswVersion` |

**İsimlendirme kuralı:** dosya adı = değişken adı = `<BileşenAdı>Version`
(PascalCase). DPDK varyantları klasör adı yerine dağıtıldıkları **ünite
adını** kullanır (`CumulusInterfaces/` kuralıyla uyumlu: DTNIRSW, CMC, VMC,
HSNIRSW → `DtnIrswVersion`, `CmcVersion`, `VmcVersion`, `HsnIrswVersion`).

Örnek içerik:

```c
/* dpdk/include/DtnIrswVersion.h */
#define DtnIrswVersion "0.1.0"
```

```python
# FirmwareUpdater/FirmwareUpdaterVersion.py
FirmwareUpdaterVersion = "0.1.0"
```

---

## 2. Çalışma anında: test raporu başlığı

MainSoftware derlenirken `MainSoftware/CMakeLists.txt` bu header'ları okur ve
sürümleri derleme tanımı (compile definition) olarak koda gömer. Sonuç, her
test raporunun **başlığında** görünür:

```
Software Version    : 0.1.0        <- MainSoftwareVersion
DPDK Version        : 0.1.0        <- seçili üniteye göre Dtn/Cmc/VmcVersion
```

- `Software Version` → `MainSoftwareVersion`
- `DPDK Version` → çalışılan üniteye göre:
  DTN→`DtnIrswVersion`, CMC→`CmcVersion`, VMC→`VmcVersion`,
  MMC/HSN→`N/A` (bu üniteler DPDK uygulaması çalıştırmaz)

İlgili kod: `MainSoftware/src/ReportManager.cpp` (`dpdkVersionForUnit()`).

---

## 3. Dağıtılmış pakette: `VERSION.txt` dosyaları

`prepare_release.sh` çalıştırıldığında her binary'nin yanına, sürümü içeren
bir `VERSION.txt` bırakılır. Böylece kaynak ağacına ihtiyaç olmadan, dağıtılan
klasöre bakarak sürümü öğrenebilirsiniz. Hepsi `MMUComputerTestSoftware/`
altındadır:

| Bileşen | Paketteki konum |
|---|---|
| **Paket (genel)** | `MMUComputerTestSoftware/VERSION.txt` (MainSoftware ile aynı) |
| MainSoftware | `MMUComputerTestSoftware/MainSoftware/bin/VERSION.txt` |
| RemoteConfigSender | `MMUComputerTestSoftware/RemoteConfigSender/VERSION.txt` |
| Test_Starters | `MMUComputerTestSoftware/test_starter_<ünite>/VERSION.txt` |
| Flicker_Detection | `MMUComputerTestSoftware/Flicker_Detection/VERSION.txt` |
| FirmwareUpdater | `MMUComputerTestSoftware/FirmwareUpdater/VERSION.txt` |
| PdfReportGenerator | `MMUComputerTestSoftware/PdfReportGenerator/VERSION.txt` |
| dpdk (DTN) | `MMUComputerTestSoftware/dpdk/VERSION.txt` |
| dpdk_cmc | `MMUComputerTestSoftware/dpdk_cmc/VERSION.txt` |
| dpdk_vmc | `MMUComputerTestSoftware/dpdk_vmc/VERSION.txt` |

> Not: `dpdk_hsn` kaynakta sürümlenir (`HsnIrswVersion`) ancak HSN ünitesi bir
> DPDK uygulaması dağıtmadığı için pakette ayrı bir `VERSION.txt` üretilmez.

Paket kökündeki `MMUComputerTestSoftware/VERSION.txt` tüm test yazılımı
paketinin sürümüdür ve tasarım gereği **MainSoftware sürümünü** yansıtır.

---

## Sürümü nasıl değiştiririm?

1. İlgili bileşenin **1. bölümdeki kaynak dosyasını** açın.
2. Sürüm dizesini güncelleyin (ör. `"0.1.0"` → `"0.2.0"`).
3. Başka hiçbir yeri elle düzenlemeyin — CMake ve `prepare_release.sh` yeni
   değeri otomatik okur.
4. MainSoftware'i yeniden derleyin ve/veya `prepare_release.sh` çalıştırın.

Anlamsal sürümleme (semantic versioning) önerilir: `MAJOR.MINOR.PATCH`.
