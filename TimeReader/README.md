# TimeReader

Bağımsız bir mini app. SyncServer'ın seri porttan gönderdiği **ham zaman
satırlarını ekrana basar** — amacı, gelen zamanın **UTC (+00)** mi yoksa
**yerel (GMT+3, +03)** mi olduğunu ve formatı gözle doğrulamaktır.

Repo içindeki diğer projelerden **tamamen bağımsızdır**.

## Derleme

```bash
./build.sh
# veya
g++ -std=c++17 -O2 -o time_reader time_reader.cpp
```

## Kullanım

```bash
./time_reader                     # /dev/ttyUSB0 @ 9600 (varsayilan)
./time_reader /dev/ttyUSB0 9600   # cihaz ve baud belirt
./time_reader --demo "2025 365 23:59:59 +00"   # donanimsiz parse testi
```

Durdurmak için **Ctrl+C**.

## Her satır için gösterilenler

- Ham metin (aynen geldiği gibi)
- Ham hex (gizli/kontrol karakterlerini de görürsün)
- Parse edilmiş alanlar: **YIL**, yılın günü **DDD** (+ hesaplanan ay/gün),
  **HH:MM:SS**, ve **TZ alanı** (`+00` / `+03` …)

## Ne anlama geliyor

```
TZ alani = +00  ->  cihaz UTC gonderiyor
TZ alani = +03  ->  cihaz yerel saat (GMT+3) gonderiyor
```

Forwarder şu an bu satırı **olduğu gibi** iletiyor (offset uygulamıyor); yani
çıktının UTC mi yerel mi olduğunu bu alan belirler. Bunu görüp karar verdikten
sonra gerekiyorsa forwarder'a sabit `+3` ekleyebiliriz.
