/*
 * Copyright 2011-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/* for secure_getenv */
#define _GNU_SOURCE
#include <e_os.h>
#include <openssl/err.h>
#ifdef OPENSSL_FIPS
# include <openssl/fips.h>
# include <openssl/rand.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
# include <errno.h>
# include <stdlib.h>
# include "internal/fips_int.h"

# define FIPS_MODE_SWITCH_FILE "/proc/sys/crypto/fips_enabled"

static void init_fips_mode(void)
{
    char buf[2] = "0";
    int fd;

    /* Ensure the selftests always run */
    /* XXX: TO SOLVE - premature initialization due to selftests */
    FIPS_mode_set(1);

    if (secure_getenv("OPENSSL_FORCE_FIPS_MODE") != NULL) {
        buf[0] = '1';
    } else if ((fd = open(FIPS_MODE_SWITCH_FILE, O_RDONLY)) >= 0) {
        while (read(fd, buf, sizeof(buf)) < 0 && errno == EINTR) ;
        close(fd);
    }
    /* Failure reading the fips mode switch file means just not
     * switching into FIPS mode. We would break too many things
     * otherwise..
     */

    if (buf[0] != '1') {
        /* drop down to non-FIPS mode if it is not requested */
        FIPS_mode_set(0);
    } else {
        /* abort if selftest failed */
        FIPS_selftest_check();
    }
}

/*
 * Perform FIPS module power on selftest and automatic FIPS mode switch.
 */

void
#ifndef OPENSSL_SYS_UEFI
__attribute__ ((constructor))
#endif
OPENSSL_init_library(void)
{
    static int done = 0;
    if (done)
        return;
    done = 1;
    if (!FIPS_module_installed()) {
        return;
    }
    init_fips_mode();
}
#endif

/*
 * Perform any essential OpenSSL initialization operations. Currently only
 * sets FIPS callbacks
 */

void OPENSSL_init(void)
{
    static int done = 0;
    if (done)
        return;
    done = 1;
#ifdef OPENSSL_FIPS
    FIPS_set_locking_callbacks(CRYPTO_lock, CRYPTO_add_lock);
    FIPS_set_error_callbacks(ERR_put_error, ERR_add_error_vdata);
    FIPS_set_malloc_callbacks(CRYPTO_malloc, CRYPTO_free);
    RAND_init_fips();
#endif
}
