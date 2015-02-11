
package info.guardianproject.iocipher;

import javax.crypto.SecretKey;

/**
 * A virtual file system container. Open and mount a virtual file system
 * container backed by a SQLCipher database for full encrypted file storage.
 */
public class VirtualFileSystem {

    /**
     * Empty dbFile results in an in memory database
     */
    private static VirtualFileSystem vfs;

    static {
        System.loadLibrary("stlport_shared");
        System.loadLibrary("sqlcipher_android");
        System.loadLibrary("iocipher");
    }

    private VirtualFileSystem() {
        // singleton!
    }

    /**
     * Get the instance of the VirtualFileSystem.
     *
     * @return the single instance of VirtualFileSystem
     */
    public static VirtualFileSystem get() {
        if (vfs == null)
            vfs = new VirtualFileSystem();
        return vfs;
    }

    /**
     * Set the path to the container file that is the virtual file system. This
     * is specified as a {@code String} to avoid confusion between
     * {@link java.io.File} and {@link info.guardianproject.iocipher.File}.
     *
     * @param {@code containerPath} the physical disk file that serves as the
     *        VFS container
     * @throws {@link IllegalArgumentException} if the containing directory does
     *         not exist or is not readable
     */
    public native void setContainerPath(String containerPath) throws IllegalArgumentException;

    /**
     * Get the full path to the file used as the virtual file system container.
     *
     * @return the path to the VFS container file
     */
    public native String getContainerPath();

    /**
     * Open and mount a virtual file system container encrypted with the
     * provided password as a {@code String}. This {@code String} is then used
     * to derive the AES key using SQLCipher's key derivation method. This
     * method is the least preferred option because it is not possible to clear
     * the password from memory since {@code String} instances are immutable.
     * <p>
     * If the vfs is already mounted, it will throw an
     * {@link IllegalStateException}. If the file does not exist or the password
     * is wrong, it will throw an {@link IllegalArgumentException}.
     *
     * @param password the password to unlock the VFS container
     * @throws IllegalArgumentException, IllegalStateException
     */
    public native void mount(String password) throws IllegalArgumentException;

    /**
     * Open and mount a virtual file system container encrypted with the
     * provided password as a {@code String}. This {@code String} is then used
     * to derive the AES key using SQLCipher's key derivation method. This
     * method is the least preferred option because it is not possible to clear
     * the password from memory since {@code String} instances are immutable.
     * <p>
     * If the vfs is already mounted, it will throw an
     * {@link IllegalStateException}. If the file does not exist or the password
     * is wrong, it will throw an {@link IllegalArgumentException}.
     *
     * @param containerPath the path to the file to mount
     * @param password the password to unlock the VFS container
     * @throws IllegalArgumentException, IllegalStateException
     */
    public void mount(String containerPath, String password) {
        setContainerPath(containerPath);
        mount(password);
    }

    /**
     * Open and mount a virtual file system container encrypted with the
     * provided key. This expects the raw AES key as a {@link SecretKey}
     * instance here to be passed directly to the underlying database layer. It
     * is not a password that will then be used to derive a key, like with
     * {@link #mount(String)}. This method only accepts raw byte values, if you
     * need to send a Unicode string, use {@link #mount(String)}. Also, a
     * {@code byte[]} can be zeroed out when it is no longer needed. This method
     * and {@link #mount(SecretKey)} were designed to be used with the CacheWord
     * library.
     * <p>
     * If the vfs is already mounted, it will throw an
     * {@link IllegalStateException}. If the file does not exist or the key is
     * wrong, it will throw an {@link IllegalArgumentException}.
     *
     * @param {@code key} the container's raw AES key
     * @throws IllegalArgumentException, IllegalStateException
     */
    public native void mount(byte[] key) throws IllegalArgumentException;

    /**
     * Open and mount a virtual file system container encrypted with the
     * provided key. This expects the raw AES key as a {@link SecretKey}
     * instance here to be passed directly to the underlying database layer. It
     * is not a password that will then be used to derive a key, like with
     * {@link #mount(String)}. This method only accepts raw byte values, if you
     * need to send a Unicode string, use {@link #mount(String)}. Also, a
     * {@code byte[]} can be zeroed out when it is no longer needed. This method
     * and {@link #mount(SecretKey)} were designed to be used with the CacheWord
     * library.
     * <p>
     * If the vfs is already mounted, it will throw an
     * {@link IllegalStateException}. If the file does not exist or the key is
     * wrong, it will throw an {@link IllegalArgumentException}.
     *
     * @param containerPath the path to the file to mount
     * @param {@code key} the container's raw AES key
     * @throws IllegalArgumentException, IllegalStateException
     */
    public void mount(String containerPath, byte[] key) {
        setContainerPath(containerPath);
        mount(key);
    }

    /**
     * Open and mount a virtual file system container encrypted with the
     * provided key. This expects the raw AES key as a {@link SecretKey}
     * instance here to be passed directly to the underlying database layer. It
     * is not a password that will then be used to derive a key, like with
     * {@link #mount(String)}. This method and {@link #mount(byte[])} were
     * designed to be used with the CacheWord library.
     * <p>
     * If the vfs is already mounted, it will throw an
     * {@link IllegalStateException}. If the file does not exist or the key is
     * wrong, it will throw an {@link IllegalArgumentException}.
     *
     * @param {@code key} the container's raw AES key
     * @throws IllegalArgumentException, IllegalStateException
     */
    public void mount(SecretKey key) {
        mount(key.getEncoded());
    }

    /**
     * Open and mount a virtual file system container encrypted with the
     * provided key. This expects the raw AES key as a {@link SecretKey}
     * instance here to be passed directly to the underlying database layer. It
     * is not a password that will then be used to derive a key, like with
     * {@link #mount(String)}. This method and {@link #mount(byte[])} were
     * designed to be used with the CacheWord library.
     * <p>
     * If the vfs is already mounted, it will throw an
     * {@link IllegalStateException}. If the file does not exist or the key is
     * wrong, it will throw an {@link IllegalArgumentException}.
     *
     * @param containerPath the path to the file to mount
     * @param {@code key} the container's raw AES key
     * @throws IllegalArgumentException, IllegalStateException
     */
    public void mount(String containerPath, SecretKey key) {
        setContainerPath(containerPath);
        mount(key);
    }

    /**
     * Unmount the file system. It will throw an {@link IllegalStateException}
     * if the vfs is not mounted, or if it cannot be unmounted because it is
     * busy (some threads are still active on it).
     *
     * @throws IllegalStateException
     */
    public native void unmount() throws IllegalStateException;

    /**
     * @return whether the VFS is mounted or not
     */
    public native boolean isMounted();

    /**
     * Call this function before performance sensitive write operations to
     * increase performance
     */
    public native void beginTransaction();

    /**
     * Call this function after performance sensitive write operations complete
     */
    public native void completeTransaction();

}
