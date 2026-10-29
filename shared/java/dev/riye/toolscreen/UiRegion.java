package dev.riye.toolscreen;

import java.nio.ByteBuffer;

public record UiRegion(int left, int top, int width, int height) {
    public static UiRegion from(ByteBuffer packet, int vertices, int displayWidth, int displayHeight) {
        float minimumX = displayWidth;
        float minimumY = displayHeight;
        float maximumX = 0;
        float maximumY = 0;
        for (int index = 0; index < vertices; index++) {
            float x = packet.getFloat(8 + index * 24);
            float y = packet.getFloat(12 + index * 24);
            if (!Float.isFinite(x) || !Float.isFinite(y)) return new UiRegion(0, 0, 0, 0);
            minimumX = Math.min(minimumX, x);
            minimumY = Math.min(minimumY, y);
            maximumX = Math.max(maximumX, x);
            maximumY = Math.max(maximumY, y);
        }
        int left = Math.max(0, (int) Math.floor(minimumX) - 2);
        int top = Math.max(0, (int) Math.floor(minimumY) - 2);
        int right = Math.min(displayWidth, (int) Math.ceil(maximumX) + 2);
        int bottom = Math.min(displayHeight, (int) Math.ceil(maximumY) + 2);
        return new UiRegion(left, top, Math.max(0, right - left), Math.max(0, bottom - top));
    }

    public int bottom() {
        return top + height;
    }
}
