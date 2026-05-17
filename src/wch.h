/**
 * wch.h - File and directory change notification
 * Summary: Portable file watcher emitting add, upd, and del events.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef KC_WCH_H
#define KC_WCH_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kc_wch kc_wch_t;

#define KC_WCH_OK      0
#define KC_WCH_ERROR  -1

#define KC_WCH_ADD     0
#define KC_WCH_UPD     1
#define KC_WCH_DEL     2

typedef struct {
    int type;
    const char *path;
} kc_wch_event_t;

/**
 * Open a file watcher on the given path.
 * @param path File or directory to watch.
 * @param recursive Non-zero to watch directories recursively.
 * @return Watcher context or NULL on failure.
 */
kc_wch_t *kc_wch_open(const char *path, int recursive);

/**
 * Poll for the next file change event.
 * @param w Watcher context.
 * @param ev Output event structure.
 * @param timeout_ms Max wait in milliseconds (-1 = infinite, 0 = no wait).
 * @return 1 on event, 0 on timeout, -1 on error.
 */
int kc_wch_poll(kc_wch_t *w, kc_wch_event_t *ev, int timeout_ms);

/**
 * Close a file watcher and release all resources.
 * @param w Watcher context (NULL safe).
 * @return None.
 */
void kc_wch_close(kc_wch_t *w);

#ifdef __cplusplus
}
#endif

#endif
