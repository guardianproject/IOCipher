
#ifdef __cplusplus
extern "C" {
#endif

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

/* These functions seem to be included in bionic libc, but they remove
 * them from the headers provided with the NDK.  So these are just
 * grapped from the platform/bionic sources:
 * libc/include/sys/linux-unistd.h */
    int getpwnam_r(const char* name, struct passwd* pwd,
                   char* buf, size_t byte_count, struct passwd** result);
    int getpwuid_r(uid_t uid, struct passwd* pwd,
                   char* buf, size_t byte_count, struct passwd** result);

/* These functions have been included in bionic libc for at least
 * since android-8, but were not exposed, so we take the code from
 * libc/unistd/pread64.c and libc/unistd/pwrite64.c */
    extern int __pread64(int fd, void *buf, size_t nbytes, loff_t offset);
    ssize_t pread64(int fd, void *buf, size_t nbytes, off64_t offset)
    {
        return __pread64(fd, buf, nbytes, offset);
    }
    extern int __pwrite64(int fd, const void *buf, size_t nbytes, loff_t offset);
    ssize_t pwrite64(int fd, const void *buf, size_t nbytes, off64_t offset)
    {
        return __pwrite64(fd, buf, nbytes, offset);
    }

#ifdef __cplusplus
}
#endif
