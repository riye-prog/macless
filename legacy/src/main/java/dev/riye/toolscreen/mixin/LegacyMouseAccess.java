package dev.riye.toolscreen.mixin;

import net.minecraft.client.MouseHandler;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
import org.spongepowered.asm.mixin.gen.Invoker;

@Mixin(MouseHandler.class)
public interface LegacyMouseAccess {
    @Accessor("xpos") void toolscreenCursorX(double value);
    @Accessor("ypos") void toolscreenCursorY(double value);
    @Accessor("accumulatedDX") void toolscreenMouseX(double value);
    @Accessor("accumulatedDY") void toolscreenMouseY(double value);
    @Accessor("mouseGrabbed") void toolscreenMouseGrabbed(boolean value);
    @Invoker("turnPlayer") void toolscreenTurnPlayer();
}
