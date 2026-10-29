package dev.riye.toolscreen;

import com.mojang.blaze3d.pipeline.RenderTarget;
import com.mojang.blaze3d.platform.NativeImage;
import com.mojang.renderpearl.api.textures.GpuTextureView;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.client.Minecraft;
import net.minecraft.client.Screenshot;
import net.minecraft.client.gui.components.debug.DebugScreenEntries;
import net.minecraft.client.gui.components.debug.DebugScreenEntryStatus;
import net.minecraft.client.gui.screens.ChatScreen;
import net.minecraft.world.Difficulty;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.LevelSettings;
import net.minecraft.world.level.WorldDataConfiguration;
import net.minecraft.world.level.levelgen.WorldOptions;
import net.minecraft.world.level.levelgen.presets.WorldPresets;

public final class WorldHarness {
    private static boolean started;
    private static boolean colorRequested;
    private static boolean fixtureQueued;
    private static volatile boolean fixtureReady;
    private static boolean captureRequested;
    private static volatile boolean captureComplete;
    private static int stage;
    private static long deadline;
    private static long nextCapture;
    private static int idleFrames;
    private static int browserSource=-1;
    private static int imageSource=-1;
    private static long sourceDeadline;
    private static boolean windowCaptureActive;
    private static boolean windowCaptureComplete;
    private static final int[] MODES={0,1,2,3,4,1,1,3,3,3,3,3,3,3,3,3,3,0,1,3,3,0,3,3};
    private static final String[] NAMES={"fullscreen","thin","wide","eyezoom","preemptive","gradient","background","eyezoom-final","studio","mirrors","sources","bindings","appearance","diagnostics","ninjabrain","output","obs-preview","f3-hidden","restored","eyezoom-live","measurement-pause","measurement-resumed","pause-menu","chat-context"};

    private WorldHarness(){}

    private static final class CaptureTarget extends RenderTarget {
        CaptureTarget(GpuTextureView texture){super("Macless world test",texture.texture().getFormat(),null);colorTexture=texture.texture();colorTextureView=texture;width=colorTexture.getWidth(0);height=colorTexture.getHeight(0);}
    }

    public static void tick(){
        if(!Boolean.getBoolean("toolscreen.world"))return;
        Minecraft game=Minecraft.getInstance();
        if(!Toolscreen.ready||Toolscreen.failed){if(++idleFrames>120){Toolscreen.LOGGER.error("TOOLSCREEN_WORLD_FAIL {}",Toolscreen.error);game.stop();}return;}
        if(!started&&game.gui.overlay()==null&&++idleFrames>30){
            started=true;deadline=System.nanoTime()+(Boolean.getBoolean("toolscreen.visual")?1_800_000_000_000L:240_000_000_000L);
            NativeBridge.reset();game.options.renderDistance().set(4);game.options.fov().set(30);game.options.pauseOnLostFocus=false;
            if(Boolean.getBoolean("toolscreen.features"))RuntimeVerification.start();
            if(Boolean.getBoolean("toolscreen.sources")&&!Boolean.getBoolean("toolscreen.features")){boolean available=NativeBridge.captureAvailable();Toolscreen.LOGGER.info("TOOLSCREEN_WINDOW_CAPTURE_PERMISSION available={}",available);prepareSources();if(available){NativeBridge.startSource(9999,"window:self");windowCaptureActive=true;}}
            LevelSettings settings=new LevelSettings("Macless verification",GameType.CREATIVE,new LevelSettings.DifficultySettings(Difficulty.PEACEFUL,false,true),true,WorldDataConfiguration.DEFAULT);
            String name="toolscreen-verification-"+System.currentTimeMillis();
            game.createWorldOpenFlows().createFreshLevel(name,settings,new WorldOptions(26,false,false),WorldPresets::createTestWorldDimensions,game.gui.screen());
        }
        if(!started)return;
        if(System.nanoTime()>deadline){Toolscreen.LOGGER.error("TOOLSCREEN_WORLD_FAIL timeout");game.stop();return;}
        if(game.level!=null&&game.player!=null&&game.gui.screen()==null&&!fixtureQueued){
            fixtureQueued=true;
            if(Boolean.getBoolean("toolscreen.features")){game.debugEntries.setOverlayVisible(true);game.debugEntries.setStatus(DebugScreenEntries.ENTITY_RENDER_STATS,DebugScreenEntryStatus.IN_OVERLAY);if(!game.getDebugOverlay().showProfilerChart())game.getDebugOverlay().toggleProfilerChart();verifyInputContexts(game);ControlsProbe.camera();}
            var server=game.getSingleplayerServer();
            server.execute(()->{
                String[] commands={"fill -12 80 -12 12 80 12 minecraft:smooth_stone","tp @a 0.5 81 0.5 0 0","time set day","weather clear","setblock 2 81 4 minecraft:spawner","summon minecraft:item_display 0.5 82.62 5.5 {item:{id:\"minecraft:ender_eye\",count:1},billboard:\"center\"}"};
                for(String command:commands)server.getCommands().performPrefixedCommand(server.createCommandSourceStack(),command);
                fixtureReady=true;
            });
            nextCapture=System.nanoTime()+5_000_000_000L;
        }
        if(fixtureReady&&Boolean.getBoolean("toolscreen.visual")){ModernVisualProbe.tick();return;}
        if(fixtureReady&&!colorRequested){colorRequested=true;RenderColorProbe.run();}
        if(fixtureReady&&game.player!=null){game.player.setYRot(0);game.player.setXRot(0);game.player.yRotO=0;game.player.xRotO=0;}
        if(captureComplete){
            if(stage==0&&windowCaptureActive){NativeBridge.stopSource(9999);windowCaptureActive=false;}
            captureComplete=false;captureRequested=false;stage++;
            if(stage>=MODES.length){Toolscreen.LOGGER.info("TOOLSCREEN_WORLD_PASS backend={}",Toolscreen.backend());NativeBridge.reset();VerificationFixtures.close();game.stop();return;}
            NativeBridge.set(0,MODES[stage]);
            if(stage==20)ControlsProbe.beginPause();if(stage==21)ControlsProbe.endPause();if(stage==22)game.pauseGame(false);if(stage==23)game.gui.setScreen(new ChatScreen("",false));
            if(stage==5){NativeBridge.set(124,1);NativeBridge.set(125,.23);NativeBridge.set(126,.11);NativeBridge.set(127,.38);}
            if(stage==6){NativeBridge.set(124,2);NativeBridge.set(121,imageSource);NativeBridge.editAsset(imageSource,14,65535);}
            if(stage==7){NativeBridge.set(124,0);NativeBridge.set(121,0);}
            if(stage==8){NativeBridge.set(124,1);game.gui.setScreen(new ToolscreenScreen(null));}
            if(stage==17){game.gui.setScreen(null);game.debugEntries.setStatus(DebugScreenEntries.ENTITY_RENDER_STATS,DebugScreenEntryStatus.NEVER);}
            if(stage==18)game.debugEntries.setStatus(DebugScreenEntries.ENTITY_RENDER_STATS,DebugScreenEntryStatus.IN_OVERLAY);
            if(stage>=9&&stage<=16){int key=stage<=13?'2'+stage-9:stage==14?'7':stage==15?'8':'1';NativeBridge.uiKey(key,192,true);NativeBridge.uiKey(key,0,false);if(stage==16){NativeBridge.uiKey('9',192,true);NativeBridge.uiKey('9',0,false);}}
            nextCapture=System.nanoTime()+2_000_000_000L;
        }
    }

    public static void observe(GpuTextureView frame){
        if(Boolean.getBoolean("toolscreen.visual")){ModernVisualProbe.observe(frame);return;}
        if(!Boolean.getBoolean("toolscreen.world")||!fixtureReady||captureRequested||System.nanoTime()<nextCapture||stage>=MODES.length)return;
        Minecraft game=Minecraft.getInstance();
        if(game.gui.overlay()!=null||game.player==null)return;
        if(stage==0&&Boolean.getBoolean("toolscreen.features")&&!RuntimeVerification.ready(game.getWindow().getWidth(),game.getWindow().getHeight()))return;
        if(stage==0&&browserSource>=0){int[] info=NativeBridge.sourceInfo(browserSource);if(info[2]<3||AssetManager.texture(browserSource)==null){if(sourceDeadline==0)sourceDeadline=System.nanoTime()+25_000_000_000L;if(System.nanoTime()<sourceDeadline)return;Toolscreen.LOGGER.error("TOOLSCREEN_BROWSER_FAIL {}",NativeBridge.sourceError(browserSource));}}
        if(stage==0&&windowCaptureActive&&!windowCaptureComplete){
            int[] info=NativeBridge.sourceInfo(9999);
            if(info[0]>0&&info[1]>0&&info[2]>3){
                try(NativeImage capture=new NativeImage(info[0],info[1],false)){
                    int sequence=NativeBridge.readSource(9999,capture.getPixelBytes(),0);
                    if(sequence>0){var pixels=capture.getPixelBytes();int red=0,blue=0;for(int offset=0;offset<pixels.capacity();offset+=16){int r=pixels.get(offset)&255,g=pixels.get(offset+1)&255,b=pixels.get(offset+2)&255;if(r>220&&g>40&&g<90&&b<90)red++;if(b>220&&r<90&&g>100&&g<150)blue++;}if(red>1000&&blue>1000){Path directory=Path.of("verification");Files.createDirectories(directory);capture.writeToFile(directory.resolve("window-capture-"+Toolscreen.backend().toLowerCase()+".png"));Toolscreen.LOGGER.info("TOOLSCREEN_WINDOW_CAPTURE_PASS size={}x{} frames={}",info[0],info[1],sequence);windowCaptureComplete=true;}}
                }catch(Exception failure){Toolscreen.LOGGER.error("TOOLSCREEN_WINDOW_CAPTURE_FAIL",failure);windowCaptureComplete=true;}
            }
            if(!windowCaptureComplete){if(sourceDeadline==0)sourceDeadline=System.nanoTime()+25_000_000_000L;if(System.nanoTime()<sourceDeadline)return;Toolscreen.LOGGER.error("TOOLSCREEN_WINDOW_CAPTURE_FAIL {}",NativeBridge.sourceError(9999));windowCaptureComplete=true;}
        }
        ControlsProbe.verifyPause(stage);captureRequested=true;String name=NAMES[stage];String backend=Toolscreen.backend().toLowerCase().replaceAll("[^a-z0-9]+","-");
        var source=game.gameRenderer.mainRenderTarget();
        Toolscreen.LOGGER.info("TOOLSCREEN_WORLD_CAPTURE {} source={}x{} output={}x{}",name,source.width,source.height,frame.texture().getWidth(0),frame.texture().getHeight(0));
        if(stage==3){float[] plan=NativeBridge.plan(game.getWindow().getWidth(),game.getWindow().getHeight(),source.width,source.height,false,0);float right=(plan[24]+plan[26])*game.getWindow().getWidth();float gameLeft=plan[4]*game.getWindow().getWidth();if(right>gameLeft)throw new IllegalStateException("EyeZoom projector overlaps the game viewport");}
        Screenshot.takeScreenshot(new CaptureTarget(frame),image->{
            try(image){Path directory=Path.of("verification");Files.createDirectories(directory);image.writeToFile(directory.resolve("world-"+name+"-"+backend+".png"));if(name.equals("fullscreen")&&browserSource>=0){int top=image.getPixel((int)(image.getWidth()*.67),(int)(image.getHeight()*.13));int bottom=image.getPixel((int)(image.getWidth()*.67),(int)(image.getHeight()*.33));int greenTop=(top>>>8)&255,greenBottom=(bottom>>>8)&255;boolean valid=greenTop>40&&greenTop<90&&greenBottom>100&&greenBottom<160;Toolscreen.LOGGER.info("TOOLSCREEN_BROWSER_{} top={} bottom={} frames={}",valid?"PASS":"FAIL",Integer.toHexString(top),Integer.toHexString(bottom),NativeBridge.sourceInfo(browserSource)[2]);validateColorFilter(image);}if(!Boolean.getBoolean("toolscreen.features")&&(name.equals("gradient")||name.equals("background")))validateBackground(image,name);if(Boolean.getBoolean("toolscreen.features"))RuntimeVerification.inspect(RuntimeVerification.rgba(image.getPixelBytes(),image.getWidth(),image.getHeight()),name,backend,stage>=8&&stage<=16);if(Boolean.getBoolean("toolscreen.features")&&(stage==17||stage==18)){boolean visible=NativeBridge.debugProbe()[0]==1;if(visible!=(stage==18))throw new IllegalStateException("F3 counter visibility is stale");Toolscreen.LOGGER.info("TOOLSCREEN_F3_VISIBILITY_PASS visible={}",visible);}}
            catch(Exception failure){Toolscreen.LOGGER.error("TOOLSCREEN_WORLD_FAIL screenshot",failure);deadline=0;}
            finally{captureComplete=true;}
        });
    }

    private static void verifyInputContexts(Minecraft game){
        NativeBridge.releaseHeldInput();NativeBridge.set(0,0);game.gui.setScreen(new ChatScreen("",false));boolean chat=Toolscreen.hotkey(66,0,1);Toolscreen.hotkey(66,0,0);boolean valid=!chat&&NativeBridge.snapshot()[0]==0;game.gui.setScreen(null);NativeBridge.set(128,0);boolean disabled=Toolscreen.hotkey(66,0,1);Toolscreen.hotkey(66,0,0);valid=valid&&!disabled&&NativeBridge.snapshot()[0]==0;NativeBridge.set(128,1);NativeBridge.releaseHeldInput();Toolscreen.LOGGER.info("TOOLSCREEN_INPUT_CONTEXT_{}",valid?"PASS":"FAIL");if(!valid)throw new IllegalStateException("Gameplay bindings fired in chat or while disabled");
    }

    private static void validateBackground(NativeImage image,String name){
        int top=image.getPixel((int)(image.getWidth()*.85),(int)(image.getHeight()*.25));
        int bottom=image.getPixel((int)(image.getWidth()*.85),(int)(image.getHeight()*.75));
        int topGreen=(top>>>8)&255,bottomGreen=(bottom>>>8)&255;
        boolean valid=name.equals("gradient")?(bottom&255)>(top&255)&&((bottom>>>16)&255)>((top>>>16)&255):topGreen>195&&bottomGreen>160&&bottomGreen<195;
        Toolscreen.LOGGER.info("TOOLSCREEN_{}_{} top={} bottom={}",name.toUpperCase(),valid?"PASS":"FAIL",Integer.toHexString(top),Integer.toHexString(bottom));
    }

    private static void validateColorFilter(NativeImage image){
        int top=image.getPixel((int)(image.getWidth()*.1),(int)(image.getHeight()*.08));
        int bottom=image.getPixel((int)(image.getWidth()*.1),(int)(image.getHeight()*.2));
        boolean valid=(top&0xffffff)==0xffffff&&(bottom&0xffffff)!=0xf0b670&&(bottom&0xffffff)!=0xffffff;
        Toolscreen.LOGGER.info("TOOLSCREEN_COLOR_FILTER_{} top={} bottom={}",valid?"PASS":"FAIL",Integer.toHexString(top),Integer.toHexString(bottom));
    }

    private static void prepareSources(){
        try{
            Path directory=Path.of("verification").toAbsolutePath();Files.createDirectories(directory);
            Path html=directory.resolve("browser-fixture.html");
            Files.writeString(html,"<!doctype html><html><head><style>html,body{margin:0;width:100%;height:100%;overflow:hidden}header{position:absolute;inset:0 0 50%;background:#ff4038}footer{position:absolute;inset:50% 0 0;background:#3879ff}p{position:absolute;inset:35% 0 auto;text-align:center;color:white;font:700 42px Arial}</style></head><body><header></header><footer></footer><p>Browser overlay <span id='time'>0</span></p><script>setInterval(()=>document.getElementById('time').textContent=String(Date.now()%10000),100)</script></body></html>");
            browserSource=NativeBridge.addAsset("browser:"+html.toUri());NativeBridge.editAsset(browserSource,14,1);
            Path png=directory.resolve("image-fixture.png");
            try(NativeImage image=new NativeImage(240,100,false)){for(int y=0;y<100;y++)for(int x=0;x<240;x++)image.setPixel(x,y,y<50?0xFF50D4A4:0xFFF0B670);image.writeToFile(png);}
            imageSource=NativeBridge.addAsset(png.toString());NativeBridge.editAsset(imageSource,5,.05);NativeBridge.editAsset(imageSource,6,.04);NativeBridge.editAsset(imageSource,14,1);
            NativeBridge.editAsset(imageSource,26,1);NativeBridge.editAsset(imageSource,27,80.0/255);NativeBridge.editAsset(imageSource,28,212.0/255);NativeBridge.editAsset(imageSource,29,164.0/255);NativeBridge.editAsset(imageSource,30,1);NativeBridge.editAsset(imageSource,11,.01);
        }catch(Exception failure){throw new IllegalStateException(failure);}
    }
}
