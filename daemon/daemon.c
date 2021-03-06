/*
 * Created 190126 lynnl
 */

#include "_u_char.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>             /* memset(3) */
#include <unistd.h>             /* close(2) */
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <sys/errno.h>          /* errno */
#include <sys/kern_control.h>   /* sockaddr_ctl  ctl_info */
#include <sys/socket.h>         /* socket(2) */
#include <sys/sys_domain.h>     /* SYSPROTO_CONTROL */
#include <sys/ioctl.h>          /* ioctl(2) */
#include <sys/select.h>         /* select(2) */

#define TIMESTR_SZ 32

static const char *timestr(void)
{
    static char str[TIMESTR_SZ];
    struct timeval tv;
    struct tm *t;

    (void) gettimeofday(&tv, NULL);     /* Won't fail */
    t = localtime(&tv.tv_sec);
    *str = '\0';
    if (t != NULL) {
        (void)
        snprintf(str, TIMESTR_SZ, "%2d/%02d/%02d %02d:%02d:%02d.%04d%+05ld",
            (1900 + t->tm_year) % 100, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec / 100,
            t->tm_gmtoff * 100 / 3600);
    }

    return str;
}

#define LOG(fmt, ...)           printf("%s " fmt "\n", timestr(), ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)       LOG("[ERR] " fmt, ##__VA_ARGS__)
#ifdef DEBUG
#define LOG_DBG(fmt, ...)       LOG("[DBG] " fmt, ##__VA_ARGS__)
#else
#define LOG_DBG(fmt, ...)       (void) ((void) 0, ##__VA_ARGS__)
#endif

#define KERN_CTL_NAME           "cn.junkman.kext.xnu_printf_prec_panic.kernctl"

/**
 * Connect to a kernel control socket
 * @kctlname    kernel control name
 * @return      file descriptor  -1 if fail(errno will set)
 */
int connect_to_kern_ctl(const char *kctlname)
{
    int e;
    int fd;
    struct sockaddr_ctl sc;
    struct ctl_info ci;

    fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd == -1) {
        LOG_ERR("socket(2) fail  errno: %d", errno);
        goto out_exit;
    }

    LOG_DBG("kernel control  fd: %d", fd);

    memset(&ci, 0, sizeof(struct ctl_info));
    strncpy(ci.ctl_name, kctlname, MAX_KCTL_NAME-1);
    ci.ctl_name[MAX_KCTL_NAME-1] = '\0';

    /*
     * The kernel control ID is dynamically generated
     *  we can obtain ci.sc_id via ioctl(2)
     * If ioctl(2) returns ENOENT  it means control name doesn't exist
     *  we should load kext before run the daemon
     */
    e = ioctl(fd, CTLIOCGINFO, &ci);
    if (e == -1) {
        LOG_ERR("ioctl(2) fail  errno: %d", errno);
        goto out_fail;
    }

    LOG_DBG("kernel control  name: %s id: %d", ci.ctl_name, ci.ctl_id);

    memset(&sc, 0, sizeof(struct sockaddr_ctl));
    sc.sc_len     = sizeof(struct sockaddr_ctl);
    sc.sc_family  = AF_SYSTEM;
    sc.ss_sysaddr = AF_SYS_CONTROL;
    sc.sc_id      = ci.ctl_id;
    sc.sc_unit    = 0;

    e = connect(fd, (struct sockaddr *) &sc, sizeof(struct sockaddr_ctl));
    if (e == -1) {
        LOG_ERR("connect(2) fail  fd: %d errno: %d", fd, errno);
        goto out_fail;
    }

    LOG("connected  name: %s id: %d fd: %d", ci.ctl_name, ci.ctl_id, fd);

out_exit:
    return fd;

out_fail:
    (void) close(fd);
    fd = -1;
    goto out_exit;
}

#define MAX_KERN_CTL_MSG_SIZE           2048

/**
 * Send setsockopt(2) message to kernel control socket
 * @fd          socket descriptor
 * @type        option
 * @msg         message
 * @len         message length
 * @return      0 if success  -1 o.w.(errno will set)
 */
int setsockopt_to_kern_ctl(int fd, int type, const char *msg, size_t len)
{
    int e;

    if (msg == NULL || len == 0) {
        /* Prevent potential programmatic error */
        assert(msg == NULL);
        assert(len == 0);
    }

    if (len > MAX_KERN_CTL_MSG_SIZE) {
        e = -1;
        errno = EMSGSIZE;
        LOG_ERR("message too long  fd: %d type: %d len: %zu", fd, type, len);
        goto out_exit;
    }

    e = setsockopt(fd, SYSPROTO_CONTROL, type, (void *) msg, (socklen_t) len);
    if (e == -1) {
        LOG_ERR("setsockopt(2) fail  fd: %d type: %d len: %zu", fd, type, len);
    }

out_exit:
    return e;
}

/* XXX: should only used for `char[]'  NOT `char *' */
#define STRLEN(s)           (sizeof(s) - 1)

static int ms_sleep(unsigned int ms)
{
    struct timeval tv = {ms / 1000, (ms % 1000) * 1000};
    return select(0, NULL, NULL, NULL, &tv);
}

int main(void)
{
    int fd;
    char junk[] = "hypothesis";

    srand(time(NULL));

    fd = connect_to_kern_ctl(KERN_CTL_NAME);
    if (fd < 0) exit(EXIT_FAILURE);

    while (1) {
        if (setsockopt_to_kern_ctl(fd, rand() % 10000, junk, STRLEN(junk))) {
            break;
        }
        (void) ms_sleep(10);
    }

    (void) close(fd);
    return 0;
}

