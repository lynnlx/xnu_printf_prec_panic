/*
 * Created 190126 lynnl
 */

#ifndef XNU_PRINTF_PREC_PANIC_H
#define XNU_PRINTF_PREC_PANIC_H

#include <libkern/libkern.h>
#include <kern/debug.h>

#ifndef __kext_makefile__
#define __TS__          "????/??/?? ??:??:??+????"
#define KEXTNAME_S      "xnu_printf_prec_panic"
#define BUNDLEID_S      "cn.junkman.kext." KEXTNAME_S
#endif

/*
 * Used to indicate unused function parameters
 * see: <sys/cdefs.h>#__unused
 */
#define UNUSED(arg0, ...)   (void) ((void) arg0, ##__VA_ARGS__)

#define LOG(fmt, ...)        printf(KEXTNAME_S ": " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)    LOG("[ERR] " fmt, ##__VA_ARGS__)
#define LOG_BUG(fmt, ...)    LOG("[BUG] " fmt, ##__VA_ARGS__)
#define LOG_OFF(fmt, ...)    UNUSED(0, ##__VA_ARGS__)
#ifdef DEBUG
#define LOG_DBG(fmt, ...)    LOG("[DBG] " fmt, ##__VA_ARGS__)
#else
#define LOG_DBG(fmt, ...)    LOG_OFF(fmt, ##__VA_ARGS__)
#endif

#define panicf(fmt, ...)                \
    panic("\n" fmt "\n%s@%s#L%d\n\n",   \
            ##__VA_ARGS__, __BASE_FILE__, __FUNCTION__, __LINE__)

#ifdef DEBUG
#define kassert(ex) (ex) ? (void) 0 : panicf("Assert `%s' failed", #ex)

#define kassertf(ex, fmt, ...) \
    (ex) ? (void) 0 : panicf("Assert `%s' failed: " fmt, #ex, ##__VA_ARGS__)
#else
#define kassert(ex) (ex) ? (void) 0 : LOG_BUG("Assert `%s' failed", #ex)

#define kassertf(ex, fmt, ...) \
    (ex) ? (void) 0 : LOG_BUG("Assert `%s' failed: " fmt, #ex, ##__VA_ARGS__)
#endif

#endif      /* XNU_PRINTF_PREC_PANIC_H */

