package dev.riye.toolscreen.mixin;

import com.mojang.blaze3d.platform.Window;
import dev.riye.toolscreen.Toolscreen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(Window.class)
public abstract class LegacyWindowMixin {
    @Shadow public abstract double getGuiScale();
    @Inject(method="getGuiScaledWidth",at=@At("HEAD"),cancellable=true)
    private void toolscreenWidth(CallbackInfoReturnable<Integer> callback){if(!Toolscreen.suspended()&&!(net.minecraft.client.Minecraft.getInstance().screen instanceof dev.riye.toolscreen.ToolscreenScreen))callback.setReturnValue((int)Math.ceil(Toolscreen.viewport()[0]/Math.max(1,getGuiScale())));}
    @Inject(method="getGuiScaledHeight",at=@At("HEAD"),cancellable=true)
    private void toolscreenHeight(CallbackInfoReturnable<Integer> callback){if(!Toolscreen.suspended()&&!(net.minecraft.client.Minecraft.getInstance().screen instanceof dev.riye.toolscreen.ToolscreenScreen))callback.setReturnValue((int)Math.ceil(Toolscreen.viewport()[1]/Math.max(1,getGuiScale())));}
}
