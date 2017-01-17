
package info.guardianproject.iocipher.tests;

import android.app.Instrumentation;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import info.guardianproject.iocipher.File;
import info.guardianproject.iocipher.VirtualFileSystem;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;
import static junit.framework.Assert.fail;

@RunWith(AndroidJUnit4.class)
public class VirtualFileSystemTest {
    private final static String TAG = "VirtualFileSystemTest";

    private VirtualFileSystem vfs;
    private String path;
    private String goodPassword = "this is the right password";
    private String badPassword = "this soooo not the right password, its wrong";
    private byte[] goodKey = {
            (byte) 0x2a, (byte) 0xfc, (byte) 0x69, (byte) 0xa1, (byte) 0x16, (byte) 0x40,
            (byte) 0x4f, (byte) 0x7d, (byte) 0x7f, (byte) 0x1b, (byte) 0x1d, (byte) 0xb9,
            (byte) 0x5e, (byte) 0x18, (byte) 0x11, (byte) 0x2e, (byte) 0x6b, (byte) 0x3c,
            (byte) 0xf7, (byte) 0x1e, (byte) 0x78, (byte) 0xaf, (byte) 0x88, (byte) 0x3c,
            (byte) 0xb1, (byte) 0x90, (byte) 0x51, (byte) 0x15, (byte) 0xbf, (byte) 0xc3,
            (byte) 0xb2, (byte) 0x8d,
    };
    private byte[] tooLongKey = {
            (byte) 0x2a, (byte) 0xfc, (byte) 0x69, (byte) 0xa1, (byte) 0x16, (byte) 0x40,
            (byte) 0x4f, (byte) 0x7d, (byte) 0x7f, (byte) 0x1b, (byte) 0x1d, (byte) 0xb9,
            (byte) 0x5e, (byte) 0x18, (byte) 0x11, (byte) 0x2e, (byte) 0x6b, (byte) 0x3c,
            (byte) 0xf7, (byte) 0x1e, (byte) 0x78, (byte) 0xaf, (byte) 0x88, (byte) 0x3c,
            (byte) 0xb1, (byte) 0x90, (byte) 0x51, (byte) 0x15, (byte) 0xbf, (byte) 0xc3,
            (byte) 0xb2, (byte) 0x8d, (byte) 0x00
    };
    private byte[] tooShortKey = {
            (byte) 0x2a, (byte) 0xfc, (byte) 0x69, (byte) 0xa1, (byte) 0x16, (byte) 0x40,
            (byte) 0x4f, (byte) 0x7d, (byte) 0x7f, (byte) 0x1b, (byte) 0x1d, (byte) 0xb9,
            (byte) 0x5e, (byte) 0x18, (byte) 0x11, (byte) 0x2e, (byte) 0x6b, (byte) 0x3c,
            (byte) 0xf7, (byte) 0x1e, (byte) 0x78, (byte) 0xaf, (byte) 0x88, (byte) 0x3c,
            (byte) 0xb1, (byte) 0x90, (byte) 0x51, (byte) 0x15, (byte) 0xbf, (byte) 0xc3,
    };
    private byte[] badKey = {
            'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B',
            'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B',
    };

    @Before
    public void setUp() {
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        path = Util.getWriteableDir(instrumentation).getAbsolutePath() + "/" + TAG + ".db";
        java.io.File db = new java.io.File(path);
        if (db.exists())
            db.delete();
        vfs = VirtualFileSystem.get();
    }

    @After
    public void tearDown() {
        if (vfs.isMounted()) {
            vfs.unmount();
        }
    }

    @Test
    public void testInitMountUnmount() {
        vfs.setContainerPath(path);
        vfs.createNewContainer(goodPassword);
        vfs.mount(goodPassword);
        if (vfs.isMounted()) {
            Log.i(TAG, "vfs is mounted");
        } else {
            Log.i(TAG, "vfs is NOT mounted");
        }
        assertTrue(vfs.isMounted());
        vfs.unmount();
    }

    @Test
    public void testInitMountMkdirUnmount() {
        vfs.setContainerPath(path);
        vfs.createNewContainer(goodPassword);
        vfs.mount(goodPassword);
        if (vfs.isMounted()) {
            Log.i(TAG, "vfs is mounted");
        } else {
            Log.i(TAG, "vfs is NOT mounted");
        }
        File d = new File("/test");
        assertTrue(d.mkdir());
        vfs.unmount();
    }

    @Test
    public void testCreateMountUnmountMountExists() {
        vfs.setContainerPath(path);
        vfs.createNewContainer(goodPassword);
        vfs.mount(goodPassword);
        File f = new File("/testCreateMountUnmountMountExists."
                + Integer.toString((int) (Math.random() * 1024)));
        try {
            f.createNewFile();
        } catch (Exception e) {
            Log.e(TAG, "cannot create " + f.getPath());
            assertFalse(true);
        }
        vfs.unmount();
        vfs.mount(goodPassword);
        assertTrue(f.exists());
        vfs.unmount();
    }

    @Test(expected = IllegalArgumentException.class)
    public void testMountPasswordWithBadPassword() {
        vfs.createNewContainer(path, goodPassword);
        vfs.mount(goodPassword);
        File d = new File("/");
        for (String f : d.list()) {
            Log.v(TAG, "file: " + f);
        }
        vfs.unmount();
        vfs.mount(badPassword);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testMountKeyWithBadKey() {
        vfs.setContainerPath(path);
        vfs.createNewContainer(goodKey);
        Log.i(TAG, "goodKey length: " + goodKey.length);
        Log.i(TAG, "badKey length: " + badKey.length);
        vfs.mount(goodKey);
        File d = new File("/");
        for (String f : d.list()) {
            Log.v(TAG, "file: " + f);
        }
        vfs.unmount();
        vfs.mount(badKey);
    }

    @Test
    public void testMountKeyWithTooLongKey() {
        vfs.createNewContainer(path, goodKey);
        Log.i(TAG, "goodKey length: " + goodKey.length);
        Log.i(TAG, "tooLongKey length: " + tooLongKey.length);
        vfs.mount(goodKey);
        File d = new File("/");
        for (String f : d.list()) {
            Log.v(TAG, "file: " + f);
        }
        vfs.unmount();
        try {
            vfs.mount(tooLongKey);
        } catch (Exception e) {
            e.printStackTrace();
            return;
        } finally {
            try {
                vfs.unmount();
            } catch (IllegalStateException e) {
                // was not mounted, ignore
            }
        }
        fail();
    }

    @Test
    public void testMountKeyWithTooShortKey() {
        vfs.createNewContainer(path, goodKey);
        Log.i(TAG, "goodKey length: " + goodKey.length);
        Log.i(TAG, "tooShortKey length: " + tooShortKey.length);
        vfs.mount(goodKey);
        File d = new File("/");
        for (String f : d.list()) {
            Log.v(TAG, "file: " + f);
        }
        vfs.unmount();
        try {
            vfs.mount(tooShortKey);
        } catch (Exception e) {
            e.printStackTrace();
            return;
        } finally {
            try {
                vfs.unmount();
            } catch (IllegalStateException e) {
                // was not mounted, ignore
            }
        }
        fail();
    }

    @Test
    public void testMountKeyWithZeroedKey() {
        vfs.setContainerPath(path);
        byte[] keyCopy = new byte[goodKey.length];
        for (int i = 0; i < goodKey.length; i++)
            keyCopy[i] = goodKey[i];
        vfs.createNewContainer(keyCopy);
        vfs.mount(keyCopy);
        File d = new File("/");
        for (String f : d.list()) {
            Log.v(TAG, "file: " + f);
        }
        vfs.unmount();
        for (int i = 0; i < keyCopy.length; i++)
            keyCopy[i] = 0;
        try {
            vfs.mount(keyCopy);
        } catch (Exception e) {
            e.printStackTrace();
            return;
        } finally {
            try {
                vfs.unmount();
            } catch (IllegalStateException e) {
                // was not mounted, ignore
            }
        }
        fail();
    }

    @Test(expected = IllegalArgumentException.class)
    public void testNoWritePermsInDir() {
        vfs.setContainerPath("/file-to-create-here");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testMountKeyNonExistentFile() {
        vfs.setContainerPath("/foo/bar/this/does/not/exist");
    }

    @Test
    public void testSetGetContainerPath() {
        vfs.setContainerPath(path);
        assertTrue(path.equals(vfs.getContainerPath()));
    }

    @Test(expected = IllegalArgumentException.class)
    public void testMountAfterFileDeleted() {
        vfs.setContainerPath(path);
        vfs.createNewContainer(goodKey);
        vfs.mount(goodKey);
        File d = new File("/testMountAfterFileDeleted");
        assertTrue(d.mkdir());
        vfs.unmount();
        java.io.File containerFile = new java.io.File(vfs.getContainerPath());
        assertTrue(containerFile.exists());
        containerFile.delete();
        assertFalse(containerFile.exists());
        vfs.mount(goodKey);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testMountWithoutCreate() {
        vfs.setContainerPath(path);
        vfs.mount(goodKey);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testMountWithoutCreateSeparat() {
        vfs.setContainerPath(path);
        vfs.mount(goodKey);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testMountWithoutCreateAtOnce() {
        vfs.mount(path, goodKey);
    }
}
