package dev.riye.toolscreen;

import com.mojang.blaze3d.platform.NativeImage;
import com.mojang.blaze3d.systems.RenderSystem;
import com.mojang.renderpearl.api.GpuFormat;
import com.mojang.renderpearl.api.commands.CommandEncoder;
import com.mojang.renderpearl.api.textures.GpuTexture;
import com.mojang.renderpearl.api.textures.GpuTextureView;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;

public final class AssetManager {
    private static final ExecutorService IO=Executors.newSingleThreadExecutor(task->{Thread thread=new Thread(task,"Macless image loader");thread.setDaemon(true);return thread;});
    private static final Map<Integer,Asset> assets=new HashMap<>();
    private static final class Asset {
        final int id;
        final String path;
        final boolean remote;
        NativeImage pending;
        NativeImage live;
        volatile String error;
        boolean canceled;
        int sequence;
        String status="";
        GpuTexture texture;
        GpuTextureView view;
        Asset(int id,String path){this.id=id;this.path=path;remote=path.startsWith("window:")||path.startsWith("browser:");}
        synchronized void close(){canceled=true;if(remote)NativeBridge.stopSource(id);if(view!=null)view.close();if(texture!=null)texture.close();if(pending!=null)pending.close();if(live!=null)live.close();pending=null;live=null;}
        synchronized void accept(NativeImage image){if(canceled)image.close();else pending=image;}
        synchronized NativeImage take(){NativeImage result=pending;pending=null;return result;}
    }
    private AssetManager(){}

    public static void update(CommandEncoder encoder){
        String[] entries=NativeBridge.assetEntries();HashSet<Integer> active=new HashSet<>();
        for(int i=0;i<entries.length;i+=2){
            int id=Integer.parseInt(entries[i]);String path=entries[i+1];active.add(id);Asset asset=assets.get(id);
            if(asset==null||!asset.path.equals(path)){
                if(asset!=null)asset.close();Asset created=new Asset(id,path);assets.put(id,created);asset=created;
                if(created.remote)NativeBridge.startSource(id,path);
                else IO.submit(()->{try(var input=Files.newInputStream(Path.of(path))){NativeImage image=NativeImage.read(input);if(image.getWidth()>16384||image.getHeight()>16384){image.close();throw new IllegalArgumentException("Image exceeds texture limits");}created.accept(image);}catch(Exception failure){created.error=failure.toString();}});
            }
            try{
                if(asset.remote)updateRemote(encoder,asset);
                else{
                    NativeImage image=asset.take();
                    if(image!=null)try(image){upload(encoder,asset,image);}
                    if(asset.error!=null)publishStatus(asset,0,0,asset.error);
                }
            }catch(Throwable failure){asset.error=failure.toString();publishStatus(asset,0,0,asset.error);}
        }
        var iterator=assets.entrySet().iterator();while(iterator.hasNext()){var entry=iterator.next();if(!active.contains(entry.getKey())){entry.getValue().close();iterator.remove();}}
    }

    private static void updateRemote(CommandEncoder encoder,Asset asset){
        int[] info=NativeBridge.sourceInfo(asset.id);
        String error=NativeBridge.sourceError(asset.id);
        if(!error.isEmpty())publishStatus(asset,0,0,error);
        if(info[0]<=0||info[1]<=0||info[2]==asset.sequence)return;
        int maximum=RenderSystem.getDevice().getDeviceInfo().limits().maxTextureSize();
        if(info[0]>maximum||info[1]>maximum||(long)info[0]*info[1]>16777216)throw new IllegalArgumentException("Capture exceeds texture limits");
        if(asset.live==null||asset.live.getWidth()!=info[0]||asset.live.getHeight()!=info[1]){if(asset.live!=null)asset.live.close();asset.live=new NativeImage(info[0],info[1],false);}
        int sequence=NativeBridge.readSource(asset.id,asset.live.getPixelBytes(),asset.sequence);
        if(sequence>0){asset.sequence=sequence;upload(encoder,asset,asset.live);}
    }

    private static void upload(CommandEncoder encoder,Asset asset,NativeImage image){
        int maximum=RenderSystem.getDevice().getDeviceInfo().limits().maxTextureSize();
        if(image.getWidth()>maximum||image.getHeight()>maximum||(long)image.getWidth()*image.getHeight()>16777216)throw new IllegalArgumentException("Image exceeds texture limits");
        if(asset.texture==null||asset.texture.getWidth(0)!=image.getWidth()||asset.texture.getHeight(0)!=image.getHeight()){
            if(asset.view!=null)asset.view.close();if(asset.texture!=null)asset.texture.close();
            asset.texture=RenderSystem.getDevice().createTexture("Macless source "+asset.id,GpuTexture.USAGE_TEXTURE_BINDING|GpuTexture.USAGE_COPY_DST,GpuFormat.RGBA8_UNORM,image.getWidth(),image.getHeight(),1,1);
            asset.view=RenderSystem.getDevice().createTextureView(asset.texture);
        }
        encoder.writeToTexture(asset.texture,image);
        publishStatus(asset,image.getWidth(),image.getHeight(),"");
    }

    private static void publishStatus(Asset asset,int width,int height,String error){
        String status=width+"x"+height+":"+error;
        if(!asset.status.equals(status)){asset.status=status;var window=net.minecraft.client.Minecraft.getInstance().getWindow();NativeBridge.assetStatus(asset.id,width,height,error,window.getWidth(),window.getHeight());if(!error.isEmpty())Toolscreen.LOGGER.warn("Macless source {}: {}",asset.path,error);}
    }

    public static GpuTextureView texture(int id){Asset asset=assets.get(id);return asset==null?null:asset.view;}
    public static void close(){for(Asset asset:assets.values())asset.close();assets.clear();IO.shutdownNow();}
}
