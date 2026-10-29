package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.ModResources;
import java.util.function.Consumer;
import net.minecraft.client.resources.ClientPackSource;
import net.minecraft.server.packs.repository.BuiltInPackSource;
import net.minecraft.server.packs.repository.Pack;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(BuiltInPackSource.class)
public abstract class ResourcePackMixin {
    @Inject(method="loadPacks",at=@At("TAIL"))
    private void toolscreenResources(Consumer<Pack> packs,CallbackInfo callback){if((Object)this instanceof ClientPackSource)ModResources.register(packs);}
}
