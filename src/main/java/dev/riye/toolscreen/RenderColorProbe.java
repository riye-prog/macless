package dev.riye.toolscreen;

import com.mojang.blaze3d.pipeline.RenderTarget;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.renderpearl.api.GpuFormat;
import com.mojang.renderpearl.api.textures.GpuTexture;
import com.mojang.renderpearl.api.textures.GpuTextureView;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.client.Screenshot;
import org.joml.Vector4f;

public final class RenderColorProbe {
    private static final class CaptureTarget extends RenderTarget {
        CaptureTarget(GpuTextureView view) {
            super("Macless color probe", view.texture().getFormat(), null);
            colorTexture = view.texture();
            colorTextureView = view;
            width = 8;
            height = 8;
        }
    }

    private RenderColorProbe() {}

    public static void run() {
        var device = RenderSystem.getDevice();
        var texture = device.createTexture("Macless color probe", GpuTexture.USAGE_RENDER_ATTACHMENT | GpuTexture.USAGE_TEXTURE_BINDING | GpuTexture.USAGE_COPY_SRC | GpuTexture.USAGE_COPY_DST, GpuFormat.RGBA8_UNORM, 8, 8, 1, 1);
        var view = device.createTextureView(texture);
        var encoder = device.createCommandEncoder();
        encoder.clearColorTexture(texture, new Vector4f(0, 0, 1, 1));
        var linear = UiColorCompositor.prepare(encoder, view, new UiRegion(2, 1, 3, 3));
        encoder.clearColorTexture(linear.texture(), new Vector4f(.5f, .5f, .5f, .5f));
        UiColorCompositor.resolve(encoder, view);
        Screenshot.takeScreenshot(new CaptureTarget(view), image -> {
            try (image) {
                var pixels = image.getPixelBytes();
                int center = (2 * 8 + 3) * 4;
                int red = pixels.get(center) & 255;
                int green = pixels.get(center + 1) & 255;
                int blue = pixels.get(center + 2) & 255;
                boolean valid = Math.abs(red - 188) <= 2 && Math.abs(green - 188) <= 2 && blue >= 253 && (pixels.get(0) & 255) <= 1 && (pixels.get(1) & 255) <= 1 && (pixels.get(2) & 255) >= 253;
                Path directory = Path.of("verification");
                Files.createDirectories(directory);
                Files.writeString(directory.resolve("ui-color-" + Toolscreen.backend().toLowerCase() + ".json"), "{\"inside\":[" + red + "," + green + "," + blue + "],\"expected\":[188,188,255],\"region_preserved\":" + valid + "}");
                if (!valid) throw new IllegalStateException("Linear UI resolve or dirty-region bounds are incorrect");
                Toolscreen.LOGGER.info("TOOLSCREEN_UI_COLOR_PASS backend={} inside={},{},{} region=true", Toolscreen.backend(), red, green, blue);
            } catch (Throwable failure) {
                Toolscreen.LOGGER.error("TOOLSCREEN_UI_COLOR_FAIL", failure);
            } finally {
                view.close();
                texture.close();
            }
        });
    }
}
