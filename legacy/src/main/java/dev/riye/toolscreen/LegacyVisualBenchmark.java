package dev.riye.toolscreen;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import org.lwjgl.opengl.GL11;

public final class LegacyVisualBenchmark {
    private static final double[] samples = new double[90];
    private static int remaining;
    private static long started;
    private static String label;

    private LegacyVisualBenchmark() {}

    public static void start(String name) {
        if (!Boolean.getBoolean("toolscreen.visual")) return;
        label = name.replaceAll("[^a-zA-Z0-9_-]", "");
        remaining = samples.length + 15;
    }

    public static void begin() {
        if (remaining == 0) return;
        GL11.glFinish();
        started = System.nanoTime();
    }

    public static void end() {
        if (remaining == 0) return;
        GL11.glFinish();
        double milliseconds = (System.nanoTime() - started) / 1_000_000.0;
        if (remaining <= samples.length) samples[samples.length - remaining] = milliseconds;
        if (--remaining != 0) return;
        double[] sorted = samples.clone();
        Arrays.sort(sorted);
        double median = sorted[sorted.length / 2];
        double p95 = sorted[(int) (sorted.length * .95)];
        var game = net.minecraft.client.Minecraft.getInstance();
        int width = game.getWindow().getWidth(), height = game.getWindow().getHeight();
        String result = "{\"scope\":\"composition CPU and GPU completion, excludes game rendering and editor frame generation\",\"label\":\"" + label + "\",\"samples\":" + samples.length + ",\"output\":[" + width + "," + height + "],\"median_ms\":" + median + ",\"p95_ms\":" + p95 + ",\"renderer\":\"" + GL11.glGetString(GL11.GL_RENDERER) + "\"}";
        try {
            Files.writeString(Path.of("verification", "benchmark-" + label + ".json"), result);
            Toolscreen.LOGGER.info("TOOLSCREEN_BENCHMARK label={} median={}ms p95={}ms", label, median, p95);
        } catch (Exception failure) {
            Toolscreen.LOGGER.warn("Visual benchmark could not be saved", failure);
        }
    }
}
