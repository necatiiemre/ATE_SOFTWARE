#include "Logger.h"
#include "Log.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <vector>

namespace logx {

namespace {

// Lines starting with this byte are file-only (see fileOnly()).
constexpr char kFileOnlyMarker = '\x1e';

std::FILE*       g_file = nullptr;
int              g_file_fd = -1;
int              g_orig_stdout = -1;
int              g_orig_stderr = -1;
int              g_pipe_read = -1;
std::thread      g_tee_thread;
std::atomic<bool> g_active{false};
std::string      g_dir;
std::string      g_path;
std::vector<std::string> g_secrets;

std::string clockTime()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);
    const int ms = static_cast<int>(
        duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "[%02d:%02d:%02d.%03d] ",
                  tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
    return buf;
}

void writeAll(int fd, const char* data, size_t len)
{
    while (len > 0) {
        ssize_t n = ::write(fd, data, len);
        if (n < 0) {
            if (errno == EINTR) continue;
            return;
        }
        data += n;
        len -= static_cast<size_t>(n);
    }
}

// Reads the stdout/stderr pipe; console bytes pass through untouched, the
// file gets a cleaned, timestamped copy.
void teeLoop()
{
    bool line_start = true;     // next byte begins a new line
    bool file_only  = false;    // current line is file-only
    bool pending_ts = false;    // timestamp not yet written for this line
    int  esc_state  = 0;        // 0 none, 1 after ESC, 2 inside CSI

    std::vector<char> buf(8192);
    std::string console_out;
    std::string file_out;

    while (true) {
        ssize_t n = ::read(g_pipe_read, buf.data(), buf.size());
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;      // all writers closed -> stop()

        console_out.clear();
        file_out.clear();

        for (ssize_t i = 0; i < n; ++i) {
            const char c = buf[static_cast<size_t>(i)];

            if (line_start) {
                line_start = false;
                if (c == kFileOnlyMarker) {
                    file_only = true;
                    file_out += clockTime() + "| ";
                    continue;
                }
                file_only = false;
                pending_ts = true;
            }

            if (!file_only) console_out += c;

            // ---- file copy: strip ANSI escape sequences and '\r' ----
            if (esc_state == 1) { esc_state = (c == '[') ? 2 : 0; continue; }
            if (esc_state == 2) { if (c >= 0x40 && c <= 0x7e) esc_state = 0; continue; }
            if (c == '\x1b') { esc_state = 1; continue; }
            if (c == '\r') continue;

            if (c == '\n') {
                file_out += '\n';
                line_start = true;
                pending_ts = false;
                continue;
            }
            if (pending_ts) {
                file_out += clockTime();
                pending_ts = false;
            }
            file_out += c;
        }

        if (!console_out.empty() && g_orig_stdout >= 0) {
            writeAll(g_orig_stdout, console_out.data(), console_out.size());
        }
        if (!file_out.empty() && g_file) {
            std::fwrite(file_out.data(), 1, file_out.size(), g_file);
            std::fflush(g_file);
        }
    }
}

// Ctrl+C / kill outside a remote program: leave a trace in the log.
void onFatalSignal(int sig)
{
    if (g_file_fd >= 0) {
        const char msg[] = "\n*** RUN INTERRUPTED BY SIGNAL - log may be incomplete ***\n";
        (void)!::write(g_file_fd, msg, sizeof(msg) - 1);
    }
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

void flushStreams()
{
    std::cout.flush();
    std::cerr.flush();
    std::fflush(stdout);
    std::fflush(stderr);
}

} // namespace

std::string timestamp()
{
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

std::string fileSafeTimestamp()
{
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &tm);
    return buf;
}

void addSecret(const std::string& secret)
{
    if (!secret.empty()) g_secrets.push_back(secret);
}

std::string mask(const std::string& text)
{
    std::string out = text;
    for (const auto& s : g_secrets) {
        size_t pos = 0;
        while ((pos = out.find(s, pos)) != std::string::npos) {
            out.replace(pos, s.size(), "****");
            pos += 4;
        }
    }
    return out;
}

bool start(const std::string& dir, const std::string& file, const std::string& header)
{
    if (g_active) return true;

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        std::cerr << "[WARN][LOG] Cannot create log directory " << dir << ": "
                  << ec.message() << " - continuing WITHOUT a log file" << std::endl;
        return false;
    }

    const std::string path = (std::filesystem::path(dir) / file).string();
    g_file = std::fopen(path.c_str(), "w");
    if (!g_file) {
        std::cerr << "[WARN][LOG] Cannot create log file " << path
                  << " - continuing WITHOUT a log file" << std::endl;
        return false;
    }
    g_file_fd = fileno(g_file);
    std::fputs(header.c_str(), g_file);
    std::fflush(g_file);

    int fds[2];
    if (::pipe(fds) != 0) {
        std::fclose(g_file);
        g_file = nullptr;
        g_file_fd = -1;
        std::cerr << "[WARN][LOG] pipe() failed - continuing WITHOUT a log file" << std::endl;
        return false;
    }

    flushStreams();
    g_orig_stdout = ::dup(STDOUT_FILENO);
    g_orig_stderr = ::dup(STDERR_FILENO);
    // Child processes must not inherit our private descriptors.
    ::fcntl(g_orig_stdout, F_SETFD, FD_CLOEXEC);
    ::fcntl(g_orig_stderr, F_SETFD, FD_CLOEXEC);
    ::fcntl(fds[0], F_SETFD, FD_CLOEXEC);
    ::fcntl(g_file_fd, F_SETFD, FD_CLOEXEC);

    ::dup2(fds[1], STDOUT_FILENO);
    ::dup2(fds[1], STDERR_FILENO);   // one pipe keeps stdout/stderr in order
    ::close(fds[1]);
    g_pipe_read = fds[0];

    // stdout is a pipe now; keep C stdio line-buffered like on a terminal.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    g_dir = dir;
    g_path = path;
    g_active = true;
    g_tee_thread = std::thread(teeLoop);

    std::signal(SIGINT, onFatalSignal);
    std::signal(SIGTERM, onFatalSignal);
    std::signal(SIGHUP, onFatalSignal);
    return true;
}

void stop()
{
    if (!g_active) return;

    flushStreams();
    // Restoring fd 1/2 closes the last write ends of the pipe -> EOF for
    // the tee thread once it has drained everything.
    ::dup2(g_orig_stdout, STDOUT_FILENO);
    ::dup2(g_orig_stderr, STDERR_FILENO);
    if (g_tee_thread.joinable()) g_tee_thread.join();

    ::close(g_orig_stdout);
    ::close(g_orig_stderr);
    ::close(g_pipe_read);
    g_orig_stdout = g_orig_stderr = g_pipe_read = -1;

    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
    std::signal(SIGHUP, SIG_DFL);

    g_file_fd = -1;
    std::fclose(g_file);
    g_file = nullptr;
    g_active = false;
}

bool active() { return g_active; }
std::string dir() { return g_active ? g_dir : std::string(); }
std::string filePath() { return g_active ? g_path : std::string(); }

void fileOnly(const std::string& text)
{
    if (!g_active) return;

    // Prefix every line with the marker and push it through the same pipe
    // as the console output so the order in the file is preserved.
    std::string out;
    std::istringstream in(mask(text));
    std::string line;
    while (std::getline(in, line)) {
        out += kFileOnlyMarker;
        out += line;
        out += '\n';
    }
    if (out.empty()) return;

    flushStreams();
    writeAll(STDOUT_FILENO, out.data(), out.size());
}

void detail(const std::string& text, bool console_ok)
{
    if (console_ok && logcfg::verbose) {
        // Console (and therefore the file, via the capture).
        std::cout << mask(text) << std::endl;
    } else {
        fileOnly(text);
    }
}

} // namespace logx
