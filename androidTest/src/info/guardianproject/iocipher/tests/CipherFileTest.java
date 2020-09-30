
package info.guardianproject.iocipher.tests;

import android.app.Instrumentation;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.DigestInputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

import info.guardianproject.iocipher.File;
import info.guardianproject.iocipher.FileInputStream;
import info.guardianproject.iocipher.FileOutputStream;
import info.guardianproject.iocipher.FileReader;
import info.guardianproject.iocipher.FileWriter;
import info.guardianproject.iocipher.IOCipherFileChannel;
import info.guardianproject.iocipher.RandomAccessFile;
import info.guardianproject.iocipher.VirtualFileSystem;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;
import static junit.framework.Assert.fail;

@RunWith(AndroidJUnit4.class)
public class CipherFileTest {
    private final static String TAG = "CipherFileTest";

    private VirtualFileSystem vfs;
    private File ROOT = null;
    private final String goodPassword = "this is my secure password";

    @Before
    public void setUp() {
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        String path = Util.getWriteableDir(instrumentation).getAbsolutePath() + "/" + TAG + ".db";
        java.io.File db = new java.io.File(path);
        if (db.exists()) {
            Log.v(TAG, "Deleting existing database file: " + db.getAbsolutePath());
            db.delete();
        }
        if (db.exists()) {
            Log.v(TAG, "Deleted file exists: " + db.getAbsolutePath());
            fail();
        }
        Log.v(TAG, "Creating new database file: " + db.getAbsolutePath());
        vfs = VirtualFileSystem.get();
        vfs.setContainerPath(path);
        vfs.createNewContainer(goodPassword);
        Log.v(TAG, "Mounting:");
        vfs.mount(goodPassword);
        ROOT = new File("/");
    }

    @After
    public void tearDown() {
        vfs.unmount();
    }

    @Test
    public void testExists() {
        File f = new File("");
        try {
            assertFalse(f.exists());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    // @Test
    // public void testGetFreeSpace() {
    // File f = new File(ROOT, "");
    // try {
    // long free = f.getFreeSpace();
    // Log.v(TAG, "f.getFreeSpace: " + Long.toString(free));
    // assertTrue(free > 0);
    // } catch (ExceptionInInitializerError e) {
    // Log.e(TAG, e.getCause().toString());
    // assertFalse(true);
    // }
    // }

    // @Test
    // public void testGetUsableSpace() {
    // File f = new File(ROOT, "");
    // try {
    // long total = f.getUsableSpace();
    // Log.v(TAG, "f.getUsableSpace: " + Long.toString(total));
    // assertTrue(total > 0);
    // } catch (ExceptionInInitializerError e) {
    // Log.e(TAG, e.getCause().toString());
    // assertFalse(true);
    // }
    // }

    // @Test
    // public void testGetTotalSpace() {
    // File f = new File(ROOT, "");
    // try {
    // long total = f.getTotalSpace();
    // Log.v(TAG, "f.getTotalSpace: " + Long.toString(total));
    // assertTrue(total > 0);
    // } catch (ExceptionInInitializerError e) {
    // Log.e(TAG, e.getCause().toString());
    // assertFalse(true);
    // }
    // }

    @Test
    public void testMkdirExists() {
        File f = new File(ROOT, "test.iocipher.dir."
                + Integer.toString((int) (Math.random() * 1024)));
        try {
            assertFalse(f.exists());
            assertTrue(f.mkdir());
            assertTrue(f.exists());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testMkdirs() {
        File f0 = new File(ROOT, Integer.toString((int) (Math.random() * Integer.MAX_VALUE)));
        File f1 = new File(f0,
                Integer.toString((int) (Math.random() * Integer.MAX_VALUE)));
        File f2 = new File(f1,
                Integer.toString((int) (Math.random() * Integer.MAX_VALUE)));
        Log.v(TAG, "f2: " + f2.getAbsolutePath());
        try {
            f2.mkdirs();
            for (String f : f0.list()) {
                Log.v(TAG, "file in f0: " + f);
            }
            for (String f : f1.list()) {
                Log.v(TAG, "file in f1: " + f);
            }
            assertTrue(f0.exists());
            assertTrue(f1.exists());
            assertTrue(f2.exists());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testSlashIsDirectory() {
        File f = ROOT;
        try {
            assertTrue(f.isDirectory());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testCanReadSlash() {
        File f = ROOT;
        try {
            assertTrue(f.isDirectory());
            assertTrue(f.canRead());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testCanWriteSlash() {
        File f = ROOT;
        try {
            assertTrue(f.isDirectory());
            assertTrue(f.canWrite());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testSlashIsFile() {
        File f = ROOT;
        try {
            assertFalse(f.isFile());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testSlashIsAbsolute() {
        File f = ROOT;
        try {
            assertTrue(f.isAbsolute());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testMkdirRemove() {
        File f = new File(ROOT, "mkdir-to-remove");
        try {
            assertTrue(f.mkdir());
            assertTrue(f.exists());
            assertTrue(f.delete());
            assertFalse(f.exists());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testRenameToExisting() {
        File d = new File(ROOT, "dir-to-rename");
        File d2 = new File(ROOT, "exists");
        try {
            d.mkdir();
            d2.mkdir();
            assertFalse(d.renameTo(new File(ROOT, "exists")));
            assertTrue(d.exists());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testMkdirRename() {
        String dir = "mkdir-to-rename";
        String newdir = "renamed";
        String firstfile = "first-file";
        File root = ROOT;
        File d = new File(root, dir);
        File newd = new File(root, newdir);
        try {
            d.mkdir();
            File f1 = new File(d, firstfile);
            f1.createNewFile();
            assertTrue(f1.exists());
            String[] files = d.list();
            assertEquals(files.length, 1);
            for (String filename : files) {
                Log.v(TAG, "testMkdirRename " + dir + ": " + filename);
            }
            assertTrue(d.renameTo(newd));
            assertTrue(new File(newd, firstfile).exists());
            File f2 = new File(newd, "second-file");
            f2.createNewFile();
            files = root.list();
            assertEquals(files.length, 1);
            for (String filename : files) {
                Log.v(TAG, "testMkdirRename root: " + filename);
            }
            files = newd.list();
            assertEquals(files.length, 2);
            for (String filename : files) {
                Log.v(TAG, "testMkdirRename " + newdir + ": " + filename);
            }
            assertFalse(d.exists());
            assertTrue(newd.exists());
            assertTrue(f2.exists());
        } catch (ExceptionInInitializerError | IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testNewFileRename() {
        File root = ROOT;
        File f = new File(Util.randomFileName(ROOT, "testNewFileRename-NEW"));
        File newf = new File(Util.randomFileName(ROOT, "testNewFileRename-RENAMED"));
        try {
            f.createNewFile();
            assertTrue(f.renameTo(newf));
            final String[] files = root.list();
            for (String filename : files) {
                Log.v(TAG, "testNewFileRename file: " + filename);
            }
            assertFalse(f.exists());
            assertTrue(newf.exists());
        } catch (ExceptionInInitializerError | IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testMkdirIsDirectory() {
        File f = new File(ROOT, "mkdir-to-test");
        try {
            f.mkdir();
            assertTrue(f.isDirectory());
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testMkdirList() {
        File root = ROOT;
        File f = new File(ROOT, "mkdir-to-list");
        try {
            f.mkdir();
            final String[] files = root.list();
            for (String filename : files) {
                Log.v(TAG, "testMkdirList file: " + filename);
            }
            Log.v(TAG, "testMkdirList list: " + files.length);
            assertTrue(files.length == 1); // ".." and "." shouldn't be included
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    /*
     * // TODO testMkdirLastModified fails public void testMkdirLastModified() {
     * File root = ROOT; File f = new File(Util.randomFileName(ROOT,
     * "test.iocipher.dir")); try { long lasttime = root.lastModified();
     * Log.v(TAG, "f.lastModified: " + Long.toString(lasttime)); f.mkdir(); long
     * thistime = root.lastModified(); Log.i(TAG,
     * "f.lastModified after setting: " + Long.toString(thistime));
     * assertTrue(thistime > lasttime); } catch (ExceptionInInitializerError e)
     * { Log.e(TAG, e.getCause().toString()); assertFalse(true); } } // TODO
     * testMkdirMtime fails public void testMkdirMtime() { File f = new
     * File("/mkdir-with-mtime"); long faketime = 1000000000L; try { f.mkdir();
     * Log.v(TAG, "f.lastModified: " + Long.toString(f.lastModified()));
     * f.setLastModified(faketime); long time = f.lastModified(); Log.v(TAG,
     * "f.lastModified after setting: " + Long.toString(time)); assertTrue(time
     * == faketime); } catch (ExceptionInInitializerError e) { Log.e(TAG,
     * e.getCause().toString()); assertFalse(true); } }
     */

    @Test
    public void testCreateNewFile() {
        File root = ROOT;
        File f = new File(Util.randomFileName(ROOT, "testCreateNewFile"));
        try {
            assertFalse(f.exists());
            f.createNewFile();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            final String[] files = root.list();
            for (String filename : files) {
                Log.v(TAG, "testCreateNewFile file: " + filename);
            }
        } catch (ExceptionInInitializerError | IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteNewFile() {
        File root = ROOT;
        File f = new File(Util.randomFileName(ROOT, "testWriteNewFile"));
        try {
            assertTrue(root.isDirectory());
            assertFalse(f.exists());
            FileOutputStream out = new FileOutputStream(f);
            out.write(123);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            final String[] files = root.list();
            for (String filename : files) {
                Log.v(TAG, "testWriteNewFile file: " + filename);
            }
        } catch (ExceptionInInitializerError | IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteByteInNewFileThenRead() {
        byte testValue = 43;
        File root = ROOT;
        File f = new File(Util.randomFileName(ROOT, "testWriteNewFile"));
        try {
            assertTrue(root.isDirectory());
            assertFalse(f.exists());
            FileOutputStream out = new FileOutputStream(f);
            out.write(testValue);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            FileInputStream in = new FileInputStream(f);
            int b = in.read();
            Log.v(TAG, "read: " + Integer.toString(b));
            assertTrue(b == testValue);
            in.close();
        } catch (ExceptionInInitializerError | IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteTextInNewFileThenReadByByte() {
        String testString = "this is a test of IOCipher!";
        File f = new File(Util.randomFileName(ROOT, "testWriteTextInNewFileThenReadByByte"));
        try {
            assertFalse(f.exists());
            BufferedWriter out = new BufferedWriter(new FileWriter(f));
            out.write(testString);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            FileInputStream in = new FileInputStream(f);
            byte[] data = new byte[testString.length()];
            in.read(data, 0, data.length);
            String dataString = new String(data);
            Log.v(TAG, "read: " + dataString);
            assertTrue(dataString.equals(testString));
            in.close();
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteTextInNewFileThenReadIntoByteArray() {
        String testString = "this is a test of IOCipher!";
        File f = new File(Util.randomFileName(ROOT, "testWriteTextInNewFileThenReadIntoByteArray"));
        try {
            assertFalse(f.exists());
            BufferedWriter out = new BufferedWriter(new FileWriter(f));
            out.write(testString);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            FileInputStream in = new FileInputStream(f);
            byte[] data = new byte[testString.length()];
            int ret = in.read(data);
            assertTrue(ret == data.length);
            String dataString = new String(data);
            assertTrue(dataString.equals(testString));
            in.close();
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteTextInNewFileThenReadOneByteByByte() {
        String testString = "01234567890abcdefgh";
        File f = new File(Util.randomFileName(ROOT, "testWriteTextInNewFileThenReadOneByteByByte."));
        try {
            assertFalse(f.exists());
            BufferedWriter out = new BufferedWriter(new FileWriter(f));
            out.write(testString);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            FileInputStream in = new FileInputStream(f);
            byte[] data = new byte[testString.length()];
            int ret = 0;
            int i = 0;
            while (i < data.length) {
                ret = in.read();
                if (ret != -1) {
                    data[i] = (byte) ret;
                    i++;
                } else {
                    break;
                }
            }
            String dataString = new String(data);
            Log.v(TAG, "read: " + dataString);
            assertTrue(dataString.equals(testString));
            in.close();
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteTextInNewFileThenCheckSize() {
        String testString = "01234567890abcdefgh";
        File f = new File(Util.randomFileName(ROOT, "testWriteTextInNewFileThenCheckSize"));
        try {
            assertFalse(f.exists());
            BufferedWriter out = new BufferedWriter(new FileWriter(f));
            out.write(testString);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            FileInputStream in = new FileInputStream(f);
            IOCipherFileChannel channel = in.getChannel();
            assertTrue(channel.size() == testString.length());
            assertTrue(testString.length() == f.length());
            in.close();
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteTextInNewFileThenSkipAndRead() {
        String testString = "01234567890abcdefghijklmnopqrstuvxyz";
        File f = new File(Util.randomFileName(ROOT, "testWriteTextInNewFileThenSkipAndRead"));
        try {
            assertFalse(f.exists());
            BufferedWriter out = new BufferedWriter(new FileWriter(f));
            out.write(testString);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            FileInputStream in = new FileInputStream(f);
            char c = (char) in.read();
            assertTrue(c == testString.charAt(0));
            in.skip(5);
            c = (char) in.read();
            Log.v(TAG, "c: " + c + "  testString.charAt(6): "
                    + testString.charAt(6));
            assertTrue(c == testString.charAt(6));
            in.skip(20);
            c = (char) in.read();
            Log.v(TAG, "c: " + c + "  testString.charAt(27): "
                    + testString.charAt(27));
            assertTrue(c == testString.charAt(27));
            in.close();
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteRepeat() {
        int i, repeat = 1000;
        String testString = "01234567890abcdefghijklmnopqrstuvxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\n";
        File f = new File(Util.randomFileName(ROOT, "testWriteRepeat"));
        try {
            assertFalse(f.exists());
            BufferedWriter out = new BufferedWriter(new FileWriter(f));
            for (i = 0; i < repeat; i++)
                out.write(testString);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            Log.v(TAG, f.toString() + ".length(): " + f.length() + " " + testString.length()
                    * repeat);
            assertTrue(f.length() == testString.length() * repeat);
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteSkipWrite() {
        int skip = 100;
        String testString = "the best of times\n";
        String testString2 = "the worst of times\n";
        File f = new File(Util.randomFileName(ROOT, "testWriteSkipWrite"));
        try {
            assertFalse(f.exists());
            RandomAccessFile inout = new RandomAccessFile(f, "rw");
            inout.writeBytes(testString);
            inout.seek(inout.getFilePointer() + skip);
            inout.writeBytes(testString2);
            inout.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            int inputLength = testString.length() + skip + testString2.length();
            Log.v(TAG, "testWriteSkipWrite: " + f.toString() + ".length(): " + f.length() + " "
                    + inputLength);
            assertTrue(f.length() == inputLength);

            inout = new RandomAccessFile(f, "rw");
            byte[] best = new byte[testString.length()];
            byte[] worst = new byte[testString2.length()];
            inout.read(best, 0, testString.length());
            inout.seek(inout.getFilePointer() + skip);
            inout.read(worst, 0, testString2.length());
            inout.close();
            assertEquals(new String(best), testString);
            assertEquals(new String(worst), testString2);
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteTextInNewFileThenFileInputStream() {
        String testString = "01234567890abcdefgh";
        File f = new File(Util.randomFileName(ROOT, "testWriteTextInNewFileThenFileInputStream"));
        try {
            assertFalse(f.exists());
            BufferedWriter out = new BufferedWriter(new FileWriter(f));
            out.write(testString);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            BufferedReader in = new BufferedReader(new FileReader(f));
            String tmp = in.readLine();
            Log.v(TAG, "in.readline(): " + tmp);
            assertTrue(testString.equals(tmp));
            in.close();
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteManyLinesInNewFileThenFileInputStream() {
        String testString = "01234567890abcdefghijklmnopqrstuvwxyz";
        File f = new File(Util.randomFileName(ROOT,
                "testWriteManyLinesInNewFileThenFileInputStream"));
        try {
            assertFalse(f.exists());
            BufferedWriter out = new BufferedWriter(new FileWriter(f));
            for (int i = 0; i < 1000; i++)
                out.write(testString + "\n");
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            BufferedReader in = new BufferedReader(new FileReader(f));
            for (int i = 0; i < 1000; i++) {
                String tmp = in.readLine();
                Log.v(TAG, "in.readline(): " + tmp);
                assertTrue(testString.equals(tmp));
            }
            in.close();
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteAndReadAfterAlreadyMountedException() {
        String testString = "01234567890abcdefghijklmnopqrstuvwxyz";
        File f = new File(Util.randomFileName(ROOT,
                "testWriteAndReadAfterAlreadyMountedException"));
        try {
            assertFalse(f.exists());
            BufferedWriter out = new BufferedWriter(new FileWriter(f));
            for (int i = 0; i < 100; i++) {
                out.write(testString + "\n");
                try {
                    vfs.mount(goodPassword);
                    fail();
                } catch (IllegalStateException e) {
                    // this is what we want, its already mounted
                }
                assertTrue(vfs.isMounted());
            }
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            BufferedReader in = new BufferedReader(new FileReader(f));
            for (int i = 0; i < 100; i++) {
                String tmp = in.readLine();
                Log.v(TAG, "in.readline(): " + tmp);
                try {
                    vfs.mount(goodPassword);
                    fail();
                } catch (IllegalStateException e) {
                    // this is what we want, its already mounted
                }
                assertTrue(vfs.isMounted());
                assertTrue(testString.equals(tmp));
            }
            in.close();
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    private byte[] digest(File f) {
        try {
            MessageDigest md = MessageDigest.getInstance("SHA-1");
            FileInputStream fstr = new FileInputStream(f);
            DigestInputStream dstr = new DigestInputStream(fstr, md);

            // read to EOF, really Java? *le sigh*
            while (dstr.read() != -1)
                ;

            dstr.close();
            return md.digest();
        } catch (NoSuchAlgorithmException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (FileNotFoundException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
        return null;
    }

    @Test
    public void testFileChannelTransferTo() {
        String input_name = "/testCopyFileChannels-input";
        String output_name = "/testCopyFileChannels-output";
        assertTrue(Util.cipherWriteRandomBytes(1000, input_name));
        File inputFile = new File(input_name);
        File outputFile = new File(output_name);

        try {
            assertTrue(inputFile.exists());
            assertTrue(inputFile.isFile());

            assertFalse(outputFile.exists());

            FileInputStream source = new FileInputStream(inputFile);
            FileOutputStream destination = new FileOutputStream(output_name);
            IOCipherFileChannel sourceFileChannel = source.getChannel();
            IOCipherFileChannel destinationFileChannel = destination.getChannel();

            sourceFileChannel.transferTo(0, sourceFileChannel.size(), destinationFileChannel);
            sourceFileChannel.close();
            destinationFileChannel.close();

            assertTrue(outputFile.exists());
            assertTrue(outputFile.isFile());
            assertEquals(inputFile.length(), outputFile.length());

            byte[] expected = digest(inputFile);
            byte[] actual = digest(outputFile);

            Log.i(TAG, "file hashes:" + Util.toHex(expected) + "    " + Util.toHex(actual));
            assertTrue(Arrays.equals(expected, actual));

            source.close();
            destination.close();
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testFileChannelTransferFrom() {
        String input_name = "/testCopyFileChannels-input";
        String output_name = "/testCopyFileChannels-output";
        assertTrue(Util.cipherWriteRandomBytes(1000, input_name));
        File inputFile = new File(input_name);
        File outputFile = new File(output_name);

        try {
            assertTrue(inputFile.exists());
            assertTrue(inputFile.isFile());

            assertFalse(outputFile.exists());

            FileInputStream source = new FileInputStream(inputFile);
            FileOutputStream destination = new FileOutputStream(output_name);
            IOCipherFileChannel sourceFileChannel = source.getChannel();
            IOCipherFileChannel destinationFileChannel = destination.getChannel();

            destinationFileChannel.transferFrom(sourceFileChannel, 0, sourceFileChannel.size());
            sourceFileChannel.close();
            destinationFileChannel.close();

            assertTrue(outputFile.exists());
            assertTrue(outputFile.isFile());
            assertEquals(inputFile.length(), outputFile.length());

            byte[] expected = digest(inputFile);
            byte[] actual = digest(outputFile);

            Log.i(TAG, "file hashes:" + Util.toHex(expected) + "    " + Util.toHex(actual));
            assertTrue(Arrays.equals(expected, actual));

            source.close();
            destination.close();
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testFileExistingTruncate() {
        String name = Util.randomFileName(ROOT, "testFileExistingTruncate");
        assertTrue(Util.cipherWriteRandomBytes(50000, name));

        File f = new File(name);
        assertEquals(50000, f.length());

        try {
            FileOutputStream out = new FileOutputStream(f);
            out.close();
            assertEquals(0, f.length());
        } catch (FileNotFoundException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testFileExistingAppend() {
        String name = Util.randomFileName(ROOT, "testFileExistingAppend");
        assertTrue(Util.cipherWriteRandomBytes(500, name));
        File f = new File(name);
        byte[] orig_buf = new byte[500];

        try {
            FileInputStream in = new FileInputStream(f);
            in.read(orig_buf, 0, 500);
            in.close();

            assertEquals(500, f.length());

            FileOutputStream out = new FileOutputStream(f, true);

            // write 4 bytes
            //both fail!
            /**
            out.write(13);
            out.write(42);
            out.write(42);
            out.write(49);
             **/
            byte[] newdat = {13,42,42,42};
            out.write(newdat);

            out.close();

            f = new File(name);
            long fLen = f.length();
            Log.d("CipherFileTest","append length: " + fLen);
            assertEquals(504, fLen);

            FileInputStream in2 = new FileInputStream(f);
            byte[] test_buf = new byte[500];
            in2.read(test_buf, 0, 500);
            assertTrue(Arrays.equals(orig_buf, test_buf));
            assertEquals(13, in2.read());
            assertEquals(42, in2.read());


            in2.close();
        } catch (FileNotFoundException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testWriteByteInExistingFileThenRead() {
        byte testValue = 43;
        byte secondTestValue = 100;
        File root = ROOT;
        File f = new File(Util.randomFileName(ROOT, "testWriteByteInExistingFileThenRead"));
        try {
            assertTrue(root.isDirectory());
            assertFalse(f.exists());
            FileOutputStream out = new FileOutputStream(f);
            out.write(testValue);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            FileInputStream in = new FileInputStream(f);
            int b = in.read();
            in.close();
            Log.v(TAG, "read: " + Integer.toString(b));
            assertTrue(b == testValue);

            // now overwrite
            out = new FileOutputStream(f);
            out.write(secondTestValue);
            out.close();
            assertTrue(f.exists());
            assertTrue(f.isFile());
            in = new FileInputStream(f);
            b = in.read();
            in.close();
            assertTrue(b == secondTestValue);
        } catch (ExceptionInInitializerError e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        } catch (IOException e) {
            Log.e(TAG, e.getCause().toString());
            assertFalse(true);
        }
    }

    @Test
    public void testEqualsAndCompareTo() {
        String filename = "thisisafile";
        File f = new File(ROOT, filename);
        File dup = new File(ROOT, filename);
        File diff = new File(ROOT, "differentfile");
        assertTrue(f.equals(dup));
        assertTrue(f.compareTo(dup) == 0);
        assertFalse(f.equals(diff));
        assertTrue(f.compareTo(diff) != 0);
    }
}
