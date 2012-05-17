package info.guardianproject.iocipher;

import java.io.IOException;

public class VirtualFileSystem implements Comparable<VirtualFileSystem> {

	private String dbFile = "";

	static {
		System.loadLibrary("iocipher");
	}

	public VirtualFileSystem(String file) throws IOException {
		if (file.equals(""))
			throw new IOException("blank file name not allowed!");
		if (file.equals(dbFile))
			throw new IOException(file + " is already open!");
		java.io.File dir = new java.io.File(file).getParentFile();
		if (!dir.exists())
			throw new IOException(dir.getPath() + " does not exist!");
		if (!dir.isDirectory())
			throw new IOException(dir.getPath() + " is not a directory!");
		if (!dir.canWrite())
			throw new IOException("Cannot write to " + dir.getPath() + "!");
		dbFile = file;
		init(dbFile);
	}

	public VirtualFileSystem(java.io.File file) throws IOException {
		this(file.getAbsolutePath());
	}

	private native void init(String dbFileName);

	public native void mount();

	public native void mount(String key);

	public native void unmount();

	public native boolean isMounted();

	@Override
	public int compareTo(VirtualFileSystem vfs) {
		return this.dbFile.compareTo(vfs.dbFile);
	}

}
