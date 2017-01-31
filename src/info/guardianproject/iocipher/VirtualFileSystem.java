
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
        System.loadLibrary("sqlcipher");
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
     * Create a new VFS container file at the path given from
     * {@link #setContainerPath(String)} and {@code password}.
     * <p>
     * If the container path was not set does not exist, it will throw an
     * {@link IllegalArgumentException}.
     *
     * @param {@code password} the password to set in the new VFS container
     * @throws {@link IllegalArgumentException}
     */
    public native void createNewContainer(String password) throws IllegalArgumentException;

    /**
     * Create a new VFS container file at the path given in
     * {@link #setContainerPath(String)} and {@code key}.
     * <p>
     * If the container path was not set does not exist, it will throw an
     * {@link IllegalArgumentException}.
     *
     * @param {@code key} the raw AES key to set in the new VFS container
     * @throws {@link IllegalArgumentException}
     */
    public native void createNewContainer(byte[] key) throws IllegalArgumentException;

    /**
     * Create a new VFS container file at the path given in
     * {@link #setContainerPath(String)} and {@code key}.
     * <p>
     * If the container path was not set does not exist, it will throw an
     * {@link IllegalArgumentException}.
     *
     * @param {@code key} the raw AES key to set in the new VFS container
     * @throws {@link IllegalArgumentException}
     */
    public void createNewContainer(SecretKey key) throws IllegalArgumentException {
        createNewContainer(key.getEncoded());
    }

    /**
     * Create a new VFS container file using the path and password given.
     * <p>
     * If the container path given does not exist or is not read/write, it will
     * throw an {@link IllegalArgumentException}.
     *
     * @param {@code containerPath} the physical disk file that serves as the
     *        VFS container
     * @throws {@link IllegalArgumentException} if the containing directory does
     *         not exist or is not readable
     */
    public void createNewContainer(String containerPath, String password)
            throws IllegalArgumentException {
        setContainerPath(containerPath);
        createNewContainer(password);
    }

    /**
     * Create a new VFS container file using the path and key given.
     * <p>
     * If the container path given does not exist or is not read/write, it will
     * throw an {@link IllegalArgumentException}.
     *
     * @param {@code containerPath} the physical disk file that serves as the
     *        VFS container
     * @param {@code key} the raw AES key to set in the new VFS container
     * @throws {@link IllegalArgumentException} if the containing directory does
     *         not exist or is not readable
     */
    public void createNewContainer(String containerPath, byte[] key)
            throws IllegalArgumentException {
        setContainerPath(containerPath);
        createNewContainer(key);
    }

    /**
     * Create a new VFS container file using the path and key given.
     * <p>
     * If the container path given does not exist or is not read/write, it will
     * throw an {@link IllegalArgumentException}.
     *
     * @param {@code containerPath} the physical disk file that serves as the
     *        VFS container
     * @param {@code key} the raw AES key to set in the new VFS container
     * @throws {@link IllegalArgumentException} if the containing directory does
     *         not exist or is not readable
     */
    public void createNewContainer(String containerPath, SecretKey key)
            throws IllegalArgumentException {
        setContainerPath(containerPath);
        createNewContainer(key);
    }

    /**
     * Delete all files associated with the container itself, i.e. the sqlfs
     * database, the SQLite WAL log file, and the SQLite SHM file.
     *
     * @return {@code true} if all related files were deleted, {@code false}
     *         otherwise
     */
    public boolean deleteContainer() {
        String containerPath = getContainerPath();
        if (containerPath == null || containerPath.equals(""))
            throw new IllegalArgumentException("Container path is null or empty!");
        return deleteContainer(containerPath);
    }

    /**
     * Delete all files associated with the container itself, i.e. the sqlfs
     * database, the SQLite WAL log file, and the SQLite SHM file.
     *
     * @param containerPath the path to the sqlfs database file
     * @return {@code true} if all related files were deleted, {@code false}
     *         otherwise
     */
    public boolean deleteContainer(String containerPath) {
        boolean result;
        java.io.File container = new java.io.File(containerPath);
        java.io.File shm = new java.io.File(containerPath + "-shm");
        java.io.File wal = new java.io.File(containerPath + "-wal");
        result = container.delete();
        if (shm.exists())
            result = shm.delete() && result;
        if (wal.exists())
            result = wal.delete() && result;
        return result;
    }

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
     * If accessing an IOCipher container in any thread separate from where
     * {@link #mount(byte[])} was called, that thread will have its own IOCipher
     * state. For threads that do not quit when complete or are reused, like
     * Android's {@code AsyncTask} or any "thread pool" model, this method must
     * be called when each thread is finished using the container. Otherwise,
     * that thread will keep its connection to the container open, and it will
     * not be possible to {@link #unmount()} successfully.
     */
    public native void detachThread();

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
