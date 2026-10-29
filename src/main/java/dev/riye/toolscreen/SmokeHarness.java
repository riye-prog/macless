package dev.riye.toolscreen;

import com.mojang.blaze3d.pipeline.RenderTarget;
import com.mojang.blaze3d.pipeline.TextureTarget;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.renderpearl.api.GpuFormat;
import com.mojang.renderpearl.api.textures.GpuTextureView;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.client.Minecraft;
import net.minecraft.client.Screenshot;
import org.joml.Vector4f;

public final class SmokeHarness {
    private static int frames;
    private static boolean requested;
    private static volatile boolean complete;
    private static TextureTarget source;

    private SmokeHarness() {}

    private static final class CaptureTarget extends RenderTarget {
        CaptureTarget(GpuTextureView texture) {
            super("Macless test readback", texture.texture().getFormat(), null);
            colorTexture = texture.texture();
            colorTextureView = texture;
            width = colorTexture.getWidth(0);
            height = colorTexture.getHeight(0);
        }
    }

    public static void tick() {
        if (!Boolean.getBoolean("toolscreen.smoke")) return;
        Minecraft game = Minecraft.getInstance();
        if (!Toolscreen.ready || Toolscreen.failed) {
            if (++frames > 180) {
                Toolscreen.LOGGER.error("TOOLSCREEN_SMOKE_FAIL {}", Toolscreen.error);
                game.stop();
            }
            return;
        }
        if (game.gui.overlay() != null) return;
        frames++;
        if (frames == 20) { RenderColorProbe.run(); game.gui.setScreen(new ToolscreenScreen(game.gui.screen())); }
        if (frames == 50) {
            Screenshot.takeScreenshot(game.gameRenderer.mainRenderTarget(), image -> {
                try (image) {
                    Path directory = Path.of("verification");
                    Files.createDirectories(directory);
                    image.writeToFile(directory.resolve("ui-" + backendName() + ".png"));
                } catch (Exception failure) {
                    Toolscreen.LOGGER.error("TOOLSCREEN_UI_CAPTURE_FAIL", failure);
                }
            });
        }
        if (frames == 80 && !requested) {
            requested = true;
            try {
                NativeBridge.reset();
                NativeBridge.set(0, 1);
                NativeBridge.set(20, 1);
                NativeBridge.set(8, 0.04);
                NativeBridge.set(9, 0.04);
                NativeBridge.set(10, 0.04);
                var device = RenderSystem.getDevice();
                source = new TextureTarget("Macless test source", 320, 960, GpuFormat.RGBA8_UNORM, null);
                var encoder = device.createCommandEncoder();
                encoder.clearColorTexture(source.getColorTexture(), new Vector4f(0, 1, 0, 1));
                float[] plan = NativeBridge.plan(800, 450, 320, 960, false, 0);
                var output = Compositor.render(encoder, source.getColorTextureView(), 800, 450, plan, NativeBridge.snapshot());
                Screenshot.takeScreenshot(new CaptureTarget(output), image -> {
                    try (image) {
                        Path directory = Path.of("verification");
                        Files.createDirectories(directory);
                        image.writeToFile(directory.resolve("gpu-" + backendName() + ".png"));
                        int center = image.getPixel(400, 225);
                        int corner = image.getPixel(2, 2);
                        int mirror = image.getPixel(700, 50);
                        boolean centerGreen = ((center >>> 8) & 255) > 200 && (center & 255) < 40 && ((center >>> 16) & 255) < 40;
                        boolean missingF3Mirror = Math.abs(((mirror >>> 8) & 255) - (mirror & 255)) < 5 && ((mirror >>> 8) & 255) < 100;
                        boolean background = Math.abs(((corner >>> 8) & 255) - (corner & 255)) < 5 && ((corner >>> 8) & 255) < 100;
                        if (centerGreen && missingF3Mirror && background) {
                            Toolscreen.LOGGER.info("TOOLSCREEN_GPU_PASS backend={} center={} corner={} mirror={}", Toolscreen.backend(), Integer.toHexString(center), Integer.toHexString(corner), Integer.toHexString(mirror));
                        } else {
                            Toolscreen.LOGGER.error("TOOLSCREEN_GPU_FAIL center={} corner={} mirror={}", Integer.toHexString(center), Integer.toHexString(corner), Integer.toHexString(mirror));
                        }
                    } catch (Exception failure) {
                        Toolscreen.LOGGER.error("TOOLSCREEN_SMOKE_FAIL", failure);
                    } finally {
                        complete = true;
                    }
                });
            } catch (Throwable failure) {
                Toolscreen.LOGGER.error("TOOLSCREEN_SMOKE_FAIL", failure);
                complete = true;
            }
        }
        if (complete || frames > 600) {
            NativeBridge.reset();
            if (source != null) source.destroyBuffers();
            if (frames > 600) Toolscreen.LOGGER.error("TOOLSCREEN_SMOKE_FAIL readback timeout");
            game.stop();
        }
    }

    private static String backendName() {
        return Toolscreen.backend().toLowerCase().replaceAll("[^a-z0-9]+", "-");
    }
}
