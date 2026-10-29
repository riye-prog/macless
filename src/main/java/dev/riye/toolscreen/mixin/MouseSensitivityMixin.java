package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.Toolscreen;
import net.minecraft.client.MouseHandler;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

@Mixin(MouseHandler.class)
public abstract class MouseSensitivityMixin {
    @ModifyArgs(method="turnPlayer",at=@At(value="INVOKE",target="Lnet/minecraft/client/player/LocalPlayer;turn(DD)V"))
    private void toolscreenSensitivity(Args args){
        if(!Toolscreen.ready||Toolscreen.suspended()||net.minecraft.client.Minecraft.getInstance().gui.screen()!=null)return;
        NativeBridge.runtimeContext(true,net.minecraft.client.Minecraft.getInstance().options.sensitivity().get());double[] factor=NativeBridge.sensitivity();
        args.set(0,args.<Double>get(0)*factor[0]);
        args.set(1,args.<Double>get(1)*factor[1]);
    }
}
