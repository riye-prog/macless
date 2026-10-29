package dev.riye.toolscreen;

import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.renderpearl.api.GpuFormat;
import com.mojang.renderpearl.api.commands.CommandEncoder;
import com.mojang.renderpearl.api.buffers.GpuBuffer;
import com.mojang.renderpearl.api.commands.RenderPass;
import com.mojang.renderpearl.api.pipeline.BindGroupLayout;
import com.mojang.renderpearl.api.pipeline.ColorTargetState;
import com.mojang.renderpearl.api.pipeline.PrimitiveTopology;
import com.mojang.renderpearl.api.pipeline.RenderPipeline;
import com.mojang.renderpearl.api.pipeline.UniformType;
import com.mojang.renderpearl.api.textures.FilterMode;
import com.mojang.renderpearl.api.textures.GpuTexture;
import com.mojang.renderpearl.api.textures.GpuTextureView;
import dev.riye.toolscreen.mixin.PipelineAccessor;
import java.util.Optional;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import net.minecraft.client.renderer.MappableRingBuffer;
import net.minecraft.resources.Identifier;

public final class UiColorCompositor {
    private static final RenderPipeline PIPELINE = RenderPipeline.builder()
        .withLocation(Identifier.fromNamespaceAndPath("toolscreen", "pipeline/ui_resolve"))
        .withVertexShader(Identifier.fromNamespaceAndPath("toolscreen", "core/ui_resolve"))
        .withFragmentShader(Identifier.fromNamespaceAndPath("toolscreen", "core/ui_resolve"))
        .withBindGroupLayout(BindGroupLayout.builder()
            .withUniform("SceneSampler", UniformType.COMBINED_IMAGE_SAMPLER)
            .withUniform("UiSampler", UniformType.COMBINED_IMAGE_SAMPLER)
            .withUniform("Region", UniformType.UNIFORM_BUFFER).build())
        .withColorTargetState(ColorTargetState.DEFAULT)
        .withDepthStencilState(Optional.empty())
        .withPrimitiveTopology(PrimitiveTopology.TRIANGLES)
        .withCull(false)
        .build();
    private static GpuTexture sceneTexture;
    private static GpuTextureView sceneView;
    private static GpuTexture layerTexture;
    private static GpuTextureView layerView;
    private static MappableRingBuffer uniforms;
    private static ByteBuffer regionData;

    private UiColorCompositor() {}

    public static void register() {
        PipelineAccessor.toolscreenRegister(PIPELINE);
    }

    public static GpuTextureView prepare(CommandEncoder encoder, GpuTextureView target, UiRegion region) {
        int width = target.texture().getWidth(0);
        int height = target.texture().getHeight(0);
        var device = RenderSystem.getDevice();
        if (sceneTexture == null || sceneTexture.getWidth(0) != width || sceneTexture.getHeight(0) != height || sceneTexture.getFormat() != target.texture().getFormat()) {
            if (sceneView != null) sceneView.close();
            if (sceneTexture != null) sceneTexture.close();
            sceneTexture = device.createTexture("Macless UI scene", GpuTexture.USAGE_TEXTURE_BINDING | GpuTexture.USAGE_COPY_DST, target.texture().getFormat(), width, height, 1, 1);
            sceneView = device.createTextureView(sceneTexture);
        }
        if (layerTexture == null || layerTexture.getWidth(0) != region.width() || layerTexture.getHeight(0) != region.height()) {
            if (layerView != null) layerView.close();
            if (layerTexture != null) layerTexture.close();
            layerTexture = device.createTexture("Macless linear UI", GpuTexture.USAGE_TEXTURE_BINDING | GpuTexture.USAGE_RENDER_ATTACHMENT | GpuTexture.USAGE_COPY_DST, GpuFormat.RGBA16_FLOAT, region.width(), region.height(), 1, 1);
            layerView = device.createTextureView(layerTexture);
        }
        int bottom = height - region.bottom();
        if (!GlUiSceneCopy.copy(target.texture(), sceneTexture, region, height)) encoder.copyTextureToTexture(target.texture(), sceneTexture, 0, region.left(), bottom, region.left(), bottom, region.width(), region.height());
        if (uniforms == null) {
            int alignment = Math.max(16, device.getDeviceInfo().limits().minUniformOffsetAlignment());
            uniforms = new MappableRingBuffer(() -> "Macless UI region", GpuBuffer.USAGE_UNIFORM | GpuBuffer.USAGE_COPY_DST | GpuBuffer.USAGE_MAP_WRITE, alignment);
            regionData = ByteBuffer.allocateDirect(16).order(ByteOrder.nativeOrder());
        }
        regionData.clear();
        regionData.putFloat((float) region.left() / width).putFloat((float) bottom / height).putFloat((float) region.width() / width).putFloat((float) region.height() / height).flip();
        encoder.writeToBuffer(uniforms.currentBuffer().slice(0, 16), regionData);
        return layerView;
    }

    public static void resolve(CommandEncoder encoder, GpuTextureView target) {
        try (RenderPass pass = encoder.createRenderPass(() -> "Macless linear UI resolve", target, Optional.empty())) {
            pass.setPipeline(RenderSystem.getCompiledPipeline(PIPELINE));
            var sampler = RenderSystem.getSamplerCache().getClampToEdge(FilterMode.NEAREST);
            pass.setUniform("SceneSampler", sceneView, sampler);
            pass.setUniform("UiSampler", layerView, sampler);
            pass.setUniform("Region", uniforms.currentBuffer().slice(0, 16));
            pass.draw(6, 1, 0, 0);
        }
        uniforms.rotate();
    }

    public static void close() {
        GlUiSceneCopy.close();
        if (uniforms != null) uniforms.close();
        uniforms = null;
        regionData = null;
        if (sceneView != null) sceneView.close();
        if (sceneTexture != null) sceneTexture.close();
        if (layerView != null) layerView.close();
        if (layerTexture != null) layerTexture.close();
        sceneTexture = null;
        sceneView = null;
        layerTexture = null;
        layerView = null;
    }
}
