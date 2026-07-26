#!/usr/bin/env bash
#
# CMC otomasyon döngüsü
# ============================================================================
# MainSoftware'i (MAIN_DEBUG_MODE=1 ile derlenmiş build) sürekli çalıştırır.
# Her koşuyu en fazla RUN_DURATION (varsayılan 2 saat) çalıştırır, süre dolunca
# DÜZGÜNCE (tek Ctrl+C = SIGINT) kapatır ve baştan başlatır. Kendiliğinden erken
# biterse (test tamamlanıp temiz kapanış yaparsa) kısa bir bekleme sonrası tekrar
# başlar.
#
# NOT: Kapatma için terminale/klavyeye gerek YOK. Script, başlattığı process'in
# PID'sine doğrudan "kill -INT <pid>" gönderir — bu, o process'e tek bir Ctrl+C
# göndermekle aynıdır, hangi terminalde olduğunun önemi yoktur.
#
# Loglar:
#   Yazılım her açılışta LOGS/CMC/<YYYY-MM-DD_HH-MM-SS>/ altında kendi tarihli
#   klasörünü oluşturur. Bu script log bölmeyle UĞRAŞMAZ — sadece başlat/durdur.
#
# DÖNGÜYÜ (script'i) durdurma — mevcut testi temiz kapatıp döngüden çıkar:
#   - Önplanda çalışıyorsa: terminalde BİR KEZ Ctrl+C.
#   - Arka planda/detached ise: sudo kill $(cat <SCRIPT_DIR>/LOGS/automation_loop.pid)
#   Her ikisi de aynı trap'i tetikler: çalışan testi SIGINT ile normal kapatır,
#   sonra script biter (yeniden başlatmaz).
#
# Kapanış (kademeli):
#   1) SIGINT  = tek Ctrl+C -> NORMAL kapanış. Yazılım monitoring döngüsünden
#      çıkıp normal akışı tamamlar: DPDK stop -> log kaydet -> PSU OFF ->
#      recordPowerOffTime -> return 0. Tercih edilen yol budur.
#   2) SIGTERM = (yalnızca SIGINT GRACE içinde kapanmazsa) SafeShutdown acil
#      yolu: önce PSU OFF, sonra loglar. exit 1.
#   3) SIGKILL = (yalnızca hâlâ takılıysa) son çare.
#   Her iki temiz yolda da loglar kaydedilir, PDF üretilmez (MAIN_DEBUG_MODE).
#
# ÖNEMLİ: kill -9 doğrudan KULLANILMAZ — sadece 1 ve 2 takılırsa son çaredir.
# ============================================================================

set -u

# ------------------------------- root / sudo --------------------------------
# Test; PSU, seri port ve DPDK deploy gibi root yetkisi isteyen isler yapar.
# Bu yuzden testi sudo ile calistiriyoruz. Ancak binary'nin onune tek tek sudo
# koymak yerine SCRIPT'IN KENDISINI bir kez sudo ile yeniden baslatiyoruz:
#   - Test root (sudo) olarak calisir.
#   - kill/sinyal (SIGTERM ile temiz kapanis) sorunsuz olur; root olmayan bir
#     script sudo'nun root process'ine sinyal gonderemez (EPERM) sorunu olmaz.
#   - 2 saatlik dongu icinde tekrar tekrar sudo cagirip sifre/timeout dertleri
#     yasanmaz (tek seferlik yetki).
# Zaten root isek (ornegin cron root ile calistiriyorsa) tekrar sudo cagrilmaz.
if [[ ${EUID:-$(id -u)} -ne 0 ]]; then
    echo "[INFO] Test root yetkisi ister; script sudo ile yeniden baslatiliyor..."
    exec sudo -E bash "$0" "$@"
fi

# ------------------------------- Ayarlar ------------------------------------
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

# mainSoftware binary yolu. MAIN_DEBUG_MODE=1 ile derlenmiş olmalı.
APP_BIN="${APP_BIN:-$SCRIPT_DIR/build/bin/mainSoftware}"

RUN_DURATION="${RUN_DURATION:-7800}"   # her koşunun üst sınırı (sn) = 2 saat 10 dakika
GRACE="${GRACE:-120}"                  # SIGTERM sonrası temiz kapanış için bekleme (sn)
RESTART_DELAY="${RESTART_DELAY:-10}"   # koşular arası bekleme (sn)
MIN_RUNTIME="${MIN_RUNTIME:-30}"       # bundan kısa koşu = crash-loop uyarısı (sn)

# Script'in kendi (watchdog) logu — yazılımın test loglarından ayrıdır.
WATCHDOG_LOG="${WATCHDOG_LOG:-$SCRIPT_DIR/LOGS/watchdog.log}"

# Döngüyü dışarıdan durdurmak için PID dosyası:
#   sudo kill $(cat "$PIDFILE")
PIDFILE="${PIDFILE:-$SCRIPT_DIR/LOGS/automation_loop.pid}"
# ----------------------------------------------------------------------------

mkdir -p "$(dirname "$WATCHDOG_LOG")"
mkdir -p "$(dirname "$PIDFILE")"

# Kendi PID'imizi yaz (sudo re-exec sonrası bu, döngüyü çalıştıran root bash'tir).
echo "$$" > "$PIDFILE"
# Script hangi yolla biterse bitsin PID dosyasını temizle.
trap 'rm -f "$PIDFILE"' EXIT

log() {
    local ts; ts="$(date '+%Y-%m-%d %H:%M:%S')"
    echo "[$ts] $*" | tee -a "$WATCHDOG_LOG"
}

if [[ ! -x "$APP_BIN" ]]; then
    log "HATA: calistirilabilir bulunamadi: $APP_BIN"
    log "Once MAIN_DEBUG_MODE=1 ile derleyin:"
    log "  cmake -S \"$SCRIPT_DIR\" -B \"$SCRIPT_DIR/build\" -DCMAKE_CXX_FLAGS=-DMAIN_DEBUG_MODE=1"
    log "  cmake --build \"$SCRIPT_DIR/build\" -j"
    exit 1
fi

CHILD_PID=""

# Çocuğu kademeli ve düzgün durdur:
#   1) SIGINT  (tek Ctrl+C) -> yazılımın NORMAL kapanışı. GRACE kadar bekle.
#   2) Takılırsa SIGTERM -> SafeShutdown acil kapanışı. 30s daha bekle.
#   3) Hâlâ yaşıyorsa SIGKILL (son çare).
graceful_stop() {
    local pid="$1"
    if ! kill -0 "$pid" 2>/dev/null; then
        return 0
    fi

    # 1) Tek Ctrl+C = SIGINT: monitoring döngüsünden çıkıp normal kapanış yapar.
    log "SIGINT (tek Ctrl+C) gonderiliyor (pid=$pid), normal kapanis icin en fazla ${GRACE}s beklenecek..."
    kill -INT "$pid" 2>/dev/null

    local waited=0
    while kill -0 "$pid" 2>/dev/null; do
        if (( waited >= GRACE )); then
            # 2) Normal kapanis takildi -> SIGTERM (SafeShutdown: once PSU OFF).
            log "UYARI: normal kapanis ${GRACE}s icinde bitmedi, SIGTERM (acil kapanis) gonderiliyor (pid=$pid)."
            kill -TERM "$pid" 2>/dev/null
            local t=0
            while kill -0 "$pid" 2>/dev/null && (( t < 30 )); do
                sleep 2; t=$(( t + 2 ))
            done
            # 3) Hala yasiyorsa son care SIGKILL.
            if kill -0 "$pid" 2>/dev/null; then
                log "UYARI: hala kapanmadi, son care SIGKILL (pid=$pid)."
                kill -KILL "$pid" 2>/dev/null
            fi
            break
        fi
        sleep 2
        waited=$(( waited + 2 ))
    done
    wait "$pid" 2>/dev/null
    log "Cocuk process durdu (pid=$pid)."
}

# Script'in kendisine SIGINT/SIGTERM gelirse: çocuğu düzgünce durdur ve çık.
STOP_REQUESTED=0
on_signal() {
    STOP_REQUESTED=1
    log "Script'e kapanma sinyali geldi, dongu durduruluyor..."
    if [[ -n "$CHILD_PID" ]]; then
        graceful_stop "$CHILD_PID"
    fi
    exit 0
}
trap on_signal INT TERM

log "===== CMC otomasyon dongusu basliyor ====="
log "Binary      : $APP_BIN"
log "Kosu suresi : ${RUN_DURATION}s"
log "Grace       : ${GRACE}s"
log "PID dosyasi : $PIDFILE (pid=$$)"
log "Durdurmak icin: onplanda Ctrl+C  |  arka planda: sudo kill $$"

cycle=0
while (( STOP_REQUESTED == 0 )); do
    cycle=$(( cycle + 1 ))
    start_ts=$(date +%s)
    log "----- Kosu #$cycle basliyor -----"

    "$APP_BIN" &
    CHILD_PID=$!
    log "mainSoftware baslatildi (pid=$CHILD_PID)."

    # RUN_DURATION dolana kadar VEYA process kendiliginden bitene kadar bekle.
    elapsed=0
    while (( elapsed < RUN_DURATION )); do
        if ! kill -0 "$CHILD_PID" 2>/dev/null; then
            break   # kendiliginden bitti (test tamamlandi -> temiz kapanis)
        fi
        sleep 5
        elapsed=$(( elapsed + 5 ))
    done

    if kill -0 "$CHILD_PID" 2>/dev/null; then
        log "Kosu #$cycle: ${RUN_DURATION}s doldu, planli yeniden baslatma."
        graceful_stop "$CHILD_PID"
    else
        wait "$CHILD_PID" 2>/dev/null
        rc=$?
        log "Kosu #$cycle: process kendiliginden bitti (rc=$rc)."
    fi
    CHILD_PID=""

    runtime=$(( $(date +%s) - start_ts ))
    if (( runtime < MIN_RUNTIME )); then
        log "UYARI: kosu cok kisa surdu (${runtime}s < ${MIN_RUNTIME}s). Crash-loop olabilir."
    fi

    (( STOP_REQUESTED == 1 )) && break
    log "Sonraki kosuya ${RESTART_DELAY}s..."
    sleep "$RESTART_DELAY"
done

log "===== CMC otomasyon dongusu bitti ====="