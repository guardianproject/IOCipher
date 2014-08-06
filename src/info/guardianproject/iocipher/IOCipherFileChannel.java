/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package info.guardianproject.iocipher;

import static info.guardianproject.libcore.io.OsConstants.EAGAIN;
import static info.guardianproject.libcore.io.OsConstants.O_ACCMODE;
import static info.guardianproject.libcore.io.OsConstants.O_RDONLY;
import static info.guardianproject.libcore.io.OsConstants.O_WRONLY;
import static info.guardianproject.libcore.io.OsConstants.SEEK_CUR;
import static info.guardianproject.libcore.io.OsConstants.SEEK_END;
import static info.guardianproject.libcore.io.OsConstants.SEEK_SET;
import info.guardianproject.libcore.io.ErrnoException;
import info.guardianproject.libcore.io.Libcore;
import info.guardianproject.libcore.io.StructStat;

import java.io.Closeable;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousCloseException;
import java.nio.channels.ByteChannel;
import java.nio.channels.ClosedByInterruptException;
import java.nio.channels.ClosedChannelException;
import java.nio.channels.NonReadableChannelException;
import java.nio.channels.NonWritableChannelException;
import java.nio.channels.ReadableByteChannel;
import java.nio.channels.WritableByteChannel;
import java.nio.channels.spi.AbstractInterruptibleChannel;

/**
 * IOCipher's implementation of the FileChannel class. It supports basic I/O
 * operations, but not everything you might be accustomed to from the Java NIO
 * classes. It is not a drop in replacement for FileChannel. Unsupported
 * operations: <li>mmap</li> <li>file locking</li> <li>scattered reads /
 * gathered writes (io vectors, readv, writev)</li>
 */
public class IOCipherFileChannel extends AbstractInterruptibleChannel implements
        ByteChannel {

    private final Object stream;
    private final FileDescriptor fd;
    private final int mode;

    /**
     * Create a new file channel implementation class that wraps the given fd
     * and operates in the specified mode.
     */
    public IOCipherFileChannel(Object stream, FileDescriptor fd, int mode) {
        this.fd = fd;
        this.stream = stream;
        this.mode = mode;
    }

    private void checkOpen() throws ClosedChannelException {
        if (!isOpen()) {
            throw new ClosedChannelException();
        }
    }

    private void checkReadable() {
        if ((mode & O_ACCMODE) == O_WRONLY) {
            throw new NonReadableChannelException();
        }
    }

    private void checkWritable() {
        if ((mode & O_ACCMODE) == O_RDONLY) {
            throw new NonWritableChannelException();
        }
    }

    /**
     * Implements the channel closing behavior.
     * <p>
     * Closes the channel with a guarantee that the channel is not currently
     * closed through another invocation of {@code close()} and that the method
     * is thread-safe.
     * <p>
     * Any outstanding threads blocked on I/O operations on this channel must be
     * released with either a normal return code, or by throwing an
     * {@code AsynchronousCloseException}.
     * 
     * @throws IOException if a problem occurs while closing the channel.
     */
    @Override
    protected void implCloseChannel() throws IOException {
        if (stream instanceof Closeable) {
            ((Closeable) stream).close();
        }
    }

    /**
     * Requests that all updates to this channel are committed to the storage
     * device.
     * <p>
     * When this method returns, all modifications made to the platform file
     * underlying this channel have been committed if the file resides on a
     * local storage device. If the file is not hosted locally, for example on a
     * networked file system, then applications cannot be certain that the
     * modifications have been committed.
     * <p>
     * There are no assurances given that changes made to the file using methods
     * defined elsewhere will be committed. For example, changes made via a
     * mapped byte buffer may not be committed.
     * <p>
     * The <code>metadata</code> parameter indicates whether the update should
     * include the file's metadata such as last modification time, last access
     * time, etc. Note that passing <code>true</code> may invoke an underlying
     * write to the operating system (if the platform is maintaining metadata
     * such as last access time), even if the channel is opened read-only.
     * 
     * @param metadata {@code true} if the file metadata should be flushed in
     *            addition to the file content, {@code false} otherwise.
     * @throws ClosedChannelException if this channel is already closed.
     * @throws IOException if another I/O error occurs.
     */
    public void force(boolean metadata) throws IOException {
        checkOpen();
        if ((mode & O_ACCMODE) != O_RDONLY) {
            try {
                Libcore.os.fsync(fd); // FUSE only has fsync, not fdatasync
            } catch (ErrnoException errnoException) {
                throw errnoException.rethrowAsIOException();
            }
        }
    }

    /**
     * IOCipher version of POSIX lseek, since the underlying FUSE layer does not
     * track the position in open files for us, we do it inside of this class.
     * This class wraps a {@link FileDescriptor}, so you cannot specify one as
     * an argument.
     * 
     * @param offset the new position to seek to.
     * @param whence changes the pointer repositioning behavior:
     *            <ul>
     *            <li>if
     *            {@link info.guardianproject.libcore.io.OsConstants.SEEK_SET
     *            SEEK_SET} then file pointer is set to <i>offset</i></li>
     *            <li>if
     *            {@link info.guardianproject.libcore.io.OsConstants.SEEK_CUR
     *            SEEK_CUR} then file pointer is set to <i>current position +
     *            offset</i></li>
     *            <li>if
     *            {@link info.guardianproject.libcore.io.OsConstants.SEEK_END
     *            SEEK_END} then file pointer is set to <i>file size +
     *            offset</i></li>
     *            </ul>
     * @throws ClosedChannelException if this channel is already closed.
     * @return new position of file pointer
     */
    public long lseek(long offset, int whence) throws IOException {
        checkOpen();
        long tmpPosition = fd.position;
        if (whence == SEEK_SET) {
            tmpPosition = offset;
        } else if (whence == SEEK_CUR) {
            tmpPosition += offset;
        } else if (whence == SEEK_END) {
            tmpPosition = size() + offset;
        } else {
            throw new IllegalArgumentException("Unknown 'whence': " + whence);
        }
        if (tmpPosition < 0)
            throw new IOException("negative resulting position: " + tmpPosition);
        else
            fd.position = tmpPosition;
        return fd.position;
    }

    /**
     * Returns the current value of the file position pointer.
     * 
     * @return the current position as a positive integer number of bytes from
     *         the start of the file.
     * @throws ClosedChannelException if this channel is closed.
     */
    public long position() throws IOException {
        checkOpen();
        return fd.position;
    }

    /**
     * Sets the file position pointer to a new value.
     * <p>
     * The argument is the number of bytes counted from the start of the file.
     * The position cannot be set to a value that is negative. The new position
     * can be set beyond the current file size. If set beyond the current file
     * size, attempts to read will return end of file. Write operations will
     * succeed but they will fill the bytes between the current end of file and
     * the new position with the required number of (unspecified) byte values.
     * 
     * @param newPosition the new file position, in bytes.
     * @return the receiver.
     * @throws IllegalArgumentException if the new position is negative.
     * @throws ClosedChannelException if this channel is closed.
     */
    public IOCipherFileChannel position(long newPosition) throws IOException {
        if (newPosition < 0)
            throw new IllegalArgumentException(
                    "negative file position not allowed: " + newPosition);
        checkOpen();
        fd.position = newPosition;
        return this;
    }

    private int readImpl(ByteBuffer buffer, long position) throws IOException {
        if (buffer.isReadOnly()) {
            throw new IllegalArgumentException("read-only buffer");
        }
        checkOpen();
        checkReadable();
        if (!buffer.hasRemaining()) {
            return 0;
        }
        int bytesRead = 0;
        boolean completed = false;
        try {
            begin();
            try {
                if (position == -1) {
                    bytesRead = Libcore.os.read(fd, buffer);
                } else {
                    bytesRead = Libcore.os.pread(fd, buffer, position);
                }
                if (bytesRead == 0) {
                    bytesRead = -1;
                }
            } catch (ErrnoException errnoException) {
                if (errnoException.errno == EAGAIN) {
                    // We don't throw if we try to read from an empty
                    // non-blocking pipe.
                    bytesRead = 0;
                } else {
                    throw errnoException.rethrowAsIOException();
                }
            }
            completed = true;
        } finally {
            end(completed && bytesRead >= 0);
        }
        if (bytesRead > 0) {
            buffer.position(buffer.position() + bytesRead);
        }
        return bytesRead;
    }

    /**
     * Reads bytes from this file channel into the given buffer.
     * <p>
     * The maximum number of bytes that will be read is the remaining number of
     * bytes in the buffer when the method is invoked. The bytes will be copied
     * into the buffer starting at the buffer's current position.
     * <p>
     * The call may block if other threads are also attempting to read from this
     * channel.
     * <p>
     * Upon completion, the buffer's position is set to the end of the bytes
     * that have been read. The buffer's limit is not changed.
     * 
     * @param buffer the byte buffer to receive the bytes.
     * @return the number of bytes actually read.
     * @throws AsynchronousCloseException if another thread closes the channel
     *             during the read.
     * @throws ClosedByInterruptException if another thread interrupts the
     *             calling thread during the read.
     * @throws ClosedChannelException if this channel is closed.
     * @throws IOException if another I/O error occurs, details are in the
     *             message.
     * @throws NonReadableChannelException if the channel has not been opened in
     *             a mode that permits reading.
     */
    @Override
    public int read(ByteBuffer buffer) throws IOException {
        return readImpl(buffer, -1);
    }

    /**
     * Reads bytes from this file channel into the given buffer starting from
     * the specified file position.
     * <p>
     * The bytes are read starting at the given file position (up to the
     * remaining number of bytes in the buffer). The number of bytes actually
     * read is returned.
     * <p>
     * If {@code position} is beyond the current end of file, then no bytes are
     * read.
     * <p>
     * Note that the file position is unmodified by this method.
     * 
     * @param buffer the buffer to receive the bytes.
     * @param position the (non-negative) position at which to read the bytes.
     * @return the number of bytes actually read.
     * @throws AsynchronousCloseException if this channel is closed by another
     *             thread while this method is executing.
     * @throws ClosedByInterruptException if another thread interrupts the
     *             calling thread while this operation is in progress. The
     *             calling thread will have the interrupt state set, and the
     *             channel will be closed.
     * @throws ClosedChannelException if this channel is closed.
     * @throws IllegalArgumentException if <code>position</code> is less than 0.
     * @throws IOException if another I/O error occurs.
     * @throws NonReadableChannelException if the channel has not been opened in
     *             a mode that permits reading.
     */
    public int read(ByteBuffer buffer, long position) throws IOException {
        if (position < 0)
            throw new IllegalArgumentException(
                    "negative file position not allowed: " + position);
        return readImpl(buffer, position);
    }

    /**
     * Returns the size of the file underlying this channel in bytes.
     * 
     * @return the size of the file in bytes.
     * @throws ClosedChannelException if this channel is closed.
     * @throws IOException if an I/O error occurs while getting the size of the
     *             file.
     */
    public long size() throws IOException {
        try {
            StructStat sb = Libcore.os.fstat(fd);
            return sb.st_size;
        } catch (ErrnoException errnoException) {
            throw errnoException.rethrowAsIOException();
        }
    }

    private int writeImpl(ByteBuffer buffer, long position) throws IOException {
        checkOpen();
        checkWritable();
        if (buffer == null) {
            throw new NullPointerException("buffer == null");
        }
        if (!buffer.hasRemaining()) {
            return 0;
        }
        int bytesWritten = 0;
        boolean completed = false;
        try {
            begin();
            try {
                if (position == -1) {
                    bytesWritten = Libcore.os.write(fd, buffer, this.mode);
                } else {
                    bytesWritten = Libcore.os.pwrite(fd, buffer, position, this.mode);
                }
            } catch (ErrnoException errnoException) {
                throw errnoException.rethrowAsIOException();
            }
            completed = true;
        } finally {
            end(completed);
        }
        if (bytesWritten > 0) {
            buffer.position(buffer.position() + bytesWritten);
        }
        return bytesWritten;
    }

    /**
     * Writes bytes from the given byte buffer to this file channel.
     * <p>
     * The bytes are written starting at the current file position, and after
     * some number of bytes are written (up to the remaining number of bytes in
     * the buffer) the file position is increased by the number of bytes
     * actually written.
     * 
     * @param src the byte buffer containing the bytes to be written.
     * @return the number of bytes actually written.
     * @throws NonWritableChannelException if the channel was not opened for
     *             writing.
     * @throws ClosedChannelException if the channel was already closed.
     * @throws AsynchronousCloseException if another thread closes the channel
     *             during the write.
     * @throws ClosedByInterruptException if another thread interrupts the
     *             calling thread while this operation is in progress. The
     *             interrupt state of the calling thread is set and the channel
     *             is closed.
     * @throws IOException if another I/O error occurs, details are in the
     *             message.
     * @see java.nio.channels.WritableByteChannel#write(java.nio.ByteBuffer)
     */
    @Override
    public int write(ByteBuffer src) throws IOException {
        return writeImpl(src, -1);
    }

    /**
     * Reads up to {@code count} bytes from {@code src} and stores them in this
     * channel's file starting at {@code position}. No bytes are transferred if
     * {@code position} is larger than the size of this channel's file. Less
     * than {@code count} bytes are transferred if there are less bytes
     * remaining in the source channel or if the source channel is non-blocking
     * and has less than {@code count} bytes immediately available in its output
     * buffer.
     * <p>
     * Note that this channel's position is not modified.
     * 
     * @param src the source channel to read bytes from.
     * @param position the non-negative start position.
     * @param count the non-negative number of bytes to transfer.
     * @return the number of bytes that are transferred.
     * @throws IllegalArgumentException if the parameters are invalid.
     * @throws NonReadableChannelException if the source channel is not
     *             readable.
     * @throws NonWritableChannelException if this channel is not writable.
     * @throws ClosedChannelException if either channel has already been closed.
     * @throws AsynchronousCloseException if either channel is closed by other
     *             threads during this operation.
     * @throws ClosedByInterruptException if the thread is interrupted during
     *             this operation.
     * @throws IOException if any I/O error occurs.
     */
    public long transferFrom(ReadableByteChannel src, long position, long count)
            throws IOException {
        checkOpen();
        if (!src.isOpen()) {
            throw new ClosedChannelException();
        }
        checkWritable();
        if (position < 0 || count < 0 || count > Integer.MAX_VALUE) {
            throw new IllegalArgumentException("position=" + position + " count=" + count);
        }
        if (position > size()) {
            return 0;
        }

        ByteBuffer buffer = ByteBuffer.allocate((int) count);
        src.read(buffer);
        buffer.flip();
        return write(buffer, position);
    }

    /**
     * Reads up to {@code count} bytes from this channel's file starting at
     * {@code position} and writes them to {@code target}. No bytes are
     * transferred if {@code position} is larger than the size of this channel's
     * file. Less than {@code count} bytes are transferred if there less bytes
     * available from this channel's file or if the target channel is
     * non-blocking and has less than {@code count} bytes free in its input
     * buffer.
     * <p>
     * Note that this channel's position is not modified.
     * 
     * @param position the non-negative position to begin.
     * @param count the non-negative number of bytes to transfer.
     * @param target the target channel to write to.
     * @return the number of bytes that were transferred.
     * @throws IllegalArgumentException if the parameters are invalid.
     * @throws NonReadableChannelException if this channel is not readable.
     * @throws NonWritableChannelException if the target channel is not
     *             writable.
     * @throws ClosedChannelException if either channel has already been closed.
     * @throws AsynchronousCloseException if either channel is closed by other
     *             threads during this operation.
     * @throws ClosedByInterruptException if the thread is interrupted during
     *             this operation.
     * @throws IOException if any I/O error occurs.
     */
    public long transferTo(long position, long count, WritableByteChannel target)
            throws IOException {
        checkOpen();
        if (!target.isOpen()) {
            throw new ClosedChannelException();
        }
        checkReadable();
        if (target instanceof IOCipherFileChannel) {
            ((IOCipherFileChannel) target).checkWritable();
        }
        if (position < 0 || count < 0) {
            throw new IllegalArgumentException("position=" + position + " count=" + count);
        }

        if (count == 0 || position >= size()) {
            return 0;
        }
        count = Math.min(count, size() - position);

        try {
            ByteBuffer buffer = ByteBuffer.allocate((int) count);
            read(buffer, position);
            buffer.flip();
            return target.write(buffer);
        } finally {
            // TODO determine whether we have memory leaking or perf issues here
            // #567
        }
    }

    /**
     * Truncates the file underlying this channel to a given size. Any bytes
     * beyond the given size are removed from the file. If there are no bytes
     * beyond the given size then the file contents are unmodified.
     * <p>
     * If the file position is currently greater than the given size, then it is
     * set to the new size.
     * 
     * @param size the maximum size of the underlying file.
     * @throws IllegalArgumentException if the requested size is negative.
     * @throws ClosedChannelException if this channel is closed.
     * @throws NonWritableChannelException if the channel cannot be written to.
     * @throws IOException if another I/O error occurs.
     * @return this channel.
     */
    public IOCipherFileChannel truncate(long size) throws IOException {
        checkOpen();
        if (size < 0) {
            throw new IllegalArgumentException("size: " + size);
        }
        checkWritable();
        if (size < size()) {
            try {
                Libcore.os.ftruncate(fd, size);
            } catch (ErrnoException errnoException) {
                throw errnoException.rethrowAsIOException();
            }
        }
        return this;
    }

    /**
     * Writes bytes from the given buffer to this file channel starting at the
     * given file position.
     * <p>
     * The bytes are written starting at the given file position (up to the
     * remaining number of bytes in the buffer). The number of bytes actually
     * written is returned.
     * <p>
     * If the position is beyond the current end of file, then the file is first
     * extended up to the given position by the required number of unspecified
     * byte values.
     * <p>
     * Note that the file position is not modified by this method.
     * 
     * @param buffer the buffer containing the bytes to be written.
     * @param position the (non-negative) position at which to write the bytes.
     * @return the number of bytes actually written.
     * @throws IllegalArgumentException if <code>position</code> is less than 0.
     * @throws ClosedChannelException if this channel is closed.
     * @throws NonWritableChannelException if the channel was not opened in
     *             write-mode.
     * @throws AsynchronousCloseException if this channel is closed by another
     *             thread while this method is executing.
     * @throws ClosedByInterruptException if another thread interrupts the
     *             calling thread while this operation is in progress. The
     *             interrupt state of the calling thread is set and the channel
     *             is closed.
     * @throws IOException if another I/O error occurs.
     */
    public int write(ByteBuffer buffer, long position) throws IOException {
        if (position < 0) {
            throw new IllegalArgumentException("position: " + position);
        }
        return writeImpl(buffer, position);
    }

    // TODO implement vectored IO? #237

    /*
     * @Override public long read(ByteBuffer[] buffers, int start, int number)
     * throws IOException { Arrays.checkOffsetAndCount(buffers.length, offset,
     * length); checkOpen(); checkReadable(); return transferIoVec(new
     * IoVec(buffers, offset, length, IoVec.Direction.READV)); }
     */
    /*
     * @Override public long write(ByteBuffer[] buffers, int offset, int length)
     * throws IOException { }
     */

}
