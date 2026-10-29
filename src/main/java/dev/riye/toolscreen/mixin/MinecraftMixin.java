package dev.riye.toolscreen.mixin;

import com.mojang.renderpearl.api.commands.CommandEncoder;
import com.mojang.renderpearl.api.device.GpuSurface;
import com.mojang.renderpearl.api.textures.GpuTextureView;
import dev.riye.toolscreen.Compositor;
import dev.riye.toolscreen.Toolscreen;
import dev.riye.toolscreen.SmokeHarness;
import dev.riye.toolscreen.ToolscreenScreen;
import dev.riye.toolscreen.UiRenderer;
import dev.riye.toolscreen.AssetManager;
import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.WorldHarness;
import dev.riye.toolscreen.NinjabrainClient;
import net.minecraft.client.Minecraft;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Minecraft.class)
public abstract class MinecraftMixin {
    @Unique private boolean toolscreenUiFailed;
    @Redirect(method = "renderFrame(Z)V", remap = false, at = @At(value = "INVOKE", remap = false, target = "Lcom/mojang/renderpearl/api/device/GpuSurface;blitFromTexture(Lcom/mojang/renderpearl/api/commands/CommandEncoder;Lcom/mojang/renderpearl/api/textures/GpuTextureView;)V"))
    private void toolscreenPresent(GpuSurface surface, CommandEncoder encoder, GpuTextureView source) {
        GpuTextureView frame = source;
        if (Toolscreen.ready) {
            AssetManager.update(encoder);
            if(Minecraft.getInstance().gui.screen() instanceof ToolscreenScreen)UiRenderer.livePreview(encoder,source);
            if (!Toolscreen.failed) {
                try {
                    frame = Compositor.compose(encoder, source);
                } catch (Throwable failure) {
                    Toolscreen.fail(failure);
                }
            }
            if (!toolscreenUiFailed && Minecraft.getInstance().gui.screen() instanceof ToolscreenScreen) {
                try {
                    UiRenderer.render(encoder, frame);
                } catch (Throwable failure) {
                    toolscreenUiFailed = true;
                    Toolscreen.fail(failure);
                }
            }
            else if(!toolscreenUiFailed&&!Toolscreen.suspended()){
                try{UiRenderer.render(encoder,frame,NativeBridge.overlayFrame(Minecraft.getInstance().getWindow().getWidth(),Minecraft.getInstance().getWindow().getHeight(),source.texture().getWidth(0),source.texture().getHeight(0)));}
                catch(Throwable failure){toolscreenUiFailed=true;Toolscreen.fail(failure);}
            }
        }
        if(dev.riye.toolscreen.BuildVariant.VERIFICATION)WorldHarness.observe(frame);
        surface.blitFromTexture(encoder, frame);
    }

    @Inject(method = "close", at = @At("HEAD"))
    private void toolscreenClose(CallbackInfo callback) {
        if (Toolscreen.ready) {
            Compositor.close();
            UiRenderer.close();
            AssetManager.close();
            NinjabrainClient.close();
        }
    }

    @Inject(method = "renderFrame", at = @At("TAIL"))
    private void toolscreenSmoke(CallbackInfo callback) {
        if(dev.riye.toolscreen.BuildVariant.VERIFICATION)SmokeHarness.tick();
        if(dev.riye.toolscreen.BuildVariant.VERIFICATION)WorldHarness.tick();
    }

    @Inject(method="pauseIfInactive",at=@At("HEAD"))
    private void toolscreenFocus(CallbackInfo callback){
        if(Toolscreen.ready&&!Toolscreen.measurementPaused()&&!Minecraft.getInstance().isWindowActive())NativeBridge.releaseHeldInput();
    }
}
