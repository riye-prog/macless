package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.BoatEyeMouse;
import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.Toolscreen;
import net.minecraft.client.MouseHandler;
import net.minecraft.client.Options;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.ModifyArgs;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
import org.spongepowered.asm.mixin.injection.invoke.arg.Args;

@Mixin(MouseHandler.class)
public abstract class LegacyMouseMixin {
    @Shadow private double accumulatedDX;
    @Shadow private double accumulatedDY;
    @Unique private final BoatEyeMouse toolscreenMouse=new BoatEyeMouse();
    @Unique private boolean toolscreenPrecision;

    @Inject(method="turnPlayer",at=@At("HEAD"))
    private void toolscreenPrepareMouse(CallbackInfo callback){
        toolscreenPrecision=false;
        var game=net.minecraft.client.Minecraft.getInstance();
        if(!Toolscreen.ready||Toolscreen.suspended()||game.screen!=null){toolscreenMouse.reset();return;}
        NativeBridge.runtimeContext(game.level!=null,game.options.sensitivity);
        double[] state=NativeBridge.snapshot();
        var motion=toolscreenMouse.prepare(game.options.sensitivity,NativeBridge.sensitivity()[0],accumulatedDX,accumulatedDY,state[128]>.5&&state[145]>.5,(int)state[0]==3);
        toolscreenPrecision=motion.precision();accumulatedDX=motion.x();accumulatedDY=motion.y();
    }

    @Redirect(method="turnPlayer",at=@At(value="FIELD",target="Lnet/minecraft/client/Options;sensitivity:D"))
    private double toolscreenCameraSensitivity(Options options){return toolscreenPrecision?BoatEyeMouse.GODSENS:options.sensitivity;}

    @Redirect(method="turnPlayer",at=@At(value="FIELD",target="Lnet/minecraft/client/Options;smoothCamera:Z"))
    private boolean toolscreenSmoothCamera(Options options){return !toolscreenPrecision&&options.smoothCamera;}

    @Unique private static void toolscreenMenuPosition(Args args){var menu=Toolscreen.menuCoordinates();if(menu!=null){args.set(0,menu.x(args.<Double>get(0)));args.set(1,menu.y(args.<Double>get(1)));}}

    @ModifyArgs(method="*",at=@At(value="INVOKE",target="Lnet/minecraft/client/gui/screens/Screen;mouseClicked(DDI)Z"))
    private static void toolscreenMenuClick(Args args){toolscreenMenuPosition(args);}

    @ModifyArgs(method="*",at=@At(value="INVOKE",target="Lnet/minecraft/client/gui/screens/Screen;mouseReleased(DDI)Z"))
    private static void toolscreenMenuRelease(Args args){toolscreenMenuPosition(args);}

    @ModifyArgs(method="*",at=@At(value="INVOKE",target="Lnet/minecraft/client/gui/screens/Screen;mouseScrolled(DDD)Z"))
    private static void toolscreenMenuScroll(Args args){toolscreenMenuPosition(args);}

    @ModifyArgs(method="*",at=@At(value="INVOKE",target="Lnet/minecraft/client/gui/components/events/GuiEventListener;mouseMoved(DD)V"))
    private static void toolscreenMenuMove(Args args){toolscreenMenuPosition(args);}

    @ModifyArgs(method="*",at=@At(value="INVOKE",target="Lnet/minecraft/client/gui/components/events/GuiEventListener;mouseDragged(DDIDD)Z"))
    private static void toolscreenMenuDrag(Args args){var menu=Toolscreen.menuCoordinates();if(menu!=null){toolscreenMenuPosition(args);args.set(3,menu.deltaX(args.<Double>get(3)));args.set(4,menu.deltaY(args.<Double>get(4)));}}

    @Inject(method="xpos",at=@At("RETURN"),cancellable=true)
    private void toolscreenMenuX(CallbackInfoReturnable<Double> callback){var menu=Toolscreen.menuCoordinates();if(menu!=null)callback.setReturnValue(menu.windowX(callback.getReturnValueD(),net.minecraft.client.Minecraft.getInstance().getWindow().getScreenWidth()));}

    @Inject(method="ypos",at=@At("RETURN"),cancellable=true)
    private void toolscreenMenuY(CallbackInfoReturnable<Double> callback){var menu=Toolscreen.menuCoordinates();if(menu!=null)callback.setReturnValue(menu.windowY(callback.getReturnValueD(),net.minecraft.client.Minecraft.getInstance().getWindow().getScreenHeight()));}
}
