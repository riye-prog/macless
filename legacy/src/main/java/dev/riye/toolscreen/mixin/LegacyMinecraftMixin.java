package dev.riye.toolscreen.mixin;

import com.mojang.blaze3d.pipeline.RenderTarget;
import com.mojang.blaze3d.platform.Window;
import com.mojang.blaze3d.vertex.PoseStack;
import dev.riye.toolscreen.LegacyHarness;
import dev.riye.toolscreen.LegacyRenderer;
import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.NinjabrainClient;
import dev.riye.toolscreen.Toolscreen;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.client.Minecraft;
import net.minecraft.util.profiling.ProfileResults;
import net.minecraft.util.profiling.ResultField;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Minecraft.class)
public abstract class LegacyMinecraftMixin {
    @Shadow private String debugPath;
    @Redirect(method="runTick",at=@At(value="INVOKE",target="Lcom/mojang/blaze3d/pipeline/RenderTarget;blitToScreen(II)V"))
    private void toolscreenPresent(RenderTarget target,int width,int height){if(!Toolscreen.ready||Toolscreen.failed){target.blitToScreen(width,height);return;}try{LegacyRenderer.present(target,width,height);}catch(Throwable failure){Toolscreen.fail(failure);target.blitToScreen(width,height);}}
    @Inject(method="runTick",at=@At("TAIL"))
    private void toolscreenHarness(boolean renderLevel,CallbackInfo callback){if(dev.riye.toolscreen.BuildVariant.VERIFICATION)dev.riye.toolscreen.LegacyVisualProbe.tick();if(dev.riye.toolscreen.BuildVariant.VERIFICATION)LegacyHarness.tick();}
    @Inject(method="close",at=@At("HEAD"))
    private void toolscreenClose(CallbackInfo callback){if(Toolscreen.ready){NinjabrainClient.close();LegacyRenderer.close();}}
    @Inject(method="renderFpsMeter",at=@At("HEAD"),cancellable=true)
    private void toolscreenPie(PoseStack pose,ProfileResults profile,CallbackInfo callback){if(!Toolscreen.ready||Toolscreen.failed)return;if(NativeBridge.enabled())callback.cancel();List<ResultField> list=new ArrayList<>(profile.getTimes(debugPath));if(list.isEmpty())return;ResultField current=list.remove(0);int count=Math.min(list.size(),32);String[] names=new String[count];double[] values=new double[1+count*3];values[0]=current.globalPercentage;for(int i=0;i<count;i++){ResultField value=list.get(i);names[i]=value.name;values[1+i*3]=value.percentage;values[2+i*3]=value.globalPercentage;values[3+i*3]=value.getColor();}NativeBridge.debugPie(debugPath.replace('\u001e','/'),names,values);}
    @Redirect(method="renderFpsMeter",at=@At(value="INVOKE",target="Lcom/mojang/blaze3d/platform/Window;getWidth()I"))
    private int toolscreenPieWidth(Window window){return Toolscreen.suspended()?window.getWidth():Toolscreen.viewport()[0];}
    @Redirect(method="renderFpsMeter",at=@At(value="INVOKE",target="Lcom/mojang/blaze3d/platform/Window;getHeight()I"))
    private int toolscreenPieHeight(Window window){return Toolscreen.suspended()?window.getHeight():Toolscreen.viewport()[1];}
}
