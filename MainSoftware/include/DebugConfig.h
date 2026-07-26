#ifndef MAIN_DEBUG_CONFIG_H
#define MAIN_DEBUG_CONFIG_H

// ============================================================================
// MainSoftware Debug / Otomasyon Konfigürasyonu
// ----------------------------------------------------------------------------
// dpdk_cmc'nin Config.h DEBUG_MODE'undan (o SADECE Health Monitor içindir)
// tamamen AYRIDIR. Bu flag MainSoftware'in başlangıç sekansını (cihaz seçimi,
// ileride test bilgisi/prompt'lar, PDF vb.) otomatikleştirmek içindir.
// ============================================================================

// 0 = normal interaktif akış (production).
// 1 = başlangıç sekansı otomatik doldurulur (kullanıcıya sorulmaz).
#ifndef MAIN_DEBUG_MODE
#define MAIN_DEBUG_MODE 1
#endif

// ----------------------------------------------------------------------------
// Cihaz seçimi (unitSelector) — MAIN_DEBUG_MODE=1 iken hardcoded kullanılır.
// Buraya yazılan cihaz otomatik seçilir. Geçerli değerler (Unit enum):
//   CMC / MMC / VMC / DTN / HSN
// ŞİMDİLİK yalnızca CMC entegre; diğerleri sonra eklenecek.
// ----------------------------------------------------------------------------
#ifndef DEBUG_UNIT
#define DEBUG_UNIT CMC
#endif

// ----------------------------------------------------------------------------
// Test bilgileri (collectTestInfo) — MAIN_DEBUG_MODE=1 iken bu değerlerle
// otomatik doldurulur (kullanıcıya sorulmaz). Buradan değiştir.
// Kısıtlar: ATE serial ≤3 hane, Bilgem tam 9 hane, serial sadece rakam.
// ----------------------------------------------------------------------------
#ifndef DEBUG_TEST_NAME
#define DEBUG_TEST_NAME     "auto_test"
#endif
#ifndef DEBUG_ATE_SERIAL
#define DEBUG_ATE_SERIAL    "1"
#endif
#ifndef DEBUG_BILGEM_NUMBER
#define DEBUG_BILGEM_NUMBER "000000000"
#endif
#ifndef DEBUG_SERIAL_NUMBER
#define DEBUG_SERIAL_NUMBER "1"
#endif
#ifndef DEBUG_TESTER_NAME
#define DEBUG_TESTER_NAME   "AUTO"
#endif
#ifndef DEBUG_QC_NAME
#define DEBUG_QC_NAME       "AUTO"
#endif

// ----------------------------------------------------------------------------
// "Teste devam?" (trigger paketi gelmediğinde sorulan y/N) — MAIN_DEBUG_MODE=1
// iken bu kadar saniye beklenip otomatik "yes" verilir (operatör beklenmez).
// ----------------------------------------------------------------------------
#ifndef MAIN_AUTO_CONTINUE_WAIT_S
#define MAIN_AUTO_CONTINUE_WAIT_S (5 * 60)
#endif

#endif // MAIN_DEBUG_CONFIG_H