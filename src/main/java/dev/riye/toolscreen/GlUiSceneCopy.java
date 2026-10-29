package dev.riye.toolscreen;

import com.mojang.renderpearl.api.textures.GpuTexture;
import com.mojang.renderpearl.backend.opengl.GlStateManager;
import com.mojang.renderpearl.backend.opengl.GlTexture;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL30;

public final class GlUiSceneCopy {
    private static int readFramebuffer;
    private static int drawFramebuffer;

    private GlUiSceneCopy() {}

    public static boolean copy(GpuTexture source, GpuTexture destination, UiRegion region, int height) {
        if (!(source instanceof GlTexture input) || !(destination instanceof GlTexture output)) return false;
        if (readFramebuffer == 0) {
            readFramebuffer = GL30.glGenFramebuffers();
            drawFramebuffer = GL30.glGenFramebuffers();
        }
        int previousRead = GlStateManager.getFrameBuffer(GL30.GL_READ_FRAMEBUFFER);
        int previousDraw = GlStateManager.getFrameBuffer(GL30.GL_DRAW_FRAMEBUFFER);
        GlStateManager._disableScissorTest();
        try {
            GlStateManager._glBindFramebuffer(GL30.GL_READ_FRAMEBUFFER, readFramebuffer);
            GlStateManager._glFramebufferTexture2D(GL30.GL_READ_FRAMEBUFFER, GL30.GL_COLOR_ATTACHMENT0, GL11.GL_TEXTURE_2D, input.glId(), 0);
            GlStateManager._glBindFramebuffer(GL30.GL_DRAW_FRAMEBUFFER, drawFramebuffer);
            GlStateManager._glFramebufferTexture2D(GL30.GL_DRAW_FRAMEBUFFER, GL30.GL_COLOR_ATTACHMENT0, GL11.GL_TEXTURE_2D, output.glId(), 0);
            int left = region.left();
            int bottom = height - region.bottom();
            int right = left + region.width();
            int top = bottom + region.height();
            GlStateManager._glBlitFrameBuffer(left, bottom, right, top, left, bottom, right, top, GL11.GL_COLOR_BUFFER_BIT, GL11.GL_NEAREST);
        } finally {
            GlStateManager._glBindFramebuffer(GL30.GL_READ_FRAMEBUFFER, previousRead);
            GlStateManager._glBindFramebuffer(GL30.GL_DRAW_FRAMEBUFFER, previousDraw);
        }
        return true;
    }

    public static void close() {
        if (readFramebuffer != 0) GlStateManager._glDeleteFramebuffers(readFramebuffer);
        if (drawFramebuffer != 0) GlStateManager._glDeleteFramebuffers(drawFramebuffer);
        readFramebuffer = 0;
        drawFramebuffer = 0;
    }
}
