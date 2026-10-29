package dev.riye.toolscreen;

import java.awt.image.BufferedImage;
import java.nio.ByteBuffer;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import javax.imageio.ImageIO;
import net.minecraft.client.Minecraft;
import org.lwjgl.opengl.GL11;
import org.lwjgl.system.MemoryUtil;

public final class LegacyAssets {
    private static final ExecutorService IO=Executors.newSingleThreadExecutor(task->{Thread thread=new Thread(task,"Macless image loader");thread.setDaemon(true);return thread;});
    private static final Map<Integer,Asset> assets=new HashMap<>();
    private static final class Asset{
        final int id;
        final String path;
        final boolean remote;
        int texture,width,height,sequence;
        ByteBuffer pixels;
        BufferedImage pending;
        volatile String error;
        boolean canceled;
        String status="";
        Asset(int id,String path){this.id=id;this.path=path;remote=path.startsWith("browser:")||path.startsWith("window:");}
        synchronized void accept(BufferedImage image){if(!canceled)pending=image;}
        synchronized BufferedImage take(){BufferedImage image=pending;pending=null;return image;}
        synchronized void close(){canceled=true;if(texture!=0)GL11.glDeleteTextures(texture);if(pixels!=null)LegacyMemory.free(pixels);if(remote)NativeBridge.stopSource(id);}
    }
    private LegacyAssets(){}
    public static void update(){
        String[] entries=NativeBridge.assetEntries();HashSet<Integer> active=new HashSet<>();
        for(int i=0;i<entries.length;i+=2){int id=Integer.parseInt(entries[i]);String path=entries[i+1];active.add(id);Asset asset=assets.get(id);if(asset==null||!asset.path.equals(path)){if(asset!=null)asset.close();Asset created=new Asset(id,path);assets.put(id,created);asset=created;if(created.remote)NativeBridge.startSource(id,path);else IO.submit(()->{try{BufferedImage image=ImageIO.read(Path.of(path).toFile());if(image==null)throw new IllegalArgumentException("Unsupported image");if((long)image.getWidth()*image.getHeight()>16777216)throw new IllegalArgumentException("Image exceeds texture limits");created.accept(image);}catch(Exception failure){created.error=failure.toString();}});}
            try{if(asset.remote){int[] info=NativeBridge.sourceInfo(id);String error=NativeBridge.sourceError(id);if(!error.isEmpty())status(asset,0,0,error);if(info[0]>0&&info[1]>0&&info[2]!=asset.sequence){ensure(asset,info[0],info[1]);int sequence=NativeBridge.readSource(id,asset.pixels,asset.sequence);if(sequence>0){asset.sequence=sequence;upload(asset);}}}else{BufferedImage image=asset.take();if(image!=null){ensure(asset,image.getWidth(),image.getHeight());asset.pixels.clear();for(int y=0;y<asset.height;y++)for(int x=0;x<asset.width;x++){int color=image.getRGB(x,y);asset.pixels.put((byte)(color>>16)).put((byte)(color>>8)).put((byte)color).put((byte)(color>>24));}asset.pixels.flip();upload(asset);}if(asset.error!=null)status(asset,0,0,asset.error);}}catch(Throwable failure){status(asset,0,0,failure.toString());}
        }
        var iterator=assets.entrySet().iterator();while(iterator.hasNext()){var value=iterator.next();if(!active.contains(value.getKey())){value.getValue().close();iterator.remove();}}
    }
    private static void ensure(Asset asset,int width,int height){if(width<1||height<1||width>Toolscreen.maximumTextureSize||height>Toolscreen.maximumTextureSize||(long)width*height>16777216)throw new IllegalArgumentException("Source exceeds texture limits");if(asset.width==width&&asset.height==height)return;if(asset.pixels!=null)LegacyMemory.free(asset.pixels);asset.pixels=MemoryUtil.memAlloc(width*height*4);asset.width=width;asset.height=height;if(asset.texture==0)asset.texture=GL11.glGenTextures();GL11.glBindTexture(GL11.GL_TEXTURE_2D,asset.texture);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MIN_FILTER,GL11.GL_LINEAR);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_MAG_FILTER,GL11.GL_LINEAR);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_WRAP_S,33071);GL11.glTexParameteri(GL11.GL_TEXTURE_2D,GL11.GL_TEXTURE_WRAP_T,33071);GL11.glTexImage2D(GL11.GL_TEXTURE_2D,0,GL11.GL_RGBA8,width,height,0,GL11.GL_RGBA,GL11.GL_UNSIGNED_BYTE,(ByteBuffer)null);}
    private static void upload(Asset asset){asset.pixels.position(0).limit(asset.width*asset.height*4);GL11.glBindTexture(GL11.GL_TEXTURE_2D,asset.texture);GL11.glTexSubImage2D(GL11.GL_TEXTURE_2D,0,0,0,asset.width,asset.height,GL11.GL_RGBA,GL11.GL_UNSIGNED_BYTE,asset.pixels);status(asset,asset.width,asset.height,"");}
    private static void status(Asset asset,int width,int height,String error){String state=width+"x"+height+":"+error;if(!state.equals(asset.status)){asset.status=state;var window=Minecraft.getInstance().getWindow();NativeBridge.assetStatus(asset.id,width,height,error,window.getWidth(),window.getHeight());}}
    public static int texture(int id){Asset asset=assets.get(id);return asset==null?0:asset.texture;}
    public static void close(){for(Asset asset:assets.values())asset.close();assets.clear();IO.shutdownNow();}
}
