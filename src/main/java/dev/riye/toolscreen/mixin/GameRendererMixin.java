package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.Compositor;
import dev.riye.toolscreen.Toolscreen;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.client.renderer.state.GameRenderState;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(GameRenderer.class)
public abstract class GameRendererMixin {
    @Shadow @Final private GameRenderState gameRenderState;

    @Inject(method="extractWindow",at=@At("HEAD"))
    private void toolscreenDebugReset(CallbackInfo callback){if(Toolscreen.ready){Toolscreen.updateMeasurementPause();Toolscreen.updateMenuLayout();}if(Toolscreen.ready&&!(net.minecraft.client.Minecraft.getInstance().gui.screen() instanceof dev.riye.toolscreen.ToolscreenScreen)&&!Toolscreen.measurementPaused()){dev.riye.toolscreen.NativeBridge.debugCounter("");if(!Toolscreen.resizeShortcutsAllowed())dev.riye.toolscreen.NativeBridge.releaseHeldInput();}}

    @Inject(method = "extractWindow", at = @At("TAIL"))
    private void toolscreenWindow(CallbackInfo callback) {
        if (!Toolscreen.resourcesReady() || Toolscreen.failed) return;
        int[] viewport = Toolscreen.viewport();
        gameRenderState.windowRenderState.width = viewport[0];
        gameRenderState.windowRenderState.height = viewport[1];
    }

    @Inject(method = "render", at = @At("HEAD"))
    private void toolscreenPrepare(CallbackInfo callback) {
        if (!Toolscreen.resourcesReady()) return;
        try {
            Compositor.prepare((GameRenderer) (Object) this, gameRenderState);
        } catch (Throwable failure) {
            Toolscreen.fail(failure);
        }
    }
}
