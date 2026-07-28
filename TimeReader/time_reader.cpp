// ============================================================================
// time_reader - SyncServer'dan gelen ham zaman satirlarini ekrana basar
// ----------------------------------------------------------------------------
// Amac: cihazin (MicroChip SyncServer) seri porttan ne gonderdigini AYNEN
// gormek. Boylece zamanin UTC (+00) mi yoksa yerel (GMT+3, +03) mi geldigini,
// format ve saat dilimi alanini gozle dogrularsin.
//
// Her satir icin:
//   * ham metin
//   * ham hex (gizli karakterleri de gorursun)
//   * parse edilmis alanlar: YIL, yilin gunu (DDD), HH:MM:SS, TZ alani (DZZ)
//   * DDD'den hesaplanan ay/gun
//
// Bu app tamamen bagimsizdir (MainSoftware'e bagli degildir).
//
// Derleme:
//   g++ -std=c++17 -O2 -o time_reader time_reader.cpp    (veya ./build.sh)
//
// Kullanim:
//   ./time_reader                       # /dev/ttyUSB0 @ 9600 (varsayilan)
//   ./time_reader /dev/ttyUSB0 9600     # cihaz ve baud belirt
//   ./time_reader --demo "2025 365 23:59:59 +00"   # donanimsiz parse testi
//
// Durdurmak icin Ctrl+C.
// ============================================================================

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cerrno>
#include <csignal>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static volatile sig_atomic_t g_run = 1;
static void onSigint(int) { g_run = 0; }

static int baudConst(int b)
{
    switch (b) {
        case 300: return B300;      case 600: return B600;
        case 1200: return B1200;    case 2400: return B2400;
        case 4800: return B4800;    case 9600: return B9600;
        case 19200: return B19200;  case 38400: return B38400;
        case 57600: return B57600;  case 115200: return B115200;
        default: return B9600;
    }
}

static int openPort(const std::string& dev, int baud)
{
    int fd = open(dev.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) { fprintf(stderr, "Port acilamadi (%s): %s\n", dev.c_str(), strerror(errno)); return -1; }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) { fprintf(stderr, "tcgetattr: %s\n", strerror(errno)); close(fd); return -1; }

    int bc = baudConst(baud);
    cfsetispeed(&tty, bc);
    cfsetospeed(&tty, bc);

    tty.c_cflag &= ~PARENB;   // parity yok
    tty.c_cflag &= ~CSTOPB;   // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;       // 8 data bit
    tty.c_cflag &= ~CRTSCTS;  // donanim akis kontrolu yok
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~(OPOST | ONLCR);
    tty.c_cc[VTIME] = 10;
    tty.c_cc[VMIN] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) { fprintf(stderr, "tcsetattr: %s\n", strerror(errno)); close(fd); return -1; }
    tcflush(fd, TCIOFLUSH);
    return fd;
}

// Bir satir oku ('\n' gorene ya da timeout'a kadar)
static std::string readLine(int fd, int timeout_ms)
{
    std::string resp;
    char buf[256];
    struct timeval start, now, tv;
    gettimeofday(&start, nullptr);

    while (g_run) {
        gettimeofday(&now, nullptr);
        long el = (now.tv_sec - start.tv_sec) * 1000 + (now.tv_usec - start.tv_usec) / 1000;
        long rem = timeout_ms - el;
        if (rem <= 0) break;

        tv.tv_sec = rem / 1000;
        tv.tv_usec = (rem % 1000) * 1000;
        fd_set rs; FD_ZERO(&rs); FD_SET(fd, &rs);
        int r = select(fd + 1, &rs, nullptr, nullptr, &tv);
        if (r < 0) { if (errno == EINTR) continue; break; }
        if (r == 0) break;

        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            resp += buf;
            if (resp.find('\n') != std::string::npos) break;
        } else if (n < 0 && errno != EAGAIN) break;
    }
    while (!resp.empty() && (resp.back() == '\n' || resp.back() == '\r')) resp.pop_back();
    return resp;
}

static bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }

static void doyToDate(int y, int doy, int& mo, int& da)
{
    static const int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int rem = doy;
    for (int m = 0; m < 12; m++) {
        int d = dim[m] + ((m == 1 && isLeap(y)) ? 1 : 0);
        if (rem <= d) { mo = m + 1; da = rem; return; }
        rem -= d;
    }
    mo = 0; da = 0;
}

// Bir satiri parse edip alanlari yazdirir
static void printLine(const std::string& line)
{
    printf("  Ham metin : \"%s\"\n", line.c_str());

    printf("  Ham hex   : ");
    for (unsigned char c : line) printf("%02X ", c);
    printf("\n");

    // Beklenen: "YYYY DDD HH:MM:SS DZZ"   (DZZ opsiyonel, or. +00 / +03)
    std::vector<std::string> tok;
    { std::string t; for (char c : line) { if (c == ' ' || c == '\t') { if (!t.empty()) { tok.push_back(t); t.clear(); } } else t += c; } if (!t.empty()) tok.push_back(t); }

    if (tok.size() >= 3) {
        int year = atoi(tok[0].c_str());
        int doy  = atoi(tok[1].c_str());
        int hh = 0, mm = 0, ss = 0;
        sscanf(tok[2].c_str(), "%d:%d:%d", &hh, &mm, &ss);
        std::string tz = (tok.size() >= 4) ? tok[3] : "(yok)";

        int mo = 0, da = 0;
        doyToDate(year, doy, mo, da);

        printf("  --------\n");
        printf("  YIL       : %d\n", year);
        printf("  DDD (gun) : %d  -> %04d/%02d/%02d\n", doy, year, mo, da);
        printf("  SAAT      : %02d:%02d:%02d\n", hh, mm, ss);
        printf("  TZ alani  : %s   <-- +00 ise UTC, +03 ise yerel (GMT+3)\n", tz.c_str());
    } else {
        printf("  (Beklenen 'YYYY DDD HH:MM:SS [TZ]' formatinda degil - %zu token)\n", tok.size());
    }
    printf("\n");
}

int main(int argc, char** argv)
{
    std::string dev = "/dev/ttyUSB0";
    int baud = 9600;

    // --demo modu: donanim olmadan parse/print testi
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--demo" && i + 1 < argc) {
            printf("=== DEMO (donanimsiz parse testi) ===\n");
            printLine(argv[i + 1]);
            return 0;
        }
    }

    if (argc >= 2) dev = argv[1];
    if (argc >= 3) baud = atoi(argv[2]);

    signal(SIGINT, onSigint);

    printf("Port: %s @ %d baud  (durdurmak icin Ctrl+C)\n\n", dev.c_str(), baud);
    int fd = openPort(dev, baud);
    if (fd < 0) return 1;

    unsigned long count = 0;
    while (g_run) {
        std::string line = readLine(fd, 2000);
        if (line.empty()) { printf("  (2 sn veri yok, bekleniyor...)\n"); continue; }

        // alis zamani (host yerel saati - referans icin)
        struct timeval tv; gettimeofday(&tv, nullptr);
        struct tm lt; localtime_r(&tv.tv_sec, &lt);
        printf("[#%lu  alindi %02d:%02d:%02d host-yerel]\n", ++count, lt.tm_hour, lt.tm_min, lt.tm_sec);
        printLine(line);
    }

    close(fd);
    printf("\nDurduruldu. Toplam %lu satir okundu.\n", count);
    return 0;
}
