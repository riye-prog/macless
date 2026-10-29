package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.Toolscreen;
import java.util.List;
import net.minecraft.client.gui.components.DebugScreenOverlay;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(DebugScreenOverlay.class)
public abstract class LegacyDebugMixin {
    @Inject(method="getGameInformation",at=@At("RETURN"))
    private void toolscreenCounter(CallbackInfoReturnable<List<String>> callback){if(!Toolscreen.ready)return;for(String line:callback.getReturnValue()){if(!line.contains("E:"))continue;String text=line.replaceAll("§[0-9a-fk-orA-FK-OR]","").trim();if(text.startsWith("E: ")){int end=text.indexOf(',');NativeBridge.debugCounter(end<0?text:text.substring(0,end));}}}
}
