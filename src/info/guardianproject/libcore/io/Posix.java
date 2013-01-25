/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package info.guardianproject.libcore.io;

import info.guardianproject.iocipher.FileDescriptor;

import java.nio.ByteBuffer;

public final class Posix implements Os {

	Posix() {
	}

	// Implemented by libsqlfs
	public native boolean access(String path, int mode) throws ErrnoException;

	public native void chmod(String path, int mode) throws ErrnoException;

	public native void close(FileDescriptor fd) throws ErrnoException;

	public native void fsync(FileDescriptor fd) throws ErrnoException;

	public native void ftruncate(FileDescriptor fd, long length)
			throws ErrnoException;

	public native void link(String from, String to) throws ErrnoException;

	public native void mkdir(String path, int mode) throws ErrnoException;

	public native FileDescriptor open(String path, int flags, int mode)
			throws ErrnoException;

	private native int preadBytes(FileDescriptor fd, Object buffer, int bufferOffset, int byteCount, long offset) throws ErrnoException;

	public int pread(FileDescriptor fd, ByteBuffer buffer, long offset)
	throws ErrnoException {
		if (buffer.isDirect()) {
			return preadBytes(fd, buffer, buffer.position(), buffer.remaining(), offset);
		} else {
			return preadBytes(fd, buffer.array(),
					buffer.arrayOffset() + buffer.position(),
					buffer.remaining(), offset);
		}
	}

	public int pread(FileDescriptor fd, byte[] bytes, int byteOffset,
			int byteCount, long offset) throws ErrnoException {
        // This indirection isn't strictly necessary, but ensures that our public interface is type safe.
        return preadBytes(fd, bytes, byteOffset, byteCount, offset);
	}

	public int read(FileDescriptor fd, ByteBuffer buffer) throws ErrnoException {
		int ret;
		if (buffer.isDirect()) {
			ret = preadBytes(fd, buffer, buffer.position(), buffer.remaining(), 0);
		} else {
			ret = preadBytes(fd, buffer.array(),
					buffer.arrayOffset() + buffer.position(),
					buffer.remaining(), 0);
		}
		fd.position += ret;
		return ret;
	}

	public int read(FileDescriptor fd, byte[] bytes, int byteOffset,
			int byteCount) throws ErrnoException {
		int ret = preadBytes(fd, bytes, byteOffset, byteCount, fd.position);
		fd.position += byteCount;
		return ret;
	}

	public native void remove(String path) throws ErrnoException;

	public native void rename(String oldPath, String newPath)
			throws ErrnoException;

	public native void rmdir(String path) throws ErrnoException;

	public native StructStatFs statfs(String path) throws ErrnoException;

	public StructStatFs fstatfs(FileDescriptor fd) throws ErrnoException {
		// since statfs is faked anyway based on the path to the database file, just call statfs()
		return statfs("");
	}

	public native String strerror(int errno);

	public native void symlink(String oldPath, String newPath)
			throws ErrnoException;

	public native void unlink(String path) throws ErrnoException;

	public int write(FileDescriptor fd, ByteBuffer buffer, int flags)
			throws ErrnoException {
		int ret;
		if (buffer.isDirect()) {
			ret = pwriteBytes(fd, buffer, buffer.position(), buffer.remaining(), 0, flags);
		} else {
			ret = pwriteBytes(fd, buffer.array(),
					buffer.arrayOffset() + buffer.position(),
					buffer.remaining(), 0, flags);
		}
		fd.position += ret;
		return ret;
	}

	public int write(FileDescriptor fd, byte[] bytes, int byteOffset,
			int byteCount, int flags) throws ErrnoException {
		int ret = pwriteBytes(fd, bytes, byteOffset, byteCount, fd.position, flags);
		fd.position += byteCount;
		return ret;
	}

	public int pwrite(FileDescriptor fd, ByteBuffer buffer, long offset, int flags)
	throws ErrnoException {
		if (buffer.isDirect()) {
			return pwriteBytes(fd, buffer, buffer.position(), buffer.remaining(), offset, flags);
		} else {
			return pwriteBytes(fd, buffer.array(),
					buffer.arrayOffset() + buffer.position(),
					buffer.remaining(), offset, flags);
		}
	}

	public int pwrite(FileDescriptor fd, byte[] bytes, int byteOffset,
			int byteCount, long offset, int flags) throws ErrnoException {
		// This indirection isn't strictly necessary, but ensures that our
		// public interface is type safe.
		return pwriteBytes(fd, bytes, byteOffset, byteCount, offset, flags);
	}

	private native int pwriteBytes(FileDescriptor fd, Object buffer, int bufferOffset,
			int byteCount, long offset, int flags) throws ErrnoException;

	public native StructStat stat(String path) throws ErrnoException;

	public native StructStat fstat(FileDescriptor fd) throws ErrnoException;

	//////////////////////////////////////////////////////////////////////////
	// Not implemented by libsqlfs
	////////////////////////////////////////////////////////////////////////
	public FileDescriptor dup(FileDescriptor oldFd)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public FileDescriptor dup2(FileDescriptor oldFd, int newFd)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public String[] environ() throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int fcntlVoid(FileDescriptor fd, int cmd)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int fcntlLong(FileDescriptor fd, int cmd, long arg)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int fcntlFlock(FileDescriptor fd, int cmd, StructFlock arg)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void fdatasync(FileDescriptor fd)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public String gai_strerror(int error) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int getegid() throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int geteuid() throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int getgid() throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public String getenv(String name) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int getpid() throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int getppid() throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public StructPasswd getpwnam(String name)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public StructPasswd getpwuid(int uid) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int getuid() throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int ioctlInt(FileDescriptor fd, int cmd, int arg)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public boolean isatty(FileDescriptor fd)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void kill(int pid, int signal) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void listen(FileDescriptor fd, int backlog)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public StructStat lstat(String path) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void mincore(long address, long byteCount, byte[] vector)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void mlock(long address, long byteCount)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public long mmap(long address, long byteCount, int prot, int flags,
			FileDescriptor fd, long offset)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void msync(long address, long byteCount, int flags)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void munlock(long address, long byteCount)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void munmap(long address, long byteCount)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public FileDescriptor[] pipe() throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int poll(StructPollfd[] fds, int timeoutMs)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int readv(FileDescriptor fd, Object[] buffers, int[] offsets,
			int[] byteCounts) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void setegid(int egid) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void seteuid(int euid) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void setgid(int gid) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void setuid(int uid) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void shutdown(FileDescriptor fd, int how)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public FileDescriptor socket(int domain, int type, int protocol)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public long sysconf(int name) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public StructUtsname uname() throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int waitpid(int pid, int status, int options)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int writev(FileDescriptor fd, Object[] buffers, int[] offsets,
			int[] byteCounts) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}
}
