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
    @Inject(method="getGuiScale",at=@At("RETURN"),cancellable=true)
    private void toolscreenMenuScale(CallbackInfoReturnable<Double> callback){var screen=net.minecraft.client.Minecraft.getInstance().screen;if(!Toolscreen.resizingSuspended()&&screen!=null&&!(screen instanceof dev.riye.toolscreen.ToolscreenScreen)){int[] view=Toolscreen.viewport();int limit=Math.max(1,Math.min(view[0]/320,view[1]/240));callback.setReturnValue(Math.min(callback.getReturnValueD(),limit));}}
    @Inject(method="getGuiScaledWidth",at=@At("HEAD"),cancellable=true)
    private void toolscreenWidth(CallbackInfoReturnable<Integer> callback){if(!Toolscreen.resizingSuspended()&&!(net.minecraft.client.Minecraft.getInstance().screen instanceof dev.riye.toolscreen.ToolscreenScreen))callback.setReturnValue((int)Math.ceil(Toolscreen.viewport()[0]/Math.max(1,getGuiScale())));}
    @Inject(method="getGuiScaledHeight",at=@At("HEAD"),cancellable=true)
    private void toolscreenHeight(CallbackInfoReturnable<Integer> callback){if(!Toolscreen.resizingSuspended()&&!(net.minecraft.client.Minecraft.getInstance().screen instanceof dev.riye.toolscreen.ToolscreenScreen))callback.setReturnValue((int)Math.ceil(Toolscreen.viewport()[1]/Math.max(1,getGuiScale())));}
}
