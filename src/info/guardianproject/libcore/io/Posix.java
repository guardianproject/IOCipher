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
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
//import java.nio.NioUtils;
//import libcore.util.MutableInt;
//import libcore.util.MutableLong;
import java.lang.UnsupportedOperationException;

;

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

	public int pread(FileDescriptor fd, ByteBuffer buffer, long offset)
			throws ErrnoException {
		if (buffer.isDirect()) {
			return preadBytes(fd, buffer, buffer.position(),
					buffer.remaining(), offset);
		} else {
			return preadBytes(fd, buffer.array(),
					buffer.arrayOffset() + buffer.position(),
					buffer.remaining(), offset);
		}
	}

	public int pread(FileDescriptor fd, byte[] bytes, int byteOffset,
			int byteCount, long offset) throws ErrnoException {
		// This indirection isn't strictly necessary, but ensures that our
		// public interface is type safe.
		return preadBytes(fd, bytes, byteOffset, byteCount, offset);
	}

	private native int preadBytes(FileDescriptor fd, Object buffer,
			int bufferOffset, int byteCount, long offset) throws ErrnoException;

	public int pwrite(FileDescriptor fd, ByteBuffer buffer, long offset)
			throws ErrnoException {
		if (buffer.isDirect()) {
			return pwriteBytes(fd, buffer, buffer.position(),
					buffer.remaining(), offset);
		} else {
			return pwriteBytes(fd, buffer.array(),
					buffer.arrayOffset() + buffer.position(),
					buffer.remaining(), offset);
		}
	}

	public int pwrite(FileDescriptor fd, byte[] bytes, int byteOffset,
			int byteCount, long offset) throws ErrnoException {
		// This indirection isn't strictly necessary, but ensures that our
		// public interface is type safe.
		return pwriteBytes(fd, bytes, byteOffset, byteCount, offset);
	}

	private native int pwriteBytes(FileDescriptor fd, Object buffer,
			int bufferOffset, int byteCount, long offset) throws ErrnoException;

	public int read(FileDescriptor fd, ByteBuffer buffer) throws ErrnoException {
		if (buffer.isDirect()) {
			return readBytes(fd, buffer, buffer.position(), buffer.remaining());
		} else {
			return readBytes(fd, buffer.array(),
					buffer.arrayOffset() + buffer.position(),
					buffer.remaining());
		}
	}

	public int read(FileDescriptor fd, byte[] bytes, int byteOffset,
			int byteCount) throws ErrnoException {
		// This indirection isn't strictly necessary, but ensures that our
		// public interface is type safe.
		return readBytes(fd, bytes, byteOffset, byteCount);
	}

	private native int readBytes(FileDescriptor fd, Object buffer, int offset,
			int byteCount) throws ErrnoException;

	public native void remove(String path) throws ErrnoException;

	public native void rename(String oldPath, String newPath)
			throws ErrnoException;

	public native void rmdir(String path) throws ErrnoException;

	public native StructStatFs statfs(String path) throws ErrnoException;

	public StructStatFs fstatfs(FileDescriptor fd) throws ErrnoException {
		// since statfs is faked anyway based on the path to the database file, just call statfs()
		return statfs("");
	}

	public native void symlink(String oldPath, String newPath)
			throws ErrnoException;

	public native void unlink(String path) throws ErrnoException;

	public int write(FileDescriptor fd, ByteBuffer buffer)
			throws ErrnoException {
		if (buffer.isDirect()) {
			return writeBytes(fd, buffer, buffer.position(), buffer.remaining());
		} else {
			return writeBytes(fd, buffer.array(),
					buffer.arrayOffset() + buffer.position(),
					buffer.remaining());
		}
	}

	public int write(FileDescriptor fd, byte[] bytes, int byteOffset,
			int byteCount) throws ErrnoException {
		// This indirection isn't strictly necessary, but ensures that our
		// public interface is type safe.
		return writeBytes(fd, bytes, byteOffset, byteCount);
	}

	private native int writeBytes(FileDescriptor fd, Object buffer, int offset,
			int byteCount) throws ErrnoException;
	
	public native StructStat stat(String path) throws ErrnoException;
	
	//////////////////////////////////////////////////////////////////////////
	// Not implemented by libsqlfs
	////////////////////////////////////////////////////////////////////////
	public FileDescriptor accept(FileDescriptor fd,
			InetSocketAddress peerAddress) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void bind(FileDescriptor fd, InetAddress address, int port)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void connect(FileDescriptor fd, InetAddress address, int port)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

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

	public StructStat fstat(FileDescriptor fd)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public String gai_strerror(int error) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public InetAddress[] getaddrinfo(String node, StructAddrinfo hints)
			throws UnsupportedOperationException {
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

	public String getnameinfo(InetAddress address, int flags)
			throws UnsupportedOperationException {
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

	public SocketAddress getsockname(FileDescriptor fd)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int getsockoptByte(FileDescriptor fd, int level, int option)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public InetAddress getsockoptInAddr(FileDescriptor fd, int level, int option)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int getsockoptInt(FileDescriptor fd, int level, int option)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public StructLinger getsockoptLinger(FileDescriptor fd, int level,
			int option) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public StructTimeval getsockoptTimeval(FileDescriptor fd, int level,
			int option) throws UnsupportedOperationException

	{
		throw new UnsupportedOperationException("Not implemented");
	}

	public int getuid() throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public String if_indextoname(int index)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public InetAddress inet_pton(int family, String address)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public InetAddress ioctlInetAddress(FileDescriptor fd, int cmd,
			String interfaceName) throws UnsupportedOperationException {
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

	public long lseek(FileDescriptor fd, long offset, int whence)
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

	public int recvfrom(FileDescriptor fd, ByteBuffer buffer, int flags,
			InetSocketAddress srcAddress) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int recvfrom(FileDescriptor fd, byte[] bytes, int byteOffset,
			int byteCount, int flags, InetSocketAddress srcAddress)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	private int recvfromBytes(FileDescriptor fd, Object buffer, int byteOffset,
			int byteCount, int flags, InetSocketAddress srcAddress)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public long sendfile(FileDescriptor outFd, FileDescriptor inFd,
			long inOffset, long byteCount) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int sendto(FileDescriptor fd, ByteBuffer buffer, int flags,
			InetAddress inetAddress, int port)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public int sendto(FileDescriptor fd, byte[] bytes, int byteOffset,
			int byteCount, int flags, InetAddress inetAddress, int port)
			throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	private int sendtoBytes(FileDescriptor fd, Object buffer, int byteOffset,
			int byteCount, int flags, InetAddress inetAddress, int port)
			throws UnsupportedOperationException {
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

	public void setsockoptByte(FileDescriptor fd, int level, int option,
			int value) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void setsockoptIfreq(FileDescriptor fd, int level, int option,
			String value) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void setsockoptInt(FileDescriptor fd, int level, int option,
			int value) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void setsockoptIpMreqn(FileDescriptor fd, int level, int option,
			int value) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void setsockoptGroupReq(FileDescriptor fd, int level, int option,
			StructGroupReq value) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void setsockoptLinger(FileDescriptor fd, int level, int option,
			StructLinger value) throws UnsupportedOperationException {
		throw new UnsupportedOperationException("Not implemented");
	}

	public void setsockoptTimeval(FileDescriptor fd, int level, int option,
			StructTimeval value) throws UnsupportedOperationException {
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

	public String strerror(int errno) throws UnsupportedOperationException {
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
