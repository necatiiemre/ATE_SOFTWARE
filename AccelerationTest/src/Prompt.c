#include "Prompt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool read_line(char *buf, size_t cap)
{
    if (!fgets(buf, (int)cap, stdin))
        return false;
    buf[strcspn(buf, "\n")] = '\0';
    return true;
}

int prompt_menu(const char *title, int min, int max, int quit_value)
{
    char line[64];

    for (;;) {
        printf("%s [%d-%d]: ", title, min, max);
        fflush(stdout);

        if (!read_line(line, sizeof line)) {
            putchar('\n');
            return quit_value;
        }
        if (line[0] == '\0')
            return quit_value;

        char *end;
        long value = strtol(line, &end, 10);
        while (*end == ' ' || *end == '\t')
            end++;

        if (*end != '\0' || end == line)
            printf("  '%s' is not a number.\n", line);
        else if (value < min || value > max)
            printf("  %ld is out of range.\n", value);
        else
            return (int)value;
    }
}

bool prompt_yes_no(const char *question, bool on_eof)
{
    char line[64];

    for (;;) {
        printf("%s [y/n]: ", question);
        fflush(stdout);

        if (!read_line(line, sizeof line)) {
            putchar('\n');
            return on_eof;
        }
        if (line[0] == 'y' || line[0] == 'Y')
            return true;
        if (line[0] == 'n' || line[0] == 'N')
            return false;
        printf("  Please answer y or n.\n");
    }
}

void prompt_pause(const char *message)
{
    char line[64];

    printf("%s", message);
    fflush(stdout);
    read_line(line, sizeof line);
}
