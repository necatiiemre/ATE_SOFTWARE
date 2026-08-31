#include "SafeShutdown.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ACTIONS 16

typedef struct {
    bool                in_use;
    const char         *name;
    int                 priority;
    shutdown_action_fn  action;
    void               *ctx;
} entry_t;

static entry_t g_entries[MAX_ACTIONS];
static volatile sig_atomic_t g_stop_requested;
static bool g_installed;

static void handle_signal(int sig)
{
    (void)sig;
    g_stop_requested = 1;
}

void safe_shutdown_install(void)
{
    if (g_installed)
        return;

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    /* No SA_RESTART: a blocking read returns EINTR so the caller notices. */
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    atexit(safe_shutdown_execute);
    g_installed = true;
}

bool safe_shutdown_requested(void)
{
    return g_stop_requested != 0;
}

void safe_shutdown_clear(void)
{
    g_stop_requested = 0;
}

int safe_shutdown_register(const char *name, int priority,
                           shutdown_action_fn action, void *ctx)
{
    for (int i = 0; i < MAX_ACTIONS; i++)
        if (!g_entries[i].in_use) {
            g_entries[i] = (entry_t){true, name, priority, action, ctx};
            return i;
        }
    return -1;
}

void safe_shutdown_unregister(int handle)
{
    if (handle >= 0 && handle < MAX_ACTIONS)
        g_entries[handle].in_use = false;
}

void safe_shutdown_execute(void)
{
    /* Repeatedly take the highest-priority entry left, so an action that
     * unregisters itself cannot make us skip another. */
    for (;;) {
        int best = -1;
        for (int i = 0; i < MAX_ACTIONS; i++)
            if (g_entries[i].in_use &&
                (best < 0 || g_entries[i].priority > g_entries[best].priority))
                best = i;
        if (best < 0)
            return;

        entry_t entry = g_entries[best];
        g_entries[best].in_use = false;
        fprintf(stderr, "[shutdown] releasing %s\n", entry.name);
        entry.action(entry.ctx);
    }
}
