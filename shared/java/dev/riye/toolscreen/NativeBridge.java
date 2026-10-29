package dev.riye.toolscreen;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.security.MessageDigest;
import java.util.HexFormat;
import net.fabricmc.loader.api.FabricLoader;

public final class NativeBridge {
    private NativeBridge() {}

    public static void load() throws Exception {
        NativePlatform platform=NativePlatform.current();
        byte[] bytes;
        try (InputStream source = NativeBridge.class.getResourceAsStream(platform.resource())) {
            if (source == null) throw new IOException("This Macless JAR has no native library for "+platform.os()+" "+platform.architecture());
            bytes = source.readAllBytes();
        }
        String digest = HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(bytes));
        Path configRoot = FabricLoader.getInstance().getConfigDir();
        Path directory = configRoot.resolve("macless");
        Files.createDirectories(directory);
        Path previous = configRoot.resolve("toolscreen-native");
        if (!Files.exists(directory.resolve("settings.conf"))) {
            for (String name : new String[]{"settings.conf", "settings.conf.bak", "ui-state.conf"}) {
                if (Files.exists(previous.resolve(name)) && !Files.exists(directory.resolve(name))) Files.copy(previous.resolve(name), directory.resolve(name));
            }
        }
        Files.createDirectories(directory);
        Path library = directory.resolve("toolscreen-" + digest.substring(0, 16) + platform.extension());
        if (!Files.exists(library) || !MessageDigest.isEqual(Files.readAllBytes(library), bytes)) {
            Path temporary = Files.createTempFile(directory, "native-", platform.extension());
            Files.write(temporary, bytes);
            Files.move(temporary, library, StandardCopyOption.REPLACE_EXISTING);
        }
        if (!platform.os().equals("macos")) DesktopPlatform.configure(directory);
        System.load(library.toAbsolutePath().toString());
        Toolscreen.LOGGER.info("Macless native platform={} path={} sha256={} sha512={}",platform.os()+"-"+platform.architecture(),library.getFileName(),digest,HexFormat.of().formatHex(MessageDigest.getInstance("SHA-512").digest(bytes)));
        if (!initialize(directory.resolve("settings.conf").toString())) {
            Toolscreen.LOGGER.warn("Macless settings were invalid; defaults are active");
        }
    }

    private static native boolean initialize(String path);
    public static native double[] snapshot();
    public static native void runtimeContext(boolean inWorld,double baseSensitivity);
    public static native void set(int index, double value);
    public static native void reset();
    public static native boolean save();
    public static native int[] viewport(int width, int height, int limit, boolean suspended);
    public static native float[] plan(int width, int height, int viewWidth, int viewHeight, boolean suspended, double seconds);
    public static native byte[] uiFrame(int width, int height, float delta, String backend, String error);
    public static native byte[] fontAtlas();
    public static native long fontRevision();
    public static native void uiDisplayScale(float value);
    public static native float[] uiProbe(int control);
    public static native void uiMouse(float x, float y);
    public static native void uiButton(int button, boolean down);
    public static native void uiScroll(float x, float y);
    public static native void uiKey(int keycode, int modifiers, boolean down);
    public static native void uiText(int codepoint);
    public static native void uiFocus(boolean focused);
    public static native boolean uiCloseRequested();
    public static native boolean uiBindingActive();
    public static native boolean uiWantsText();
    public static native boolean uiEscapeActive();
    public static native int keyEvent(int keycode,int modifiers,int action,boolean panelOpen);
    public static native double[] sensitivity();
    public static native byte[] overlayFrame(int width,int height,int viewWidth,int viewHeight);
    public static native String[] assetEntries();
    public static native void startSource(int id,String path);
    public static native void stopSource(int id);
    public static native int[] sourceInfo(int id);
    public static native int readSource(int id,java.nio.ByteBuffer buffer,int sequence);
    public static native String sourceError(int id);
    public static native void assetStatus(int id,int width,int height,String error,int canvasWidth,int canvasHeight);
    public static native int addAsset(String path);
    public static native int addSelectedImage(String path,boolean background);
    public static native int addMirror(String name,int preset);
    public static native void editAsset(int id,int field,double value);
    public static native int[] remapKey(int scan,int key);
    public static native boolean captureAvailable();
    public static native void releaseHeldInput();
    public static native int inputEvent(int scan,int modifiers,int action,boolean gameplay,boolean editorAllowed);
    public static native float[] planForOutput(int width,int height,int viewWidth,int viewHeight,boolean suspended,double seconds,int route);
    public static native String option(String name);
    public static native void setOption(String name,String value);
    public static native void debugClear();
    public static native void debugCounter(String value);
    public static native void debugPie(String path,String[] names,double[] data);
    public static native void ninjaUpdate(String data,String error);
    public static native boolean uiPhysicalKey(int scan,int keycode,int modifiers,boolean down);
    public static native byte[] overlayFrameForOutput(int width,int height,int viewWidth,int viewHeight,int route);
    public static native double[] debugProbe();
    public static native boolean enabled();
}
