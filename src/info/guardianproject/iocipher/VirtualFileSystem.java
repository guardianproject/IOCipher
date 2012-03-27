package info.guardianproject.iocipher;

public class VirtualFileSystem {

	static {
		System.loadLibrary("iocipher");
	}

	public VirtualFileSystem (String dirname) {
		init(dirname);
	}

	public VirtualFileSystem (java.io.File dir) {
		this(dir.getAbsolutePath());
	}

	private native int init(String dbFileName);

	public native int mount();

	public native int mount(String key);

	public native int unmount();

	public native boolean isOpen();

}
