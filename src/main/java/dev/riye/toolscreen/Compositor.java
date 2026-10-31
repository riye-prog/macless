package dev.riye.toolscreen;

import com.mojang.blaze3d.pipeline.RenderTarget;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.renderpearl.api.GpuFormat;
import com.mojang.renderpearl.api.buffers.GpuBuffer;
import com.mojang.renderpearl.api.commands.CommandEncoder;
import com.mojang.renderpearl.api.commands.RenderPass;
import com.mojang.renderpearl.api.device.GpuDevice;
import com.mojang.renderpearl.api.pipeline.BindGroupLayout;
import com.mojang.renderpearl.api.pipeline.BlendFunction;
import com.mojang.renderpearl.api.pipeline.ColorTargetState;
import com.mojang.renderpearl.api.pipeline.RenderPipeline;
import com.mojang.renderpearl.api.pipeline.PrimitiveTopology;
import com.mojang.renderpearl.api.pipeline.UniformType;
import com.mojang.renderpearl.api.textures.FilterMode;
import com.mojang.renderpearl.api.textures.GpuTexture;
import com.mojang.renderpearl.api.textures.GpuTextureView;
import dev.riye.toolscreen.mixin.PipelineAccessor;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Optional;
import net.minecraft.client.Minecraft;
import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.client.renderer.MappableRingBuffer;
import net.minecraft.client.renderer.state.GameRenderState;
import net.minecraft.resources.Identifier;
import org.joml.Vector4f;

public final class Compositor {
    private static final RenderPipeline PIPELINE = RenderPipeline.builder()
        .withLocation(Identifier.fromNamespaceAndPath("toolscreen", "pipeline/composite"))
        .withVertexShader(Identifier.fromNamespaceAndPath("toolscreen", "core/composite"))
        .withFragmentShader(Identifier.fromNamespaceAndPath("toolscreen", "core/composite"))
        .withBindGroupLayout(BindGroupLayout.builder()
            .withUniform("InSampler", UniformType.COMBINED_IMAGE_SAMPLER)
            .withUniform("Layer", UniformType.UNIFORM_BUFFER).build())
        .withColorTargetState(new ColorTargetState(BlendFunction.TRANSLUCENT))
        .withDepthStencilState(Optional.empty())
        .withPrimitiveTopology(PrimitiveTopology.TRIANGLES)
        .withCull(false)
        .build();
    private static final class OutputTarget{
        GpuTexture texture;
        GpuTextureView view;
        void close(){if(view!=null)view.close();if(texture!=null)texture.close();view=null;texture=null;}
    }
    private static final OutputTarget DISPLAY=new OutputTarget();
    private static final OutputTarget PREVIEW=new OutputTarget();
    private static MappableRingBuffer uniforms;
    private static int renderWidth;
    private static int renderHeight;
    private static int stride = 256;
    private static ByteBuffer parameters;
    private static int layerCapacity;
    private static final long START_NANOS = System.nanoTime();

    private Compositor() {}

    public static void register() {
        PipelineAccessor.toolscreenRegister(PIPELINE);
    }

    public static void prepare(GameRenderer renderer, GameRenderState state) {
        if (!Toolscreen.resourcesReady()) return;
        Minecraft game = Minecraft.getInstance();
        Toolscreen.maximumTextureSize = RenderSystem.getDevice().getDeviceInfo().limits().maxTextureSize();
        int width = state.windowRenderState.width;
        int height = state.windowRenderState.height;
        if (width <= 0 || height <= 0) return;
        if (Toolscreen.failed) {
            width = game.getWindow().getWidth();
            height = game.getWindow().getHeight();
        }
        RenderTarget target = renderer.mainRenderTarget();
        if (renderWidth != width || renderHeight != height || target.width != width || target.height != height) {
            renderer.resize(width, height);
            renderWidth = width;
            renderHeight = height;
        }
    }

    public static GpuTextureView compose(CommandEncoder encoder, GpuTextureView source) {
        return composeForOutput(encoder,source,0);
    }

    public static GpuTextureView composeForOutput(CommandEncoder encoder,GpuTextureView source,int route){
        if (!Toolscreen.resourcesReady() || Toolscreen.failed) return source;
        Minecraft game = Minecraft.getInstance();
        int width = game.getWindow().getWidth();
        int height = game.getWindow().getHeight();
        if (width <= 0 || height <= 0) return source;
        boolean suspended = Toolscreen.suspended();
        double[] settings = NativeBridge.snapshot();
        if (Toolscreen.resizingSuspended()&&route==0) return source;
        GpuTexture texture = source.texture();
        float[] plan = NativeBridge.planForOutput(width, height, texture.getWidth(0), texture.getHeight(0), suspended, (System.nanoTime() - START_NANOS) / 1.0e9,route);
        if(((int)settings[133]&(1<<route))==0){settings[8]=settings[9]=settings[10]=0;}
        return renderForOutput(encoder, source, width, height, plan, settings,route);
    }

    public static GpuTextureView render(CommandEncoder encoder, GpuTextureView source, int width, int height, float[] plan, double[] settings) {
        return renderForOutput(encoder,source,width,height,plan,settings,0);
    }

    public static GpuTextureView renderForOutput(CommandEncoder encoder,GpuTextureView source,int width,int height,float[] plan,double[] settings,int route){
        GpuDevice device = RenderSystem.getDevice();
        OutputTarget output=route==0?DISPLAY:PREVIEW;
        ensureOutput(device,output,width,height,source.texture().getFormat());
        ensureUniforms(device,plan.length/20);
        parameters.clear();
        for (int layer = 0; layer < plan.length / 20; layer++) {
            for (int item = 0; item < 20; item++) parameters.putFloat(layer * stride + item * 4, plan[layer * 20 + item]);
        }
        parameters.limit(stride * layerCapacity);
        GpuBuffer buffer = uniforms.currentBuffer();
        encoder.writeToBuffer(buffer.slice(), parameters);
        try (RenderPass pass = encoder.createRenderPass(() -> "Macless compositor", output.view,
                Optional.of(new Vector4f((float) settings[8], (float) settings[9], (float) settings[10], 1)))) {
            pass.setPipeline(RenderSystem.getCompiledPipeline(PIPELINE));
            for (int layer = 0; layer < plan.length / 20; layer++) {
                int flags=(int)plan[layer*20+12];
                FilterMode filter=(flags&64)!=0?FilterMode.NEAREST:FilterMode.LINEAR;
                int sourceId=(int)plan[layer*20+19];
                GpuTextureView input=sourceId==0?source:AssetManager.texture(sourceId);
                if(input==null)continue;
                pass.setUniform("InSampler", input, RenderSystem.getSamplerCache().getClampToEdge(filter));
                pass.setUniform("Layer", buffer.slice((long) layer * stride, 80));
                pass.draw(6, 1, 0, 0);
            }
        }
        uniforms.rotate();
        return output.view;
    }

    private static void ensureOutput(GpuDevice device,OutputTarget output,int width,int height,GpuFormat format) {
        if (output.texture != null && output.texture.getWidth(0)==width&&output.texture.getHeight(0)==height&&output.texture.getFormat()==format)return;
        output.close();
        output.texture = device.createTexture("Macless output", GpuTexture.USAGE_RENDER_ATTACHMENT | GpuTexture.USAGE_TEXTURE_BINDING | GpuTexture.USAGE_COPY_SRC,
            format, width, height, 1, 1);
        output.view = device.createTextureView(output.texture);
    }

    private static void ensureUniforms(GpuDevice device,int count){
        if(uniforms!=null&&layerCapacity>=count)return;
        if(uniforms!=null)uniforms.close();
        int alignment=Math.max(1,device.getDeviceInfo().limits().minUniformOffsetAlignment());
        stride=((80+alignment-1)/alignment)*alignment;
        layerCapacity=Math.max(8,Integer.highestOneBit(Math.max(1,count-1))<<1);
        uniforms=new MappableRingBuffer(()->"Macless layer uniforms",GpuBuffer.USAGE_UNIFORM|GpuBuffer.USAGE_COPY_DST|GpuBuffer.USAGE_MAP_WRITE,stride*layerCapacity);
        parameters=ByteBuffer.allocateDirect(stride*layerCapacity).order(ByteOrder.nativeOrder());
    }

    public static void close() {
        DISPLAY.close();PREVIEW.close();
        if (uniforms != null) uniforms.close();
        uniforms = null;
    }
}
