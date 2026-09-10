#define _GNU_SOURCE            /* fopencookie */

#include "Log.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static FILE *g_file;       /* the log */
static FILE *g_terminal;   /* the real stdout, kept while the tee is installed */
static FILE *g_tee;        /* what stdout is, between log_open and log_close */
static char  g_path[512];

/* The screen is redrawn in place, which means escape sequences. They are
 * instructions to a terminal, not something anyone printed, and in a file they
 * turn every redraw into a jumble. The terminal gets them; the file does not.
 * Everything else reaches both byte for byte. */
static void write_without_escapes(const char *data, size_t len, FILE *out)
{
    static bool in_escape;      /* a sequence can be split across two writes */
    size_t plain = 0;

    for (size_t i = 0; i < len; i++) {
        if (in_escape) {
            /* A CSI sequence ends at the first byte in 0x40..0x7E. */
            if ((unsigned char)data[i] >= 0x40 && (unsigned char)data[i] <= 0x7E)
                in_escape = false;
            plain = i + 1;
            continue;
        }
        if (data[i] == 0x1B) {
            if (i > plain)
                fwrite(data + plain, 1, i - plain, out);
            in_escape = true;
            plain = i + 1;
            /* Skip the '[' that introduces the parameters, if it is next. */
            if (i + 1 < len && data[i + 1] == '[') {
                i++;
                plain = i + 1;
            }
        }
    }
    if (len > plain)
        fwrite(data + plain, 1, len - plain, out);
}

/* Everything printed goes to both. The log is meant to be a transcript, so
 * rather than asking every caller to write twice, stdout itself becomes a
 * stream that writes twice - which also catches the parts of the output this
 * project did not write, like the health-monitor dashboards copied verbatim
 * from dpdk_vmc. */
static ssize_t tee_write(void *cookie, const char *data, size_t len)
{
    (void)cookie;

    if (g_terminal) {
        fwrite(data, 1, len, g_terminal);
        fflush(g_terminal);
    }
    if (g_file) {
        write_without_escapes(data, len, g_file);
        fflush(g_file);
    }
    return (ssize_t)len;
}

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

    /* Install the tee. If any of this fails the run still goes ahead - it just
     * writes to the terminal only, which is the same as having no log. */
    cookie_io_functions_t io = {.write = tee_write};

    g_terminal = stdout;
    g_tee = fopencookie(NULL, "w", io);
    if (!g_tee) {
        g_terminal = NULL;
        fclose(g_file);
        g_file = NULL;
        g_path[0] = '\0';
        return false;
    }
    /* Unbuffered, so the file and the screen stay in step and a run cut short
     * by the rig losing power still has everything up to that moment. */
    setvbuf(g_tee, NULL, _IONBF, 0);
    stdout = g_tee;
    return true;
}

const char *log_path(void)
{
    return g_path;
}

void log_line(const char *fmt, ...)
{
    char stamp[32], text[1024];
    va_list ap;

    timestamp(stamp, sizeof stamp);
    va_start(ap, fmt);
    vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);

    /* An ordinary print. The tee below puts it in the file too. */
    printf("%s  %s\n", stamp, text);
    fflush(stdout);
}

void log_close(void)
{
    if (!g_file)
        return;

    fflush(stdout);
    if (g_tee) {
        fclose(g_tee);          /* the stream we installed as stdout */
        g_tee = NULL;
    }
    if (g_terminal) {
        stdout = g_terminal;
        g_terminal = NULL;
    }
    fclose(g_file);
    g_file = NULL;
    g_path[0] = '\0';
}
