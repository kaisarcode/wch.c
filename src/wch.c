/**
 * wch.c - File and directory change notification CLI
 * Summary: Watches files/directories and emits add, upd, and del events.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#include "wch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define KC_WCH_VERSION "1.0.0"

static volatile int kc_wch_running = 1;

/**
 * Signal handler for SIGINT and SIGTERM.
 * @param sig Signal number.
 * @return None.
 */
static void kc_wch_signal(int sig) {
    (void)sig;
    kc_wch_running = 0;
}

/**
 * Print command usage information.
 * @param name Program executable name.
 * @return None.
 */
static void kc_print_help(const char *name) {
    printf("Usage: %s [options] <path>\n", name);
    printf("\n");
    printf("Options:\n");
    printf("  <path>         Path to file or directory\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s .\n", name);
    printf("  %s ./file.txt\n", name);
}

/**
 * Print version information.
 * @return None.
 */
static void kc_print_version(void) {
    printf("wch %s\n", KC_WCH_VERSION);
}

/**
 * Entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process status code.
 */
int main(int argc, char **argv) {
    const char *path = NULL;
    int i = 1;

    if (i < argc && argv[i][0] != '-') {
        path = argv[i++];
    }

    while (i < argc) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            kc_print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 ||
            strcmp(argv[i], "--version") == 0) {
            kc_print_version();
            return 0;
        } else if (argv[i][0] != '-') {
            path = argv[i];
        } else {
            fprintf(stderr, "wch: unknown option '%s'\n", argv[i]);
            return 1;
        }
        i++;
    }

    if (!path) {
        fprintf(stderr, "wch: missing path\n");
        return 1;
    }

    signal(SIGINT, kc_wch_signal);
    signal(SIGTERM, kc_wch_signal);

    kc_wch_t *w = kc_wch_open(path, 1);
    if (!w) {
        fprintf(stderr, "wch: failed to watch '%s'\n", path);
        return 1;
    }

    kc_wch_event_t ev;
    const char *labels[] = {"add", "upd", "del"};

    while (kc_wch_running) {
        int rc = kc_wch_poll(w, &ev, -1);
        if (rc < 0) {
            break;
        }
        if (rc == 0) {
            continue;
        }
        if (ev.type >= 0 && ev.type <= 2 && ev.path) {
            printf("%s:%s\n", labels[ev.type], ev.path);
            fflush(stdout);
        }
    }

    kc_wch_close(w);
    return 0;
}
