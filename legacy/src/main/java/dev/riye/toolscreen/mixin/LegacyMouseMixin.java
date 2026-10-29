package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.Toolscreen;
import net.minecraft.client.MouseHandler;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

@Mixin(MouseHandler.class)
public abstract class LegacyMouseMixin {
    @ModifyArgs(method="turnPlayer",at=@At(value="INVOKE",target="Lnet/minecraft/client/player/LocalPlayer;turn(DD)V"))
    private void toolscreenSensitivity(Args args){if(!Toolscreen.suspended()&&net.minecraft.client.Minecraft.getInstance().screen==null){double[] scale=NativeBridge.sensitivity();args.set(0,(double)args.get(0)*scale[0]);args.set(1,(double)args.get(1)*scale[1]);}}
}
