package dev.riye.toolscreen.mixin;

import net.minecraft.client.Minecraft;
import net.minecraft.util.profiling.ProfileResults;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(Minecraft.class)
public interface LegacyProfilerAccess {
    @Accessor("debugPath") void toolscreenSetPath(String path);
    @Accessor("fpsPieResults") ProfileResults toolscreenResults();
}
