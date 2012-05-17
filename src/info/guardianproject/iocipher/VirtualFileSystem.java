package info.guardianproject.iocipher;


public class VirtualFileSystem implements Comparable<VirtualFileSystem> {

	private String dbFile = "";

	static {
		System.loadLibrary("iocipher");
	}

	public VirtualFileSystem(String file) throws IllegalArgumentException {
		if (file.equals(""))
			throw new IllegalArgumentException("blank file name not allowed!");
		if (file.equals(dbFile))
			throw new IllegalArgumentException(file + " is already open!");
		java.io.File dir = new java.io.File(file).getParentFile();
		if (!dir.exists())
			throw new IllegalArgumentException(dir.getPath() + " does not exist!");
		if (!dir.isDirectory())
			throw new IllegalArgumentException(dir.getPath() + " is not a directory!");
		if (!dir.canWrite())
			throw new IllegalArgumentException("Cannot write to " + dir.getPath() + "!");
		dbFile = file;
		init(dbFile);
	}

	public VirtualFileSystem(java.io.File file) throws IllegalArgumentException {
		this(file.getAbsolutePath());
	}

	private native void init(String dbFileName);

	public native void mount() throws IllegalArgumentException;

	public native void mount(String key) throws IllegalArgumentException;

	public native void unmount();

	public native boolean isMounted();

	@Override
	public int compareTo(VirtualFileSystem vfs) {
		return this.dbFile.compareTo(vfs.dbFile);
	}

}
