package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.PhysicalKeys;
import dev.riye.toolscreen.Toolscreen;
import dev.riye.toolscreen.LegacyKeys;
import java.util.HashMap;
import java.util.Map;
import net.minecraft.client.KeyboardHandler;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(KeyboardHandler.class)
public abstract class LegacyKeyboardMixin {
    @Unique private boolean toolscreenForwarding;
    @Unique private final Map<Integer,int[]> toolscreenHeld=new HashMap<>();
    @Inject(method="keyPress",at=@At("HEAD"),cancellable=true)
    private void toolscreenKeys(long window,int key,int scan,int action,int modifiers,CallbackInfo callback){if(toolscreenForwarding||!Toolscreen.ready)return;int hid=PhysicalKeys.fromGlfw(key,scan);if(Toolscreen.hotkey(hid,LegacyKeys.modifiers(window,modifiers),action)){callback.cancel();return;}int[] target=action==1?null:toolscreenHeld.get(hid);if(action==1||action==0)toolscreenHeld.remove(hid);if(target==null&&!Toolscreen.suspended()&&net.minecraft.client.Minecraft.getInstance().screen==null&&action==1){int[] mapping=NativeBridge.remapKey(hid,PhysicalKeys.sdlCode(hid));if(mapping[0]!=hid){int targetKey=PhysicalKeys.glfwCode(mapping[0]);target=new int[]{targetKey,targetKey>=0?org.lwjgl.glfw.GLFW.glfwGetKeyScancode(targetKey):PhysicalKeys.macScan(mapping[0])};toolscreenHeld.put(hid,target);}}if(target!=null){toolscreenForwarding=true;try{((KeyboardHandler)(Object)this).keyPress(window,target[0],target[1],action,modifiers);}finally{toolscreenForwarding=false;}callback.cancel();}}
}
