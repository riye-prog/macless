package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.Toolscreen;
import dev.riye.toolscreen.NativeBridge;
import java.util.HashMap;
import java.util.Map;
import net.minecraft.client.Minecraft;
import net.minecraft.client.KeyboardHandler;
import net.minecraft.client.input.KeyEvent;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(KeyboardHandler.class)
public abstract class KeyboardMixin {
    @Unique private boolean toolscreenForwarding;
    @Unique private final Map<Integer,KeyEvent> toolscreenHeldRemaps=new HashMap<>();
    @Inject(method = "keyPress", at = @At("HEAD"), cancellable = true)
    private void toolscreenKeys(long window, int action, KeyEvent event, CallbackInfo callback) {
        if(toolscreenForwarding)return;
        if (Toolscreen.hotkey(event.key(),event.modifiers(),action)) callback.cancel();
        if(callback.isCancelled()||!Toolscreen.ready)return;
        KeyEvent target=action==1?null:toolscreenHeldRemaps.get(event.key());
        if(action==1)toolscreenHeldRemaps.remove(event.key());
        if(action==0)toolscreenHeldRemaps.remove(event.key());
        if(target==null&&!Toolscreen.suspended()&&Minecraft.getInstance().gui.screen()==null&&action==1){int[] mapping=NativeBridge.remapKey(event.key(),event.keycode());if(mapping[0]!=event.key()||mapping[1]!=event.keycode()){target=new KeyEvent(mapping[0],mapping[1],event.modifiers());toolscreenHeldRemaps.put(event.key(),target);}}
        if(target!=null){toolscreenForwarding=true;try{((KeyboardHandler)(Object)this).keyPress(window,action,new KeyEvent(target.key(),target.keycode(),event.modifiers()));}finally{toolscreenForwarding=false;}callback.cancel();}
    }
}
