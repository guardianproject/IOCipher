/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package info.guardianproject.iocipher;

import static info.guardianproject.libcore.io.OsConstants.O_RDONLY;
import static info.guardianproject.libcore.io.OsConstants.SEEK_CUR;
import info.guardianproject.libcore.io.IoBridge;
import info.guardianproject.libcore.io.IoUtils;

import java.io.BufferedInputStream;
import java.io.Closeable;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

/**
 * An input stream that reads bytes from a file.
 * 
 * <pre>
 * {@code
 *   File file = ...
 *   InputStream in = null;
 *   try {
 *     in = new BufferedInputStream(new FileInputStream(file));
 *     ...
 *   } finally {
 *     if (in != null) {
 *       in.close();
 *     }
 *   }
 * }
 * </pre>
 * <p>
 * This stream is <strong>not buffered</strong>. Most callers should wrap this
 * stream with a {@link BufferedInputStream}.
 * <p>
 * Use {@link FileReader} to read characters, as opposed to bytes, from a file.
 * 
 * @see BufferedInputStream
 * @see FileOutputStream
 */
public class FileInputStream extends InputStream implements Closeable {

    private FileDescriptor fd;
    private final boolean shouldClose;

    private IOCipherFileChannel channel;

    /**
     * Constructs a new {@code FileInputStream} that reads from {@code file}.
     * 
     * @param file the file from which this stream reads.
     * @throws FileNotFoundException if {@code file} does not exist.
     */
    public FileInputStream(File file) throws FileNotFoundException {
        if (file == null) {
            throw new NullPointerException("file == null");
        }
        this.fd = IoBridge.open(file.getAbsolutePath(), O_RDONLY);
        getChannel(); // init channel
        this.shouldClose = true;
    }

    /**
     * Constructs a new {@code FileInputStream} that reads from {@code fd}.
     * 
     * @param fd the FileDescriptor from which this stream reads.
     * @throws NullPointerException if {@code fd} is {@code null}.
     */
    public FileInputStream(FileDescriptor fd) {
        if (fd == null) {
            throw new NullPointerException("fd == null");
        }
        this.fd = fd;
        getChannel(); // init channel
        this.shouldClose = false;
    }

    /**
     * Equivalent to {@code new FileInputStream(new File(path))}.
     */
    public FileInputStream(String path) throws FileNotFoundException {
        this(new File(path));
    }

    @Override
    public int available() throws IOException {
        long value = channel.size() - channel.position();
        if (value < 0) {
            // The result is the difference between the file size and the file
            // position. This may be negative if the position is past the end
            // of the file.
            value = 0;
        }
        return (int) value;
    }

    @Override
    public void close() throws IOException {
        synchronized (this) {
            if (channel != null) {
                channel.close();
            }
            if (shouldClose) {
                IoUtils.close(fd);
            } else {
                // An owned fd has been invalidated by IoUtils.close, but
                // we need to explicitly stop using an unowned fd
                // (http://b/4361076).
                fd = new FileDescriptor();
            }
        }
    }

    /**
     * Ensures that all resources for this stream are released when it is about
     * to be garbage collected.
     * 
     * @throws IOException if an error occurs attempting to finalize this
     *             stream.
     */
    @Override
    protected void finalize() throws IOException {
        try {
            close();
        } finally {
            try {
                super.finalize();
            } catch (Throwable t) {
                // for consistency with the RI, we must override
                // Object.finalize() to
                // remove the 'throws Throwable' clause.
                throw new AssertionError(t);
            }
        }
    }

    /**
     * Returns a read-only {@link IOCipherFileChannel} that shares its position
     * with this stream.
     */
    public IOCipherFileChannel getChannel() {
        synchronized (this) {
            if (channel == null) {
                channel = new IOCipherFileChannel(this, fd, O_RDONLY);
            }
            return channel;
        }
    }

    /**
     * Returns the underlying file descriptor.
     */
    public final FileDescriptor getFD() throws IOException {
        return fd;
    }

    public static int readSingleByte(InputStream in) throws IOException {
        byte[] buffer = new byte[1];
        int result = in.read(buffer, 0, 1);
        return (result != -1) ? buffer[0] & 0xff : -1;
    }

    @Override
    public int read() throws IOException {
        return readSingleByte(this);
    }

    @Override
    public int read(byte[] b) throws IOException {
        return IoBridge.read(fd, b, 0, b.length);
    }

    @Override
    public int read(byte[] buffer, int byteOffset, int byteCount) throws IOException {
        return IoBridge.read(fd, buffer, byteOffset, byteCount);
    }

    @Override
    public long skip(long byteCount) throws IOException {
        if (byteCount < 0) {
            throw new IOException("byteCount < 0: " + byteCount);
        }
        long before = channel.position();
        long after = channel.lseek(byteCount, SEEK_CUR);
        return after - before;
    }
}
