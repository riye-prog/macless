package dev.riye.toolscreen;

import com.mojang.blaze3d.pipeline.RenderTarget;
import java.awt.image.BufferedImage;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Optional;
import javax.imageio.ImageIO;
import net.minecraft.client.Minecraft;
import org.lwjgl.opengl.EXTFramebufferObject;
import org.lwjgl.opengl.ARBSamplerObjects;
import org.lwjgl.glfw.GLFW;
import org.lwjgl.opengl.GL;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL12;
import org.lwjgl.opengl.GL13;
import org.lwjgl.opengl.GL14;
import org.lwjgl.opengl.GL15;
import org.lwjgl.opengl.GL20;
import org.lwjgl.opengl.GL21;
import org.lwjgl.opengl.GL30;
import org.lwjgl.system.MemoryUtil;
import org.lwjgl.system.MemoryStack;

public final class LegacyRenderer {
    private static final String COMPOSITE_VERTEX="""
        #version 120
        uniform vec4 Source;
        uniform vec4 Destination;
        varying vec2 texCoord;
        varying vec2 localCoord;
        void main(){vec2 uv=gl_MultiTexCoord0.xy;gl_Position=vec4((Destination.xy+uv*Destination.zw)*2.0-1.0,0.0,1.0);texCoord=Source.xy+uv*Source.zw;localCoord=uv;}
        """;
    private static final String COMPOSITE_FRAGMENT="""
        #version 120
        uniform sampler2D InSampler;
        uniform vec4 Source;
        uniform vec4 Destination;
        uniform vec4 KeyColor;
        uniform vec4 Effects;
        uniform vec4 Dimensions;
        varying vec2 texCoord;
        varying vec2 localCoord;
        bool flag(float bit){return mod(floor(Effects.x/bit),2.0)>0.5;}
        void main(){
            if(flag(8.0)){gl_FragColor=vec4(KeyColor.rgb,Effects.w);return;}
            if(flag(32.0)){float y=Destination.y+localCoord.y*Destination.w;gl_FragColor=vec4(mix(Source.rgb,KeyColor.rgb,y),1.0);return;}
            vec4 color=texture2D(InSampler,vec2(texCoord.x,flag(16.0)?1.0-texCoord.y:texCoord.y));
            if(flag(512.0))color.a=1.0;
            if(flag(128.0)){float edge=1e10;if(Source.x<0.000001)edge=min(edge,localCoord.x*Dimensions.x);if(Source.x+Source.z>0.999999)edge=min(edge,(1.0-localCoord.x)*Dimensions.x);if(Source.y<0.000001)edge=min(edge,localCoord.y*Dimensions.y);if(Source.y+Source.w>0.999999)edge=min(edge,(1.0-localCoord.y)*Dimensions.y);if(edge<Effects.y){gl_FragColor=vec4(KeyColor.rgb,1.0);return;}}
            float foreground=smoothstep(Effects.y,Effects.y+0.03,distance(color.rgb,KeyColor.rgb));
            if(flag(4.0)){foreground=1.0-smoothstep(Effects.y,Effects.y+0.005,distance(color.rgb,KeyColor.rgb));color.a*=foreground;if(flag(256.0))color.rgb=vec3(1.0);}else if(flag(1.0))color.a*=foreground;
            if(flag(2.0)&&foreground>0.5)color.rgb=0.55+0.45*cos(6.283185*(Effects.z*0.18+localCoord.x+vec3(0.0,0.3333,0.6667)));
            color.a*=Effects.w;gl_FragColor=color;
        }
        """;
    private static final String UI_VERTEX="""
        #version 120
        uniform vec4 DisplaySize;
        uniform vec2 Origin;
        varying vec2 texCoord;
        varying vec4 vertexColor;
        void main(){gl_Position=vec4((gl_Vertex.x-Origin.x)/DisplaySize.x*2.0-1.0,1.0-(gl_Vertex.y-Origin.y)/DisplaySize.y*2.0,0.0,1.0);texCoord=vec2(gl_MultiTexCoord0.x,DisplaySize.z>0.5?1.0-gl_MultiTexCoord0.y:gl_MultiTexCoord0.y);vertexColor=gl_Color;}
        """;
    private static final String UI_FRAGMENT="""
        #version 120
        uniform sampler2D FontSampler;
        varying vec2 texCoord;
        varying vec4 vertexColor;
        vec3 decodeColor(vec3 value){return mix(value/12.92,pow((value+0.055)/1.055,vec3(2.4)),step(vec3(0.04045),value));}
        void main(){vec4 sampled=texture2D(FontSampler,texCoord);gl_FragColor=vec4(decodeColor(vertexColor.rgb)*decodeColor(sampled.rgb),vertexColor.a*sampled.a);}
        """;
    private static final String UI_RESOLVE_VERTEX="""
        #version 120
        uniform vec4 Destination;
        varying vec2 texCoord;
        varying vec2 sceneCoord;
        void main(){texCoord=gl_MultiTexCoord0.xy;sceneCoord=Destination.xy+texCoord*Destination.zw;gl_Position=vec4(sceneCoord*2.0-1.0,0.0,1.0);}
        """;
    private static final String UI_RESOLVE_FRAGMENT="""
        #version 120
        uniform sampler2D SceneSampler;
        uniform sampler2D UiSampler;
        varying vec2 texCoord;
        varying vec2 sceneCoord;
        vec3 decodeColor(vec3 value){return mix(value/12.92,pow((value+0.055)/1.055,vec3(2.4)),step(vec3(0.04045),value));}
        vec3 encodeColor(vec3 value){value=max(value,vec3(0));return mix(value*12.92,1.055*pow(value,vec3(1.0/2.4))-0.055,step(vec3(0.0031308),value));}
        void main(){vec4 scene=texture2D(SceneSampler,sceneCoord);vec4 ui=texture2D(UiSampler,texCoord);gl_FragColor=vec4(encodeColor(ui.rgb+decodeColor(scene.rgb)*(1.0-ui.a)),ui.a+scene.a*(1.0-ui.a));}
        """;
    public static byte[] uiPacket;
    private static final Framebuffer DISPLAY=new Framebuffer(),PREVIEW=new Framebuffer(),FROZEN=new Framebuffer(),UI_SCENE=new Framebuffer(),UI_LINEAR=new Framebuffer();
    private static int compositeProgram,uiProgram,uiResolveProgram,fontTexture,vertexBuffer;
    private static long fontGeneration;
    private static int[] compositeUniforms;
    private static ByteBuffer vertexBytes,readback;
    private static boolean frozen;

    private LegacyRenderer(){}
    private static final class Framebuffer{
        int id,texture,width,height,format;
        void ensure(int width,int height){ensure(width,height,GL11.GL_RGBA8);}
        void ensure(int width,int height,int format){if(texture!=0&&this.width==width&&this.height==height&&this.format==format)return;close();this.width=width;this.height=height;this.format=format;texture=GL11.glGenTextures();GL11.glBindTexture(GL11.GL_TEXTURE_2D,texture);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MIN_FILTER,GL11.GL_NEAREST);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MAG_FILTER,GL11.GL_NEAREST);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_WRAP_S,33071);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_WRAP_T,33071);GL11.glTexImage2D(GL11.GL_TEXTURE_2D,0,format,width,height,0,GL11.GL_RGBA,GL11.GL_UNSIGNED_BYTE,(ByteBuffer)null);id=GL.getCapabilities().OpenGL30?GL30.glGenFramebuffers():EXTFramebufferObject.glGenFramebuffersEXT();bind(id);if(GL.getCapabilities().OpenGL30)GL30.glFramebufferTexture2D(36160,36064,GL11.GL_TEXTURE_2D,texture,0);else EXTFramebufferObject.glFramebufferTexture2DEXT(36160,36064,GL11.GL_TEXTURE_2D,texture,0);int status=GL.getCapabilities().OpenGL30?GL30.glCheckFramebufferStatus(36160):EXTFramebufferObject.glCheckFramebufferStatusEXT(36160);if(status!=36053)throw new IllegalStateException("Incomplete Toolscreen framebuffer: "+status);}
        void close(){if(id!=0){if(GL.getCapabilities().OpenGL30)GL30.glDeleteFramebuffers(id);else EXTFramebufferObject.glDeleteFramebuffersEXT(id);}if(texture!=0)GL11.glDeleteTextures(texture);id=texture=0;}
    }
    private static final class State implements AutoCloseable{
        final int framebuffer=GL11.glGetInteger(36006),program=GL11.glGetInteger(GL20.GL_CURRENT_PROGRAM),array=GL11.glGetInteger(GL15.GL_ARRAY_BUFFER_BINDING),pack=GL11.glGetInteger(GL21.GL_PIXEL_PACK_BUFFER_BINDING),unpack=GL11.glGetInteger(GL21.GL_PIXEL_UNPACK_BUFFER_BINDING),active=GL11.glGetInteger(GL13.GL_ACTIVE_TEXTURE),client=GL11.glGetInteger(GL13.GL_CLIENT_ACTIVE_TEXTURE);
        final boolean samplerObjects=GL.getCapabilities().OpenGL33||GL.getCapabilities().GL_ARB_sampler_objects;
        final int sampler,samplerSecond;
        State(){GL11.glPushAttrib(GL11.GL_ALL_ATTRIB_BITS);GL11.glPushClientAttrib(GL11.GL_CLIENT_ALL_ATTRIB_BITS);GL13.glActiveTexture(GL13.GL_TEXTURE0);GL13.glClientActiveTexture(GL13.GL_TEXTURE0);sampler=samplerObjects?GL11.glGetInteger(0x8919):0;GL13.glActiveTexture(GL13.GL_TEXTURE1);samplerSecond=samplerObjects?GL11.glGetInteger(0x8919):0;if(samplerObjects)ARBSamplerObjects.glBindSampler(1,0);GL13.glActiveTexture(GL13.GL_TEXTURE0);if(samplerObjects)ARBSamplerObjects.glBindSampler(0,0);GL15.glBindBuffer(GL21.GL_PIXEL_PACK_BUFFER,0);GL15.glBindBuffer(GL21.GL_PIXEL_UNPACK_BUFFER,0);GL11.glPixelStorei(GL11.GL_UNPACK_ALIGNMENT,1);GL11.glPixelStorei(GL12.GL_UNPACK_ROW_LENGTH,0);GL11.glPixelStorei(GL12.GL_UNPACK_SKIP_ROWS,0);GL11.glPixelStorei(GL12.GL_UNPACK_SKIP_PIXELS,0);if(GL.getCapabilities().OpenGL30||GL.getCapabilities().GL_EXT_framebuffer_sRGB||GL.getCapabilities().GL_ARB_framebuffer_sRGB)GL11.glDisable(0x8db9);GL11.glDisable(GL11.GL_DEPTH_TEST);GL11.glDisable(GL11.GL_CULL_FACE);GL11.glDisable(GL11.GL_ALPHA_TEST);GL11.glDisable(GL11.GL_SCISSOR_TEST);GL11.glDisable(GL11.GL_FOG);GL11.glDisable(GL11.GL_STENCIL_TEST);GL11.glDisable(GL11.GL_COLOR_LOGIC_OP);GL11.glDisable(GL11.GL_DITHER);GL11.glDisable(GL11.GL_POLYGON_STIPPLE);GL11.glDisable(GL11.GL_POLYGON_SMOOTH);GL11.glDisable(GL13.GL_SAMPLE_ALPHA_TO_COVERAGE);GL11.glDisable(GL13.GL_SAMPLE_ALPHA_TO_ONE);GL11.glDisable(GL13.GL_SAMPLE_COVERAGE);GL11.glPolygonMode(GL11.GL_FRONT_AND_BACK,GL11.GL_FILL);GL11.glDepthMask(false);GL11.glColorMask(true,true,true,true);GL11.glEnable(GL11.GL_BLEND);GL14.glBlendFuncSeparate(GL11.GL_SRC_ALPHA,GL11.GL_ONE_MINUS_SRC_ALPHA,GL11.GL_ONE,GL11.GL_ONE_MINUS_SRC_ALPHA);GL20.glBlendEquationSeparate(GL14.GL_FUNC_ADD,GL14.GL_FUNC_ADD);}
        @Override public void close(){bind(framebuffer);GL20.glUseProgram(program);GL11.glPopClientAttrib();GL11.glPopAttrib();GL15.glBindBuffer(GL15.GL_ARRAY_BUFFER,array);GL15.glBindBuffer(GL21.GL_PIXEL_PACK_BUFFER,pack);GL15.glBindBuffer(GL21.GL_PIXEL_UNPACK_BUFFER,unpack);if(samplerObjects){ARBSamplerObjects.glBindSampler(0,sampler);ARBSamplerObjects.glBindSampler(1,samplerSecond);}GL13.glActiveTexture(active);GL13.glClientActiveTexture(client);}
    }
    public static int[] displayInfo(){var window=Minecraft.getInstance().getWindow();try(MemoryStack stack=MemoryStack.stackPush()){var width=stack.mallocInt(1);var height=stack.mallocInt(1);var pointsWidth=stack.mallocInt(1);var pointsHeight=stack.mallocInt(1);GLFW.glfwGetFramebufferSize(window.getWindow(),width,height);GLFW.glfwGetWindowSize(window.getWindow(),pointsWidth,pointsHeight);return new int[]{width.get(0)>0?width.get(0):window.getWidth(),height.get(0)>0?height.get(0):window.getHeight(),Math.max(1,pointsWidth.get(0)),Math.max(1,pointsHeight.get(0))};}}
    public static int[] displaySize(){int[] info=displayInfo();return new int[]{info[0],info[1]};}
    private static void bind(int framebuffer){if(GL.getCapabilities().OpenGL30)GL30.glBindFramebuffer(36160,framebuffer);else EXTFramebufferObject.glBindFramebufferEXT(36160,framebuffer);}
    public static void prepare(){Toolscreen.maximumTextureSize=GL11.glGetInteger(GL11.GL_MAX_TEXTURE_SIZE);Minecraft game=Minecraft.getInstance();int[] size=Toolscreen.viewport();RenderTarget target=game.getMainRenderTarget();if(target.width!=size[0]||target.height!=size[1]){target.resize(size[0],size[1],Minecraft.ON_OSX);game.gameRenderer.resize(size[0],size[1]);target.bindWrite(true);}}
    private static int shader(int type,String source){int shader=GL20.glCreateShader(type);GL20.glShaderSource(shader,source);GL20.glCompileShader(shader);if(GL20.glGetShaderi(shader,GL20.GL_COMPILE_STATUS)==0)throw new IllegalStateException(GL20.glGetShaderInfoLog(shader));return shader;}
    private static int program(String vertex,String fragment){int first=shader(GL20.GL_VERTEX_SHADER,vertex),second=shader(GL20.GL_FRAGMENT_SHADER,fragment),program=GL20.glCreateProgram();GL20.glAttachShader(program,first);GL20.glAttachShader(program,second);GL20.glLinkProgram(program);GL20.glDeleteShader(first);GL20.glDeleteShader(second);if(GL20.glGetProgrami(program,GL20.GL_LINK_STATUS)==0)throw new IllegalStateException(GL20.glGetProgramInfoLog(program));return program;}
    private static void initialize(){if(compositeProgram==0){compositeProgram=program(COMPOSITE_VERTEX,COMPOSITE_FRAGMENT);uiProgram=program(UI_VERTEX,UI_FRAGMENT);uiResolveProgram=program(UI_RESOLVE_VERTEX,UI_RESOLVE_FRAGMENT);compositeUniforms=new int[5];String[] names={"Source","Destination","KeyColor","Effects","Dimensions"};for(int i=0;i<5;i++)compositeUniforms[i]=GL20.glGetUniformLocation(compositeProgram,names[i]);vertexBuffer=GL15.glGenBuffers();}initializeFont();}
    private static void initializeFont(){long generation=NativeBridge.fontRevision();if(fontTexture!=0&&fontGeneration==generation)return;if(fontTexture!=0)GL11.glDeleteTextures(fontTexture);byte[] bytes=NativeBridge.fontAtlas();ByteBuffer header=ByteBuffer.wrap(bytes).order(ByteOrder.nativeOrder());int width=header.getInt(),height=header.getInt();ByteBuffer pixels=MemoryUtil.memAlloc(width*height*4);pixels.put(bytes,8,width*height*4).flip();fontTexture=GL11.glGenTextures();GL11.glBindTexture(GL11.GL_TEXTURE_2D,fontTexture);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MIN_FILTER,GL11.GL_LINEAR_MIPMAP_LINEAR);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MAG_FILTER,GL11.GL_LINEAR);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_WRAP_S,33071);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_WRAP_T,33071);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL12.GL_TEXTURE_BASE_LEVEL,0);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL12.GL_TEXTURE_MAX_LEVEL,2);GL11.glTexImage2D(GL11.GL_TEXTURE_2D,0,GL11.GL_RGBA8,width,height,0,GL11.GL_RGBA,GL11.GL_UNSIGNED_BYTE,pixels);if(GL.getCapabilities().OpenGL30)GL30.glGenerateMipmap(GL11.GL_TEXTURE_2D);else EXTFramebufferObject.glGenerateMipmapEXT(GL11.GL_TEXTURE_2D);LegacyMemory.free(pixels);fontGeneration=generation;}
    private static void quad(){GL11.glBegin(GL11.GL_TRIANGLE_STRIP);GL11.glTexCoord2f(0,0);GL11.glVertex2f(0,0);GL11.glTexCoord2f(1,0);GL11.glVertex2f(1,0);GL11.glTexCoord2f(0,1);GL11.glVertex2f(0,1);GL11.glTexCoord2f(1,1);GL11.glVertex2f(1,1);GL11.glEnd();}
    private static void compose(Framebuffer output,int source,int sourceWidth,int sourceHeight,int width,int height,int route,boolean suspended){output.ensure(width,height);bind(output.id);GL11.glViewport(0,0,width,height);GL11.glDisable(GL11.GL_SCISSOR_TEST);double[] settings=NativeBridge.snapshot();boolean background=((int)settings[133]&(1<<route))!=0;GL11.glClearColor(background?(float)settings[8]:0,background?(float)settings[9]:0,background?(float)settings[10]:0,1);GL11.glClear(GL11.GL_COLOR_BUFFER_BIT);GL20.glUseProgram(compositeProgram);GL20.glUniform1i(GL20.glGetUniformLocation(compositeProgram,"InSampler"),0);float[] plan=NativeBridge.planForOutput(width,height,sourceWidth,sourceHeight,suspended,System.nanoTime()/1.0e9,route);for(int offset=0;offset<plan.length;offset+=20){int sourceId=(int)plan[offset+19],texture=sourceId==0?source:LegacyAssets.texture(sourceId);if(texture==0)continue;GL11.glBindTexture(GL11.GL_TEXTURE_2D,texture);int oldMin=GL11.glGetTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MIN_FILTER),oldMag=GL11.glGetTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MAG_FILTER);int filter=((int)plan[offset+12]&64)!=0?GL11.GL_NEAREST:GL11.GL_LINEAR;GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MIN_FILTER,filter);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MAG_FILTER,filter);for(int block=0;block<5;block++){int index=offset+block*4;GL20.glUniform4f(compositeUniforms[block],plan[index],plan[index+1],plan[index+2],plan[index+3]);}quad();GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MIN_FILTER,oldMin);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MAG_FILTER,oldMag);}}
    private static void blit(int texture,int framebuffer,int width,int height){bind(framebuffer);GL11.glDrawBuffer(framebuffer==0?(GL11.glGetBoolean(GL11.GL_DOUBLEBUFFER)?GL11.GL_BACK:GL11.GL_FRONT):36064);GL11.glViewport(0,0,width,height);GL11.glDisable(GL11.GL_SCISSOR_TEST);GL20.glUseProgram(compositeProgram);GL20.glUniform1i(GL20.glGetUniformLocation(compositeProgram,"InSampler"),0);GL11.glBindTexture(GL11.GL_TEXTURE_2D,texture);GL20.glUniform4f(compositeUniforms[0],0,0,1,1);GL20.glUniform4f(compositeUniforms[1],0,0,1,1);GL20.glUniform4f(compositeUniforms[2],0,0,0,1);GL20.glUniform4f(compositeUniforms[3],512,0,0,1);GL20.glUniform4f(compositeUniforms[4],width,height,0,0);quad();}
    private static void prepareUiScene(int framebuffer,int width,int height,UiRegion region){UI_SCENE.ensure(width,height);UI_LINEAR.ensure(region.width(),region.height(),0x881a);bind(framebuffer);GL11.glReadBuffer(framebuffer==0?(GL11.glGetBoolean(GL11.GL_DOUBLEBUFFER)?GL11.GL_BACK:GL11.GL_FRONT):36064);GL13.glActiveTexture(GL13.GL_TEXTURE0);GL11.glBindTexture(GL11.GL_TEXTURE_2D,UI_SCENE.texture);int bottom=height-region.bottom();GL11.glCopyTexSubImage2D(GL11.GL_TEXTURE_2D,0,region.left(),bottom,region.left(),bottom,region.width(),region.height());}
    private static void resolveUi(int framebuffer,int width,int height,UiRegion region){bind(framebuffer);GL11.glViewport(0,0,width,height);GL11.glDisable(GL11.GL_SCISSOR_TEST);GL11.glDisable(GL11.GL_BLEND);GL20.glUseProgram(uiResolveProgram);GL20.glUniform1i(GL20.glGetUniformLocation(uiResolveProgram,"SceneSampler"),0);GL20.glUniform1i(GL20.glGetUniformLocation(uiResolveProgram,"UiSampler"),1);GL20.glUniform4f(GL20.glGetUniformLocation(uiResolveProgram,"Destination"),(float)region.left()/width,(float)(height-region.bottom())/height,(float)region.width()/width,(float)region.height()/height);GL13.glActiveTexture(GL13.GL_TEXTURE1);GL11.glBindTexture(GL11.GL_TEXTURE_2D,UI_LINEAR.texture);GL13.glActiveTexture(GL13.GL_TEXTURE0);GL11.glBindTexture(GL11.GL_TEXTURE_2D,UI_SCENE.texture);quad();GL11.glEnable(GL11.GL_BLEND);}
    private static void renderUi(byte[] packet,int framebuffer,int width,int height,int source,int sourceWidth,int sourceHeight){
        if(packet==null||packet.length<8)return;ByteBuffer metadata=ByteBuffer.wrap(packet).order(ByteOrder.nativeOrder());int vertices=metadata.getInt(),commands=metadata.getInt();if(vertices<=0||vertices>1000000||commands<0||packet.length!=8L+vertices*24L+commands*32L)return;UiRegion region=UiRegion.from(metadata,vertices,width,height);if(region.width()==0||region.height()==0)return;boolean preview=false;int previewRoute=0;for(int i=0;i<commands;i++){int id=metadata.getInt(8+vertices*24+i*32+24);if(id==3||id==4)preview=true;if(id==4)previewRoute=1;}if(preview){int[] view=NativeBridge.viewport(width,height,Toolscreen.maximumTextureSize,false);compose(PREVIEW,frozen?FROZEN.texture:source,view[0],view[1],width,height,previewRoute,false);}
        prepareUiScene(framebuffer,width,height,region);bind(UI_LINEAR.id);GL11.glViewport(0,0,region.width(),region.height());GL11.glDisable(GL11.GL_SCISSOR_TEST);GL11.glClearColor(0,0,0,0);GL11.glClear(GL11.GL_COLOR_BUFFER_BIT);GL20.glUseProgram(uiProgram);GL20.glUniform1i(GL20.glGetUniformLocation(uiProgram,"FontSampler"),0);GL20.glUniform2f(GL20.glGetUniformLocation(uiProgram,"Origin"),region.left(),region.top());int bytes=vertices*24;if(vertexBytes==null||vertexBytes.capacity()<bytes){if(vertexBytes!=null)LegacyMemory.free(vertexBytes);vertexBytes=MemoryUtil.memAlloc(Integer.highestOneBit(bytes-1)<<1);}vertexBytes.clear();vertexBytes.put(packet,8,bytes).flip();GL15.glBindBuffer(GL15.GL_ARRAY_BUFFER,vertexBuffer);GL15.glBufferData(GL15.GL_ARRAY_BUFFER,vertexBytes,GL15.GL_STREAM_DRAW);GL11.glEnableClientState(GL11.GL_VERTEX_ARRAY);GL11.glEnableClientState(GL11.GL_TEXTURE_COORD_ARRAY);GL11.glEnableClientState(GL11.GL_COLOR_ARRAY);GL11.glVertexPointer(3,GL11.GL_FLOAT,24,0L);GL11.glTexCoordPointer(2,GL11.GL_FLOAT,24,12L);GL11.glColorPointer(4,GL11.GL_UNSIGNED_BYTE,24,20L);GL11.glEnable(GL11.GL_SCISSOR_TEST);metadata.position(8+bytes);
        for(int i=0;i<commands;i++){int first=metadata.getInt(),count=metadata.getInt();float x=metadata.getFloat(),y=metadata.getFloat(),w=metadata.getFloat(),h=metadata.getFloat();int textureId=metadata.getInt();metadata.getInt();int texture=textureId==1?fontTexture:textureId==2?FROZEN.texture:textureId==3||textureId==4?PREVIEW.texture:LegacyAssets.texture(textureId);int left=Math.max(region.left(),(int)Math.floor(x)),top=Math.max(region.top(),(int)Math.floor(y)),right=Math.min(region.left()+region.width(),(int)Math.ceil(x+w)),bottom=Math.min(region.bottom(),(int)Math.ceil(y+h));if(texture==0||count<=0||first<0||first+count>vertices||right<=left||bottom<=top)continue;GL20.glUniform4f(GL20.glGetUniformLocation(uiProgram,"DisplaySize"),region.width(),region.height(),textureId>=2&&textureId<=4?1:0,0);GL11.glBindTexture(GL11.GL_TEXTURE_2D,texture);GL11.glScissor(left-region.left(),region.bottom()-bottom,right-left,bottom-top);GL11.glDrawArrays(GL11.GL_TRIANGLES,first,count);}GL11.glDisable(GL11.GL_SCISSOR_TEST);GL11.glDisableClientState(GL11.GL_VERTEX_ARRAY);GL11.glDisableClientState(GL11.GL_TEXTURE_COORD_ARRAY);GL11.glDisableClientState(GL11.GL_COLOR_ARRAY);resolveUi(framebuffer,width,height,region);
    }
    public static void verifyUiColor()throws Exception{
        try(State state=new State()){
            initialize();Framebuffer target=new Framebuffer();
            try{
                target.ensure(8,8);bind(target.id);GL11.glClearColor(0,0,1,1);GL11.glClear(GL11.GL_COLOR_BUFFER_BIT);UiRegion region=new UiRegion(2,1,3,3);prepareUiScene(target.id,8,8,region);bind(UI_LINEAR.id);GL11.glClearColor(.5f,.5f,.5f,.5f);GL11.glClear(GL11.GL_COLOR_BUFFER_BIT);resolveUi(target.id,8,8,region);
                ByteBuffer pixels=read(target.id,8,8);int center=(5*8+3)*4,red=pixels.get(center)&255,green=pixels.get(center+1)&255,blue=pixels.get(center+2)&255;
                boolean valid=Math.abs(red-188)<=2&&Math.abs(green-188)<=2&&blue>=253&&(pixels.get(0)&255)<=1&&(pixels.get(1)&255)<=1&&(pixels.get(2)&255)>=253;
                Path directory=Path.of("verification");Files.createDirectories(directory);Files.writeString(directory.resolve("ui-color-1.16.1.json"),"{\"inside\":["+red+","+green+","+blue+"],\"expected\":[188,188,255],\"region_preserved\":"+valid+"}");
                if(!valid)throw new IllegalStateException("Linear UI resolve or dirty-region bounds are incorrect: "+red+","+green+","+blue);
                Toolscreen.LOGGER.info("TOOLSCREEN_UI_COLOR_PASS backend=OpenGL-1.16.1 inside={},{},{} region=true",red,green,blue);
            }finally{target.close();}
        }
    }
    public static void present(RenderTarget source,int width,int height){if(dev.riye.toolscreen.BuildVariant.VERIFICATION)LegacyVisualBenchmark.begin();if(dev.riye.toolscreen.BuildVariant.VERIFICATION)LegacyVisualProbe.overlayData();try(State state=new State()){initialize();LegacyAssets.update();boolean suspended=Toolscreen.suspended(),studio=Minecraft.getInstance().screen instanceof ToolscreenScreen;if(studio){((ToolscreenScreen)Minecraft.getInstance().screen).frame();initializeFont();}
        compose(DISPLAY,source.colorTextureId,source.width,source.height,width,height,0,suspended);if(!suspended&&!studio)renderUi(NativeBridge.overlayFrameForOutput(width,height,source.width,source.height,0),DISPLAY.id,width,height,source.colorTextureId,source.width,source.height);int[] display=displaySize();blit(DISPLAY.texture,0,display[0],display[1]);if(studio)renderUi(uiPacket,0,display[0],display[1],source.colorTextureId,source.width,source.height);if(dev.riye.toolscreen.BuildVariant.VERIFICATION)LegacyVisualBenchmark.end();if(dev.riye.toolscreen.BuildVariant.VERIFICATION)LegacyVisualProbe.observe();if(dev.riye.toolscreen.BuildVariant.VERIFICATION)LegacyHarness.observe();}}
    private static ByteBuffer read(int framebuffer,int width,int height){int bytes=width*height*4;if(readback==null||readback.capacity()<bytes){if(readback!=null)LegacyMemory.free(readback);readback=MemoryUtil.memAlloc(bytes);}bind(framebuffer);GL11.glReadBuffer(framebuffer==0?GL11.GL_BACK:36064);GL11.glPixelStorei(GL11.GL_PACK_ALIGNMENT,1);GL11.glPixelStorei(GL12.GL_PACK_ROW_LENGTH,0);GL11.glPixelStorei(GL12.GL_PACK_SKIP_ROWS,0);GL11.glPixelStorei(GL12.GL_PACK_SKIP_PIXELS,0);readback.clear().limit(bytes);GL11.glReadPixels(0,0,width,height,GL11.GL_RGBA,GL11.GL_UNSIGNED_BYTE,readback);return readback;}
    public static void visualSnapshot()throws Exception{Path folder=Path.of("verification");Files.createDirectories(folder);int[] size=displaySize();writePixels(read(0,size[0],size[1]),size[0],size[1],true,folder.resolve("visual-window.png"));GL11.glBindTexture(GL11.GL_TEXTURE_2D,fontTexture);int width=GL11.glGetTexLevelParameteri(GL11.GL_TEXTURE_2D,0,GL11.GL_TEXTURE_WIDTH),height=GL11.glGetTexLevelParameteri(GL11.GL_TEXTURE_2D,0,GL11.GL_TEXTURE_HEIGHT);ByteBuffer pixels=MemoryUtil.memAlloc(width*height*4);try{GL11.glGetTexImage(GL11.GL_TEXTURE_2D,0,GL11.GL_RGBA,GL11.GL_UNSIGNED_BYTE,pixels);writePixels(pixels,width,height,false,folder.resolve("visual-gpu-atlas.png"));}finally{LegacyMemory.free(pixels);}byte[] atlas=NativeBridge.fontAtlas();ByteBuffer data=ByteBuffer.wrap(atlas).order(ByteOrder.nativeOrder());int cpuWidth=data.getInt(),cpuHeight=data.getInt();writePixels(data.slice(),cpuWidth,cpuHeight,false,folder.resolve("visual-cpu-atlas.png"));Toolscreen.LOGGER.info("TOOLSCREEN_VISUAL_INFO framebuffer={}x{} window={}x{} atlas={}x{} minFilter={} magFilter={} renderer={}",size[0],size[1],Minecraft.getInstance().getWindow().getScreenWidth(),Minecraft.getInstance().getWindow().getScreenHeight(),width,height,GL11.glGetTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MIN_FILTER),GL11.glGetTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MAG_FILTER),GL11.glGetString(GL11.GL_RENDERER));}
    private static void writePixels(ByteBuffer pixels,int width,int height,boolean flipped,Path path)throws Exception{BufferedImage image=new BufferedImage(width,height,BufferedImage.TYPE_INT_ARGB);for(int y=0;y<height;y++)for(int x=0;x<width;x++){int i=((flipped?height-1-y:y)*width+x)*4;image.setRGB(x,y,((pixels.get(i+3)&255)<<24)|((pixels.get(i)&255)<<16)|((pixels.get(i+1)&255)<<8)|(pixels.get(i+2)&255));}ImageIO.write(image,"PNG",path.toFile());}
    public static void verifyReadback(){var window=Minecraft.getInstance().getWindow();while(GL11.glGetError()!=0){}read(0,window.getWidth(),window.getHeight());int error=GL11.glGetError();Toolscreen.LOGGER.info("TOOLSCREEN_DISPLAY_READBACK framebuffer={} dimensions={}x{} backbufferError={}",DISPLAY.id,DISPLAY.width,DISPLAY.height,error);}
    public static void screenshot(Path path)throws Exception{var window=Minecraft.getInstance().getWindow();int width=window.getWidth(),height=window.getHeight();var source=Minecraft.getInstance().getMainRenderTarget();if(Minecraft.getInstance().screen instanceof ToolscreenScreen)renderUi(uiPacket,DISPLAY.id,width,height,source.colorTextureId,source.width,source.height);ByteBuffer pixels=read(DISPLAY.id,width,height);BufferedImage image=new BufferedImage(width,height,BufferedImage.TYPE_INT_ARGB);for(int y=0;y<height;y++)for(int x=0;x<width;x++){int offset=((height-1-y)*width+x)*4;int rgba=((pixels.get(offset+3)&255)<<24)|((pixels.get(offset)&255)<<16)|((pixels.get(offset+1)&255)<<8)|(pixels.get(offset+2)&255);image.setRGB(x,y,rgba);}Files.createDirectories(path.getParent());ImageIO.write(image,"PNG",path.toFile());}
    public static void freeze(){if(!Toolscreen.ready)return;try(State state=new State()){initialize();RenderTarget source=Minecraft.getInstance().getMainRenderTarget();FROZEN.ensure(source.width,source.height);bind(source.frameBufferId);GL11.glReadBuffer(36064);GL11.glBindTexture(GL11.GL_TEXTURE_2D,FROZEN.texture);GL11.glCopyTexSubImage2D(GL11.GL_TEXTURE_2D,0,0,0,0,0,source.width,source.height);frozen=true;}}
    public static void releasePreview(){frozen=false;}
    public static void close(){DISPLAY.close();PREVIEW.close();FROZEN.close();UI_SCENE.close();UI_LINEAR.close();LegacyAssets.close();if(vertexBytes!=null)LegacyMemory.free(vertexBytes);if(readback!=null)LegacyMemory.free(readback);if(fontTexture!=0)GL11.glDeleteTextures(fontTexture);if(vertexBuffer!=0)GL15.glDeleteBuffers(vertexBuffer);if(uiProgram!=0)GL20.glDeleteProgram(uiProgram);if(uiResolveProgram!=0)GL20.glDeleteProgram(uiResolveProgram);if(compositeProgram!=0)GL20.glDeleteProgram(compositeProgram);}
}
