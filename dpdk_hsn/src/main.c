/**
 * @file main.c
 * @brief HSN entry point: parse --daemon, optionally fork, then hsn_run().
 *
 * All HSN logic lives in HsnRun.c (EAL + port setup + workers + stats).
 * The legacy DTN worker code remains in the tree but is not called here.
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Helpers.h"   /* helper_set_daemon_mode, force_quit */
#include "HsnRun.h"

/* Remove --daemon / -d from argv (so EAL doesn't see it); return if present. */
static bool check_and_remove_daemon_flag(int *argc, char const *argv[])
{
    bool found = false;
    int n = 0;
    for (int i = 0; i < *argc; i++) {
        if (strcmp(argv[i], "--daemon") == 0 || strcmp(argv[i], "-d") == 0)
            found = true;
        else
            argv[n++] = argv[i];
    }
    *argc = n;
    return found;
}

int main(int argc, char const *argv[])
{
    bool daemon_mode = check_and_remove_daemon_flag(&argc, argv);
    helper_set_daemon_mode(daemon_mode);

    printf("=== HSN DPDK Test (Ethernet-only, full burst, 2048 VLs) ===\n");
    printf("Mode: %s\n", daemon_mode ? "DAEMON" : "FOREGROUND");

    if (daemon_mode) {
        /* Fork to background so a remote SSH launcher can detach. */
        fflush(stdout);
        fflush(stderr);
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
        } else if (pid > 0) {
            printf("HSN running in background (PID %d), log: /tmp/dpdk_app.log\n", pid);
            _exit(0);
        } else {
            setsid();
            int fd = open("/tmp/dpdk_app.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd >= 0) {
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
            close(STDIN_FILENO);
            (void)open("/dev/null", O_RDONLY);
        }
    }

    return hsn_run(argc, argv);
}
