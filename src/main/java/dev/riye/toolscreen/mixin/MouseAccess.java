package dev.riye.toolscreen.mixin;

import net.minecraft.client.MouseHandler;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
import org.spongepowered.asm.mixin.gen.Invoker;

@Mixin(MouseHandler.class)
public interface MouseAccess {
    @Accessor("xpos") void toolscreenCursorX(double value);
    @Accessor("ypos") void toolscreenCursorY(double value);
    @Accessor("accumulatedDX") void toolscreenMouseX(double value);
    @Accessor("accumulatedDY") void toolscreenMouseY(double value);
    @Invoker("turnPlayer") void toolscreenTurnPlayer(double delta);
}
