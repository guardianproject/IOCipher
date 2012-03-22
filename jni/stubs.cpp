
#include <pwd.h>

/* from bionic/libc/bionic/stubs.c */

int getpwnam_r(const char* name, struct passwd* pwd,
        char* buf, size_t byte_count, struct passwd** result)
{
    return do_getpw_r(1, name, -1, pwd, buf, byte_count, result);
}

int getpwuid_r(uid_t uid, struct passwd* pwd,
        char* buf, size_t byte_count, struct passwd** result)
{
    return do_getpw_r(0, NULL, uid, pwd, buf, byte_count, result);
}
