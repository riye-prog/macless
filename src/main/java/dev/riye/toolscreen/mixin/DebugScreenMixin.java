package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.Toolscreen;
import java.util.List;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.components.DebugScreenOverlay;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(DebugScreenOverlay.class)
public abstract class DebugScreenMixin {
    @Inject(method="extractLines",at=@At("HEAD"))
    private void toolscreenCounter(GuiGraphicsExtractor graphics,List<String> lines,boolean alignLeft,int screenWidth,CallbackInfo callback){
        if(!Toolscreen.ready)return;
        for(String line:lines){if(!line.contains("E:"))continue;String text=line.replaceAll("§[0-9a-fk-orA-FK-OR]","").trim();if(text.startsWith("E: ")){int end=text.indexOf(',');NativeBridge.debugCounter(end<0?text:text.substring(0,end));}}
    }
}
