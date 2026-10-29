package dev.riye.toolscreen;

import java.nio.file.Files;
import java.util.List;
import java.util.Optional;
import java.util.function.Consumer;
import net.fabricmc.loader.api.FabricLoader;
import net.minecraft.network.chat.Component;
import net.minecraft.server.packs.PackLocationInfo;
import net.minecraft.server.packs.PackSelectionConfig;
import net.minecraft.server.packs.PathPackResources;
import net.minecraft.server.packs.repository.Pack;
import net.minecraft.server.packs.repository.PackCompatibility;
import net.minecraft.server.packs.repository.PackSource;
import net.minecraft.world.flag.FeatureFlagSet;

public final class ModResources {
    private ModResources(){}
    public static void register(Consumer<Pack> packs){
        var container=FabricLoader.getInstance().getModContainer("toolscreen-native").orElseThrow();
        int index=0;
        for(var root:container.getRootPaths()){
            if(!Files.isDirectory(root.resolve("assets/toolscreen")))continue;
            var location=new PackLocationInfo("macless-renderer-"+index++,Component.literal("Macless renderer"),PackSource.BUILT_IN,Optional.empty());
            var metadata=new Pack.Metadata(Component.literal("Macless renderer"),PackCompatibility.COMPATIBLE,FeatureFlagSet.of(),List.of());
            packs.accept(new Pack(location,new PathPackResources.PathResourcesSupplier(root),metadata,new PackSelectionConfig(true,Pack.Position.TOP,true)));
        }
    }
}
