package dev.riye.toolscreen.mixin;

import com.mojang.blaze3d.platform.Window;
import dev.riye.toolscreen.LegacyRenderer;
import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.Toolscreen;
import dev.riye.toolscreen.ToolscreenScreen;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.GameRenderer;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(GameRenderer.class)
public abstract class LegacyRendererMixin {
    @Inject(method="render",at=@At("HEAD"))
    private void toolscreenPrepare(float partial,long time,boolean renderLevel,CallbackInfo callback){if(!Toolscreen.ready)return;try{Toolscreen.updateMeasurementPause();if(!(Minecraft.getInstance().screen instanceof ToolscreenScreen)&&!Toolscreen.measurementPaused()){NativeBridge.debugClear();if(Minecraft.getInstance().screen!=null||!Minecraft.getInstance().isWindowActive())NativeBridge.releaseHeldInput();}LegacyRenderer.prepare();}catch(Throwable failure){Toolscreen.fail(failure);}}
    @Redirect(method={"render","getProjectionMatrix"},at=@At(value="INVOKE",target="Lcom/mojang/blaze3d/platform/Window;getWidth()I"))
    private int toolscreenWidth(Window window){return Toolscreen.suspended()?window.getWidth():Toolscreen.viewport()[0];}
    @Redirect(method={"render","getProjectionMatrix"},at=@At(value="INVOKE",target="Lcom/mojang/blaze3d/platform/Window;getHeight()I"))
    private int toolscreenHeight(Window window){return Toolscreen.suspended()?window.getHeight():Toolscreen.viewport()[1];}
}
