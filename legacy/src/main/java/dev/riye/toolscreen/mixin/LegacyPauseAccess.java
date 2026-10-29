package dev.riye.toolscreen.mixin;

import net.minecraft.client.gui.screens.PauseScreen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(PauseScreen.class)
public interface LegacyPauseAccess {
    @Accessor("showPauseMenu") boolean toolscreenShowsPauseMenu();
}
