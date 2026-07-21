/**
 * test.c - libwch public API tests.
 * Summary: Tests each public libwch function through one CTest case.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#include "libwch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Verifies one integer result.
 * @param name Check description.
 * @param expected Expected value.
 * @param actual Actual value.
 * @return 0 on success, 1 on failure.
 */
static int expect_int(const char *name, int expected, int actual) {
    if (expected != actual) {
        printf("\033[31m[FAIL]\033[0m %s: expected %d, got %d\n", name, expected, actual);
        return 1;
    }
    printf("\033[32m[PASS]\033[0m %s\n", name);
    return 0;
}

/**
 * Verifies one boolean condition.
 * @param name Check description.
 * @param condition Non-zero when the check passed.
 * @return 0 on success, 1 on failure.
 */
static int expect_true(const char *name, int condition) {
    if (!condition) {
        printf("\033[31m[FAIL]\033[0m %s\n", name);
        return 1;
    }
    printf("\033[32m[PASS]\033[0m %s\n", name);
    return 0;
}

/**
 * Tests kc_wch_version.
 * @return 0 on success, 1 on failure.
 */
static int case_version(void) {
    kc_wch_version();
    return expect_true("version does not crash", 1);
}

/**
 * Tests kc_wch_options_default.
 * @return 0 on success, 1 on failure.
 */
static int case_options_default(void) {
    kc_wch_options_default();
    return expect_true("options_default does not crash", 1);
}

/**
 * Tests kc_wch_options_load_env.
 * @return 0 on success, 1 on failure.
 */
static int case_options_load_env(void) {
    kc_wch_options_t opts = {0};
    kc_wch_options_load_env(&opts);
    kc_wch_options_load_env(NULL);
    return expect_true("load_env does not crash", 1);
}

/**
 * Tests kc_wch_options_free.
 * @return 0 on success, 1 on failure.
 */
static int case_options_free(void) {
    kc_wch_options_t opts = {0};
    kc_wch_options_free(&opts);
    kc_wch_options_free(NULL);
    return expect_true("options_free does not crash", 1);
}

/**
 * Tests kc_wch_open and kc_wch_close.
 * @return 0 on success, 1 on failure.
 */
static int case_open_close(void) {
    kc_wch_t *w = NULL;
    kc_wch_options_t opts;
    int rc;

    rc = 0;
    opts = kc_wch_options_default();
    rc += expect_int("open(NULL, path, opts) returns ERROR", KC_WCH_ERROR, kc_wch_open(NULL, "/tmp", &opts));
    rc += expect_int("open(out, NULL, opts) returns ERROR", KC_WCH_ERROR, kc_wch_open(&w, NULL, &opts));
    rc += expect_int("open(out, nonexistent, opts) returns ERROR", KC_WCH_ERROR, kc_wch_open(&w, "/nonexistent/path", &opts));
    rc += expect_int("close(NULL) returns OK", KC_WCH_OK, kc_wch_close(NULL));
    return rc == 0 ? 0 : 1;
}

/**
 * Tests kc_wch_stop.
 * @return 0 on success, 1 on failure.
 */
static int case_stop(void) {
    kc_wch_t *w;
    kc_wch_options_t opts;
    int rc;

    rc = 0;
    rc += expect_int("stop(NULL) returns ERROR", KC_WCH_ERROR, kc_wch_stop(NULL));
    opts = kc_wch_options_default();
    if (kc_wch_open(&w, "/tmp", &opts) != KC_WCH_OK) return 1;
    rc += expect_int("stop returns OK", KC_WCH_OK, kc_wch_stop(w));
    rc += expect_int("stop is idempotent", KC_WCH_OK, kc_wch_stop(w));
    kc_wch_close(w);
    return rc == 0 ? 0 : 1;
}

/**
 * Tests kc_wch_poll error paths.
 * @return 0 on success, 1 on failure.
 */
static int case_poll(void) {
    kc_wch_event_t ev;
    int rc;

    rc = 0;
    rc += expect_int("poll(NULL) returns ERROR", -1, kc_wch_poll(NULL, &ev, 0));
    return rc == 0 ? 0 : 1;
}

/**
 * Tests two contexts coexist.
 * @return 0 on success, 1 on failure.
 */
static int case_multictx(void) {
    kc_wch_t *a;
    kc_wch_t *b;
    kc_wch_options_t opts;
    int rc;

    rc = 0;
    opts = kc_wch_options_default();
    if (kc_wch_open(&a, "/tmp", &opts) != KC_WCH_OK) return 1;
    if (kc_wch_open(&b, "/tmp", &opts) != KC_WCH_OK) {
        kc_wch_close(a);
        return 1;
    }
    rc += expect_int("stop a returns OK", KC_WCH_OK, kc_wch_stop(a));
    rc += expect_int("stop b returns OK", KC_WCH_OK, kc_wch_stop(b));
    rc += expect_int("close a returns OK", KC_WCH_OK, 0);
    kc_wch_close(a);
    rc += expect_int("close b returns OK", KC_WCH_OK, 0);
    kc_wch_close(b);
    return rc == 0 ? 0 : 1;
}

/**
 * Runs one libwch public API test case.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, 1 or 2 on failure.
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "test case: expected one argument, got %d\n", argc - 1);
        return 2;
    }
    if (strcmp(argv[1], "version") == 0) return case_version();
    if (strcmp(argv[1], "options-default") == 0) return case_options_default();
    if (strcmp(argv[1], "options-load-env") == 0) return case_options_load_env();
    if (strcmp(argv[1], "options-free") == 0) return case_options_free();
    if (strcmp(argv[1], "open-close") == 0) return case_open_close();
    if (strcmp(argv[1], "stop") == 0) return case_stop();
    if (strcmp(argv[1], "poll") == 0) return case_poll();
    if (strcmp(argv[1], "multictx") == 0) return case_multictx();
    fprintf(stderr, "unknown test case: %s\n", argv[1]);
    return 2;
}
