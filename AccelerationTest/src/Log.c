#include "Log.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static FILE *g_file;
static char  g_path[512];

static void timestamp(char *out, size_t cap)
{
    struct timespec ts;
    struct tm tm;

    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm);
    snprintf(out, cap, "%02d:%02d:%02d.%03ld", tm.tm_hour, tm.tm_min, tm.tm_sec,
             ts.tv_nsec / 1000000L);
}

static bool make_path(const char *path)
{
    return mkdir(path, 0775) == 0 || errno == EEXIST;
}

bool log_open(const char *unit, const char *profile)
{
    char dir[400], stamp[32];
    struct tm tm;
    time_t now = time(NULL);

    localtime_r(&now, &tm);
    strftime(stamp, sizeof stamp, "%Y-%m-%d_%H-%M-%S", &tm);

    snprintf(dir, sizeof dir, "LOGS");
    if (!make_path(dir))
        return false;
    snprintf(dir, sizeof dir, "LOGS/%s", unit);
    if (!make_path(dir))
        return false;

    snprintf(g_path, sizeof g_path, "%s/%s_%s.log", dir, profile, stamp);
    g_file = fopen(g_path, "w");
    if (!g_file) {
        g_path[0] = '\0';
        return false;
    }
    return true;
}

const char *log_path(void)
{
    return g_path;
}

static void write_line(bool to_terminal, const char *fmt, va_list ap)
{
    char stamp[32], text[1024];

    timestamp(stamp, sizeof stamp);
    vsnprintf(text, sizeof text, fmt, ap);

    if (to_terminal)
        printf("%s  %s\n", stamp, text);
    if (g_file) {
        fprintf(g_file, "%s  %s\n", stamp, text);
        fflush(g_file);
    }
    if (to_terminal)
        fflush(stdout);
}

void log_line(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    write_line(true, fmt, ap);
    va_end(ap);
}

void log_file_only(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    write_line(false, fmt, ap);
    va_end(ap);
}

void log_close(void)
{
    if (g_file) {
        fclose(g_file);
        g_file = NULL;
    }
}
