package info.guardianproject.iocipher;

public class VirtualFileSystem {

	public VirtualFileSystem (String dirname) {
		init(dirname);
	}

	public VirtualFileSystem (java.io.File dir) {
		this(dir.getAbsolutePath());
	}

	public native int init(String dbFileName);

	public native int open();

	public native int open(String key);

	public native int close();

	public native boolean isOpen();

}
