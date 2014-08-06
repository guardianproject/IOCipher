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

import info.guardianproject.libcore.io.ErrnoException;
import info.guardianproject.libcore.io.Libcore;

import java.io.SyncFailedException;

/**
 * Represents a file descriptor, but does not have the same semantics as a POSIX
 * fd.
 */
public final class FileDescriptor {
    /**
     * The sqlfs file descriptor backing this FileDescriptor. sqlfs uses the
     * full path as the token for accessing files rather than an int or long. A
     * value of "INVALID" indicates that this FileDescriptor is invalid since
     * all sqlfs paths must start with "/".
     */
    private final String invalid = "INVALID";
    private String path = invalid;

    public long position = 0;

    /**
     * Constructs a new invalid FileDescriptor.
     */
    public FileDescriptor() {
    }

    /**
     * Ensures that data which is buffered within the underlying implementation
     * is written out to the appropriate device before returning.
     */
    public void sync() throws SyncFailedException {
        try {
            Libcore.os.fsync(this);
        } catch (ErrnoException errnoException) {
            SyncFailedException sfe = new SyncFailedException(
                    errnoException.getMessage());
            sfe.initCause(errnoException);
            throw sfe;
        }
    }

    /**
     * Tests whether this {@code FileDescriptor} is valid.
     */
    public boolean valid() {
        return !path.equals(invalid);
    }

    @Override
    public String toString() {
        return "FileDescriptor[" + path + "]";
    }
}
