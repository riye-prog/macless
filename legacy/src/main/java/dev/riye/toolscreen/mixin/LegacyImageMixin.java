package dev.riye.toolscreen.mixin;

import com.mojang.blaze3d.platform.NativeImage;
import dev.riye.toolscreen.LegacyImageResize;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(NativeImage.class)
public abstract class LegacyImageMixin {
    @Shadow private long pixels;
    @Shadow public abstract int getWidth();
    @Shadow public abstract NativeImage.Format format();
    @Inject(method="resizeSubRectTo",at=@At("HEAD"),cancellable=true)
    private void toolscreenResize(int x,int y,int width,int height,NativeImage target,CallbackInfo callback){if(!LegacyImageResize.available())return;if(target.format()!=format())throw new UnsupportedOperationException("Image formats must match");long destination=((LegacyImagePixels)(Object)target).toolscreenPixels();if(pixels==0||destination==0)throw new IllegalStateException("Image is closed");int channels=format().components();LegacyImageResize.resize(pixels+(long)(x+y*getWidth())*channels,width,height,getWidth()*channels,destination,target.getWidth(),target.getHeight(),channels);callback.cancel();}
}
