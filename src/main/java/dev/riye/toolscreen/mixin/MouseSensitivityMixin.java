package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.BoatEyeMouse;
import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.Toolscreen;
import net.minecraft.client.MouseHandler;
import net.minecraft.client.OptionInstance;
import net.minecraft.client.Options;
import net.minecraft.client.player.LocalPlayer;
import com.mojang.blaze3d.platform.Window;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(MouseHandler.class)
public abstract class MouseSensitivityMixin {
    @Shadow private double accumulatedDX;
    @Shadow private double accumulatedDY;
    @Shadow private double xpos;
    @Shadow private double ypos;
    @Unique private final BoatEyeMouse toolscreenMouse=new BoatEyeMouse();
    @Unique private boolean toolscreenPrecision;

    @Inject(method="turnPlayer",at=@At("HEAD"))
    private void toolscreenPrepareMouse(double delta,CallbackInfo callback){
        toolscreenPrecision=false;
        var game=net.minecraft.client.Minecraft.getInstance();
        if(!Toolscreen.ready||Toolscreen.suspended()||game.gui.screen()!=null){toolscreenMouse.reset();return;}
        double sensitivity=game.options.sensitivity().get();NativeBridge.runtimeContext(true,sensitivity);
        double[] state=NativeBridge.snapshot();
        boolean precision=state[128]>.5&&state[145]>.5;
        double multiplier=NativeBridge.sensitivity()[0];
        if(precision&&game.options.getCameraType().isFirstPerson()&&game.player.isScoping())multiplier/=8;
        var motion=toolscreenMouse.prepare(sensitivity,multiplier,accumulatedDX,accumulatedDY,precision,(int)state[0]==3);
        toolscreenPrecision=motion.precision();accumulatedDX=motion.x();accumulatedDY=motion.y();
    }

    @Redirect(method="turnPlayer",at=@At(value="INVOKE",target="Lnet/minecraft/client/OptionInstance;get()Ljava/lang/Object;",ordinal=0))
    private Object toolscreenCameraSensitivity(OptionInstance<?> option){return toolscreenPrecision?BoatEyeMouse.GODSENS:option.get();}

    @Redirect(method="turnPlayer",at=@At(value="FIELD",target="Lnet/minecraft/client/Options;smoothCamera:Z"))
    private boolean toolscreenSmoothCamera(Options options){return !toolscreenPrecision&&options.smoothCamera;}

    @Redirect(method="turnPlayer",at=@At(value="INVOKE",target="Lnet/minecraft/client/player/LocalPlayer;isScoping()Z"))
    private boolean toolscreenScoping(LocalPlayer player){return !toolscreenPrecision&&player.isScoping();}

    @Inject(method="getScaledXPos(Lcom/mojang/blaze3d/platform/Window;D)D",at=@At("HEAD"),cancellable=true)
    private static void toolscreenMenuDeltaX(Window window,double value,CallbackInfoReturnable<Double> callback){var menu=Toolscreen.menuCoordinates();if(menu!=null)callback.setReturnValue(menu.deltaX(value*window.getGuiScaledWidth()/window.getScreenWidth()));}

    @Inject(method="getScaledYPos(Lcom/mojang/blaze3d/platform/Window;D)D",at=@At("HEAD"),cancellable=true)
    private static void toolscreenMenuDeltaY(Window window,double value,CallbackInfoReturnable<Double> callback){var menu=Toolscreen.menuCoordinates();if(menu!=null)callback.setReturnValue(menu.deltaY(value*window.getGuiScaledHeight()/window.getScreenHeight()));}

    @Inject(method="getScaledXPos(Lcom/mojang/blaze3d/platform/Window;)D",at=@At("HEAD"),cancellable=true)
    private void toolscreenMenuX(Window window,CallbackInfoReturnable<Double> callback){var menu=Toolscreen.menuCoordinates();if(menu!=null)callback.setReturnValue(menu.rawX(xpos,window.getScreenWidth()));}

    @Inject(method="getScaledYPos(Lcom/mojang/blaze3d/platform/Window;)D",at=@At("HEAD"),cancellable=true)
    private void toolscreenMenuY(Window window,CallbackInfoReturnable<Double> callback){var menu=Toolscreen.menuCoordinates();if(menu!=null)callback.setReturnValue(menu.rawY(ypos,window.getScreenHeight()));}
}
