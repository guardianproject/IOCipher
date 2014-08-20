
package info.guardianproject.iocipher;

import android.text.TextUtils;

import java.io.IOException;

import javax.crypto.SecretKey;

/**
 * A virtual file system container. Open and mount a virtual file system
 * container backed by a SQLCipher database for full encrypted file storage.
 */
public class VirtualFileSystem {

    /**
     * Empty dbFile results in an in memory database
     */
    private static String dbFileName = "";
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
     * @param containerPath the physical disk file that serves as the VFS
     *            container
     * @throws IllegalArgumentException
     */
    public void setContainerPath(String containerPath) {
        if (TextUtils.isEmpty(containerPath))
            throw new IllegalArgumentException("blank file name not allowed!");
        java.io.File file = new java.io.File(containerPath);
        java.io.File dir = file.getParentFile();
        if (!dir.exists())
            throw new IllegalArgumentException(dir.getPath() + " does not exist!");
        if (!dir.isDirectory())
            throw new IllegalArgumentException(dir.getPath() + " is not a directory!");
        if (!dir.canWrite())
            throw new IllegalArgumentException("Cannot write to " + dir.getPath() + "!");
        try {
            dbFileName = file.getCanonicalPath();
        } catch (IOException e) {
            e.printStackTrace();
            dbFileName = file.getAbsolutePath();
        }
    }

    /**
     * Open and mount an UNENCRYPTED virtual file system
     *
     * @throws IllegalArgumentException
     */
    public native void mount_unencrypted() throws IllegalArgumentException;

    /**
     * Open and mount a virtual file system container encrypted with the
     * provided key as a {@code String}
     *
     * @param key the container's password
     * @throws IllegalArgumentException
     */
    public native void mount(String key) throws IllegalArgumentException;

    /**
     * Open and mount a virtual file system container encrypted with the
     * provided key. This method only accepts ASCII characters, if you need to
     * send a Unicode string, use {@link #mount(String)}
     *
     * @param {@code key} the container's password as a {@code byte[]} so it can
     *        be zeroed out when it is no longer needed
     * @throws IllegalArgumentException
     */
    public native void mount(byte[] key) throws IllegalArgumentException;

    /**
     * Open and mount a virtual file system container encrypted with the
     * provided key.
     *
     * @param {@code key} the container's password
     * @throws IllegalArgumentException
     */
    public void mount(SecretKey key) {
        mount(key.getEncoded());
    }

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
