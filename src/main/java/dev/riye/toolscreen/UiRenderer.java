package dev.riye.toolscreen;

import com.mojang.blaze3d.platform.NativeImage;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.blaze3d.vertex.DefaultVertexFormat;
import com.mojang.renderpearl.api.GpuFormat;
import com.mojang.renderpearl.api.buffers.GpuBuffer;
import com.mojang.renderpearl.api.commands.CommandEncoder;
import com.mojang.renderpearl.api.commands.RenderPass;
import com.mojang.renderpearl.api.pipeline.BindGroupLayout;
import com.mojang.renderpearl.api.pipeline.BlendFunction;
import com.mojang.renderpearl.api.pipeline.ColorTargetState;
import com.mojang.renderpearl.api.pipeline.PrimitiveTopology;
import com.mojang.renderpearl.api.pipeline.RenderPipeline;
import com.mojang.renderpearl.api.pipeline.UniformType;
import com.mojang.renderpearl.api.textures.FilterMode;
import com.mojang.renderpearl.api.textures.GpuTexture;
import com.mojang.renderpearl.api.textures.GpuTextureView;
import dev.riye.toolscreen.mixin.PipelineAccessor;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Optional;
import net.minecraft.client.renderer.MappableRingBuffer;
import net.minecraft.resources.Identifier;
import org.joml.Vector4f;

public final class UiRenderer {
    private static final RenderPipeline PIPELINE = RenderPipeline.builder()
        .withLocation(Identifier.fromNamespaceAndPath("toolscreen", "pipeline/imgui"))
        .withVertexShader(Identifier.fromNamespaceAndPath("toolscreen", "core/imgui"))
        .withFragmentShader(Identifier.fromNamespaceAndPath("toolscreen", "core/imgui"))
        .withBindGroupLayout(BindGroupLayout.builder()
            .withUniform("FontSampler", UniformType.COMBINED_IMAGE_SAMPLER)
            .withUniform("Screen", UniformType.UNIFORM_BUFFER).build())
        .withVertexBinding(0, DefaultVertexFormat.POSITION_TEX_COLOR)
        .withColorTargetState(new ColorTargetState(Optional.of(BlendFunction.TRANSLUCENT), GpuFormat.RGBA16_FLOAT, ColorTargetState.WRITE_ALL))
        .withDepthStencilState(Optional.empty())
        .withPrimitiveTopology(PrimitiveTopology.TRIANGLES)
        .withCull(false)
        .build();
    private static volatile byte[] frame;
    private static GpuTexture fontTexture;
    private static long fontGeneration;
    private static GpuTextureView fontView;
    private static GpuTexture previewTexture;
    private static GpuTextureView previewView;
    private static MappableRingBuffer vertices;
    private static MappableRingBuffer uniforms;
    private static ByteBuffer vertexData;
    private static ByteBuffer screenData;
    private static int uniformStride;
    private static boolean previewFrozen;

    private UiRenderer() {}

    public static void register() {
        PipelineAccessor.toolscreenRegister(PIPELINE);
        UiColorCompositor.register();
    }

    public static void publish(byte[] data) {
        frame = data;
    }

    public static void render(CommandEncoder encoder, GpuTextureView target) {
        render(encoder,target,frame);
    }

    public static void render(CommandEncoder encoder,GpuTextureView target,byte[] packet){
        if (packet == null || packet.length < 8) return;
        ByteBuffer metadata = ByteBuffer.wrap(packet).order(ByteOrder.nativeOrder());
        int vertexCount = metadata.getInt();
        int commandCount = metadata.getInt();
        if (vertexCount <= 0 || vertexCount > 1000000 || commandCount < 0 || commandCount > 10000) return;
        int vertexBytes = Math.multiplyExact(vertexCount, 24);
        if (packet.length != 8L + vertexBytes + commandCount * 32L) return;
        initializeFont(encoder);
        ensureBuffers(vertexBytes);
        vertexData.clear();
        vertexData.put(packet, 8, vertexBytes).flip();
        var vertexBuffer = vertices.currentBuffer();
        var uniformBuffer = uniforms.currentBuffer();
        encoder.writeToBuffer(vertexBuffer.slice(0, vertexBytes), vertexData);
        int width = target.texture().getWidth(0);
        int height = target.texture().getHeight(0);
        UiRegion region = UiRegion.from(metadata, vertexCount, width, height);
        if (region.width() == 0 || region.height() == 0) return;
        boolean needsPreview=false;
        boolean needsComposition=false;
        int previewRoute=0;for(int command=0;command<commandCount;command++){int textureId=metadata.getInt(8+vertexBytes+command*32+24);if(textureId>=2&&textureId<=4)needsPreview=true;if(textureId==3||textureId==4)needsComposition=true;if(textureId==4)previewRoute=1;}
        if(needsPreview&&!previewFrozen)updatePreview(encoder,target,width,height);
        GpuTextureView composition=null;
        if(needsComposition&&previewView!=null){int[] virtual=NativeBridge.viewport(width,height,Toolscreen.maximumTextureSize,false);float[] plan=NativeBridge.planForOutput(width,height,virtual[0],virtual[1],false,System.nanoTime()/1.0e9,previewRoute);double[] settings=NativeBridge.snapshot();if(((int)settings[133]&(1<<previewRoute))==0)settings[8]=settings[9]=settings[10]=0;composition=Compositor.renderForOutput(encoder,previewView,width,height,plan,settings,2);}
        screenData.clear();
        screenData.putFloat(region.width()).putFloat(region.height()).putFloat(0).putFloat(0);
        screenData.putFloat(region.left()).putFloat(region.top()).putFloat(0).putFloat(0);
        screenData.position(uniformStride);
        screenData.putFloat(region.width()).putFloat(region.height()).putFloat(1).putFloat(0);
        screenData.putFloat(region.left()).putFloat(region.top()).putFloat(0).putFloat(0);
        screenData.position(0).limit(uniformStride*2);
        encoder.writeToBuffer(uniformBuffer.slice(), screenData);
        GpuTextureView linearTarget = UiColorCompositor.prepare(encoder, target, region);
        try (RenderPass pass = encoder.createRenderPass(() -> "Macless Dear ImGui", linearTarget, Optional.of(new Vector4f(0, 0, 0, 0)))) {
            pass.setPipeline(RenderSystem.getCompiledPipeline(PIPELINE));
            pass.setVertexBuffer(0, vertexBuffer.slice(0, vertexBytes));
            metadata.position(8 + vertexBytes);
            for (int command = 0; command < commandCount; command++) {
                int first = metadata.getInt();
                int count = metadata.getInt();
                float x = metadata.getFloat();
                float y = metadata.getFloat();
                float clipWidth = metadata.getFloat();
                float clipHeight = metadata.getFloat();
                int textureId=metadata.getInt();
                metadata.getInt();
                GpuTextureView texture=textureId==1?fontView:textureId==2?previewView:textureId==3||textureId==4?composition:AssetManager.texture(textureId);
                if(texture==null)continue;
                int x0 = Math.max(region.left(), (int) Math.floor(x));
                int y0 = Math.max(region.top(), (int) Math.floor(y));
                int x1 = Math.min(region.left() + region.width(), (int) Math.ceil(x + clipWidth));
                int y1 = Math.min(region.bottom(), (int) Math.ceil(y + clipHeight));
                if (count <= 0 || first < 0 || first + count > vertexCount || x1 <= x0 || y1 <= y0) continue;
                pass.setUniform("FontSampler",texture,RenderSystem.getSamplerCache().getClampToEdge(FilterMode.LINEAR,textureId==1));
                pass.setUniform("Screen",uniformBuffer.slice(textureId>=2&&textureId<=4?uniformStride:0,32));
                pass.enableScissor(x0-region.left(), region.bottom()-y1, x1-x0, y1-y0);
                pass.draw(count, 1, first, 0);
            }
            pass.disableScissor();
        }
        UiColorCompositor.resolve(encoder, target);
        vertices.rotate();
        uniforms.rotate();
    }

    private static void ensureBuffers(int size) {
        if (vertices != null && vertices.size() >= size) return;
        if (vertices != null) vertices.close();
        int capacity = Integer.highestOneBit(Math.max(size-1, 65535)) << 1;
        vertices = new MappableRingBuffer(() -> "Macless ImGui vertices", GpuBuffer.USAGE_VERTEX | GpuBuffer.USAGE_COPY_DST | GpuBuffer.USAGE_MAP_WRITE, capacity);
        vertexData = ByteBuffer.allocateDirect(capacity).order(ByteOrder.nativeOrder());
        if(uniforms==null){uniformStride=Math.max(32,RenderSystem.getDevice().getDeviceInfo().limits().minUniformOffsetAlignment());uniforms=new MappableRingBuffer(()->"Macless ImGui display",GpuBuffer.USAGE_UNIFORM|GpuBuffer.USAGE_COPY_DST|GpuBuffer.USAGE_MAP_WRITE,uniformStride*2);screenData=ByteBuffer.allocateDirect(uniformStride*2).order(ByteOrder.nativeOrder());}
    }

    private static void updatePreview(CommandEncoder encoder,GpuTextureView source,int width,int height){
        if(previewTexture==null||previewTexture.getWidth(0)!=width||previewTexture.getHeight(0)!=height||previewTexture.getFormat()!=source.texture().getFormat()){
            if(previewView!=null)previewView.close();if(previewTexture!=null)previewTexture.close();
            previewTexture=RenderSystem.getDevice().createTexture("Macless live preview",GpuTexture.USAGE_TEXTURE_BINDING|GpuTexture.USAGE_COPY_DST,source.texture().getFormat(),width,height,1,1);
            previewView=RenderSystem.getDevice().createTextureView(previewTexture);
        }
        encoder.copyTextureToTexture(source.texture(),previewTexture,0,0,0,0,0,width,height);
    }

    public static void freezePreview(GpuTextureView source){
        updatePreview(RenderSystem.getDevice().createCommandEncoder(),source,source.texture().getWidth(0),source.texture().getHeight(0));
        previewFrozen=true;
    }
    public static void livePreview(CommandEncoder encoder,GpuTextureView source){updatePreview(encoder,source,source.texture().getWidth(0),source.texture().getHeight(0));previewFrozen=true;}

    public static void releasePreview(){previewFrozen=false;}

    private static void initializeFont(CommandEncoder encoder) {
        long generation = NativeBridge.fontRevision();
        if (fontTexture != null && fontGeneration == generation) return;
        if (fontView != null) fontView.close();
        if (fontTexture != null) fontTexture.close();
        byte[] atlas = NativeBridge.fontAtlas();
        ByteBuffer data = ByteBuffer.wrap(atlas).order(ByteOrder.nativeOrder());
        int width = data.getInt();
        int height = data.getInt();
        if (width <= 0 || height <= 0 || atlas.length != 8L + width * (long) height * 4) throw new IllegalStateException("Invalid ImGui font atlas");
        var device = RenderSystem.getDevice();
        int mipLevels = Math.min(3, 32 - Integer.numberOfLeadingZeros(Math.max(width, height)));
        fontTexture = device.createTexture("Macless ImGui font", GpuTexture.USAGE_TEXTURE_BINDING | GpuTexture.USAGE_COPY_DST, GpuFormat.RGBA8_UNORM, width, height, 1, mipLevels);
        fontView = device.createTextureView(fontTexture);
        fontGeneration = generation;
        try (NativeImage image = new NativeImage(width, height, false)) {
            image.getPixelBytes().put(atlas, 8, atlas.length - 8);
            encoder.writeToTexture(fontTexture, image);
            for (int level = 1; level < mipLevels; level++) {
                try (NativeImage reduced = new NativeImage(Math.max(1, width >> level), Math.max(1, height >> level), false)) {
                    image.resizeSubRectTo(0, 0, width, height, reduced);
                    encoder.writeToTexture(fontTexture, reduced, level, 0, 0, 0);
                }
            }
        }
    }

    public static void close() {
        UiColorCompositor.close();
        if (vertices != null) vertices.close();
        if (uniforms != null) uniforms.close();
        if (fontView != null) fontView.close();
        if (fontTexture != null) fontTexture.close();
        if(previewView!=null)previewView.close();if(previewTexture!=null)previewTexture.close();
        vertices = null;
        uniforms = null;
        fontView = null;
        fontTexture = null;
        previewView=null;previewTexture=null;
        frame = null;
    }
}
