
package info.guardianproject.iocipher;

/**
 * A virtual file system container. Open and mount a virtual file system
 * container backed by a SQLCipher database for full encrypted file storage.
 */
public class VirtualFileSystem {

    /**
     * Empty dbFile results in an in memory database
     */
    private static String dbFileName = "";

    static {
        System.loadLibrary("stlport_shared");
        System.loadLibrary("sqlcipher_android");
        System.loadLibrary("iocipher");
    }

    /**
     * Create a virtual file system container
     *
     * @param file the physical disk file that will contain the container
     * @throws IllegalArgumentException
     */
    public VirtualFileSystem(String file) throws IllegalArgumentException {
        if (file.equals(""))
            throw new IllegalArgumentException("blank file name not allowed!");
        java.io.File dir = new java.io.File(file).getParentFile();
        if (!dir.exists())
            throw new IllegalArgumentException(dir.getPath() + " does not exist!");
        if (!dir.isDirectory())
            throw new IllegalArgumentException(dir.getPath() + " is not a directory!");
        if (!dir.canWrite())
            throw new IllegalArgumentException("Cannot write to " + dir.getPath() + "!");
        dbFileName = file;
    }

    /**
     * Create a virtual file system container
     *
     * @param file the physical disk file that will contain the container
     * @throws IllegalArgumentException
     */
    public VirtualFileSystem(java.io.File file) throws IllegalArgumentException {
        this(file.getAbsolutePath());
    }

    /**
     * Open and mount an UNENCRYPTED virtual file system
     *
     * @throws IllegalArgumentException
     */
    public native void mount_unencrypted() throws IllegalArgumentException;

    /**
     * Open and mount a virtual file system container encrypted with the
     * provided key
     *
     * @param key the container's password
     * @throws IllegalArgumentException
     */
    public native void mount(String key) throws IllegalArgumentException;

    /**
     * Unmount the file system.
     */
    public native void unmount();

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
