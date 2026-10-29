package dev.riye.toolscreen.mixin;

import com.mojang.renderpearl.api.pipeline.RenderPipeline;
import net.minecraft.client.renderer.RenderPipelines;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;

@Mixin(RenderPipelines.class)
public interface PipelineAccessor {
    @Invoker("register")
    static RenderPipeline toolscreenRegister(RenderPipeline pipeline) {
        throw new AssertionError();
    }
}
