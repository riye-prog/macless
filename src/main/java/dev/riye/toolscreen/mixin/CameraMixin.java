package dev.riye.toolscreen.mixin;

import com.mojang.blaze3d.platform.Window;
import dev.riye.toolscreen.Toolscreen;
import net.minecraft.client.Camera;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;

@Mixin(Camera.class)
public abstract class CameraMixin {
    @Redirect(method="update",at=@At(value="INVOKE",target="Lcom/mojang/blaze3d/platform/Window;getWidth()I"))
    private int toolscreenProjectionWidth(Window window){return Toolscreen.suspended()?window.getWidth():Toolscreen.viewport()[0];}

    @Redirect(method="update",at=@At(value="INVOKE",target="Lcom/mojang/blaze3d/platform/Window;getHeight()I"))
    private int toolscreenProjectionHeight(Window window){return Toolscreen.suspended()?window.getHeight():Toolscreen.viewport()[1];}

    @Redirect(method="createProjectionMatrixForCulling",at=@At(value="INVOKE",target="Lcom/mojang/blaze3d/platform/Window;getWidth()I"))
    private int toolscreenCullingWidth(Window window){
        if(Toolscreen.suspended())return window.getWidth();
        int[] view=Toolscreen.viewport();
        return Math.max(view[0],(int)Math.ceil((double)view[1]*window.getWidth()/Math.max(window.getHeight(),1)));
    }

    @Redirect(method="createProjectionMatrixForCulling",at=@At(value="INVOKE",target="Lcom/mojang/blaze3d/platform/Window;getHeight()I"))
    private int toolscreenCullingHeight(Window window){return Toolscreen.suspended()?window.getHeight():Toolscreen.viewport()[1];}
}
