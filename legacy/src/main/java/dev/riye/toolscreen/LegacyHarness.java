package dev.riye.toolscreen;

import dev.riye.toolscreen.mixin.LegacyProfilerAccess;
import java.nio.ByteBuffer;
import java.nio.file.Path;
import java.util.ArrayDeque;
import java.util.HashSet;
import java.util.List;
import java.util.Properties;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screens.TitleScreen;
import net.minecraft.core.RegistryAccess;
import net.minecraft.util.profiling.ProfileResults;
import net.minecraft.util.profiling.ResultField;
import net.minecraft.world.Difficulty;
import net.minecraft.world.level.DataPackConfig;
import net.minecraft.world.level.GameRules;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.LevelSettings;
import net.minecraft.world.level.levelgen.WorldGenSettings;
import org.lwjgl.BufferUtils;

public final class LegacyHarness {
    private static final int[] MODES={0,1,2,3,4,0,3,3,3,3,3,3,3,3,0,1,3,3,0,3,3};
    private static final String[] NAMES={"fullscreen","thin","wide","eyezoom","preemptive","mirrors","studio","mirror-inspector","sources","bindings","appearance","ninjabrain","output","diagnostics","f3-hidden","restored","eyezoom-live","measurement-pause","measurement-resumed","pause-menu","chat-context"};
    private static boolean started,fixtureQueued,pathFound,ninjaPassed,mirrorsPassed;
    private static volatile boolean fixtureReady;
    private static int idle,stage,closingFrames;
    private static long deadline,nextCapture,pathAttempt;
    private LegacyHarness(){}
    public static void tick(){
        if(!Boolean.getBoolean("toolscreen.world"))return;Minecraft game=Minecraft.getInstance();if(closingFrames>0){if(--closingFrames==0){VerificationFixtures.close();game.stop();}return;}
        if(Toolscreen.failed){Toolscreen.LOGGER.error("TOOLSCREEN_LEGACY_FAIL {}",Toolscreen.error);game.stop();return;}
        if(!started&&Toolscreen.ready&&game.screen instanceof TitleScreen&&++idle>30){started=true;deadline=System.nanoTime()+240_000_000_000L;NativeBridge.reset();NativeBridge.setOption("minecraft_version","1.16.1");RuntimeVerification.start();game.options.renderDistance=4;game.options.pauseOnLostFocus=false;game.options.fov=70;Properties properties=new Properties();properties.setProperty("level-type","flat");properties.setProperty("level-seed","16161");properties.setProperty("generate-structures","false");properties.setProperty("generator-settings","{\"biome\":\"minecraft:plains\",\"layers\":[{\"block\":\"minecraft:bedrock\",\"height\":1},{\"block\":\"minecraft:dirt\",\"height\":2},{\"block\":\"minecraft:grass_block\",\"height\":1}],\"structures\":{\"structures\":{}}}");game.createLevel("toolscreen-verification-"+System.currentTimeMillis(),new LevelSettings("Macless verification",GameType.CREATIVE,false,Difficulty.PEACEFUL,true,new GameRules(),DataPackConfig.DEFAULT),RegistryAccess.builtin(),WorldGenSettings.create(properties));}
        if(!started)return;if(System.nanoTime()>deadline){Toolscreen.LOGGER.error("TOOLSCREEN_LEGACY_FAIL timeout");game.stop();return;}
        if(game.level!=null&&game.player!=null&&!fixtureQueued){game.options.pauseOnLostFocus=false;if(game.screen instanceof net.minecraft.client.gui.screens.PauseScreen)game.setScreen(null);}
        if(game.level!=null&&game.player!=null&&game.screen==null&&!fixtureQueued){fixtureQueued=true;verifyInputContexts(game);LegacyControlsProbe.camera();game.options.renderDebug=true;game.options.renderDebugCharts=true;game.options.guiScale=1;game.getWindow().setGuiScale(1);game.getSingleplayerServer().execute(()->{var server=game.getSingleplayerServer();for(String command:new String[]{"tp @a 0.5 4 0.5 0 0","time set day","weather clear","setblock 2 4 4 minecraft:spawner","summon item 0.5 5.6 5 {Item:{id:\"minecraft:ender_eye\",Count:1b},NoGravity:1b,Age:-32768}"})server.getCommands().performCommand(server.createCommandSourceStack(),command);fixtureReady=true;});nextCapture=System.nanoTime()+6_000_000_000L;}
        if(fixtureReady&&game.player!=null){game.player.yRot=0;game.player.xRot=0;game.player.yRotO=0;game.player.xRotO=0;}
        if(fixtureReady&&!pathFound&&System.nanoTime()>pathAttempt){pathAttempt=System.nanoTime()+500_000_000L;ProfileResults results=((LegacyProfilerAccess)game).toolscreenResults();String path=findBlockParent(results);if(path!=null){((LegacyProfilerAccess)game).toolscreenSetPath(path);pathFound=true;Toolscreen.LOGGER.info("TOOLSCREEN_LEGACY_PROFILER_PATH {}",path.replace('\u001e','/'));}}
    }
    private static void verifyInputContexts(Minecraft game){NativeBridge.releaseHeldInput();NativeBridge.set(0,0);game.setScreen(new net.minecraft.client.gui.screens.ChatScreen(""));boolean chat=Toolscreen.hotkey(66,0,1);Toolscreen.hotkey(66,0,0);boolean valid=!chat&&NativeBridge.snapshot()[0]==0;game.setScreen(null);NativeBridge.set(128,0);boolean disabled=Toolscreen.hotkey(66,0,1);Toolscreen.hotkey(66,0,0);valid=valid&&!disabled&&NativeBridge.snapshot()[0]==0;NativeBridge.set(128,1);NativeBridge.releaseHeldInput();Toolscreen.LOGGER.info("TOOLSCREEN_INPUT_CONTEXT_{}",valid?"PASS":"FAIL");if(!valid)throw new IllegalStateException("Bindings fired in legacy chat or while disabled");}
    public static String findBlockParent(ProfileResults profile){if(profile==null)return null;ArrayDeque<String> queue=new ArrayDeque<>();HashSet<String> seen=new HashSet<>();queue.add("root");while(!queue.isEmpty()&&seen.size()<256){String path=queue.removeFirst();if(!seen.add(path)||path.chars().filter(value->value==30).count()>10)continue;List<ResultField> rows=profile.getTimes(path);for(int i=1;i<rows.size();i++){String name=rows.get(i).name;if(name.equalsIgnoreCase("blockentities")||name.equalsIgnoreCase("block_entities"))return path;if(!name.equals("unspecified")&&!name.isEmpty())queue.addLast(path+'\u001e'+name);}}return null;}
    public static void observe(){
        if(!Boolean.getBoolean("toolscreen.world")||!fixtureReady||stage>=MODES.length)return;Minecraft game=Minecraft.getInstance();double[] info=NativeBridge.debugProbe();
        if(!ninjaPassed&&info[5]==1&&info[4]==2&&info[6]==1604&&info[7]==-796){ninjaPassed=true;Toolscreen.LOGGER.info("TOOLSCREEN_LEGACY_NINJA_PASS x={} z={} certainty={}",info[6],info[7],info[8]);}
        if(!mirrorsPassed&&info[0]==1&&info[1]>0&&info[2]==1){mirrorsPassed=true;Toolscreen.LOGGER.info("TOOLSCREEN_LEGACY_MIRRORS_PASS eCounter={} rows={} blockEntities={}",info[0],info[1],info[3]);}
        if(System.nanoTime()<nextCapture)return;if(stage==0&&(!ninjaPassed||!mirrorsPassed||!RuntimeVerification.ready(game.getWindow().getWidth(),game.getWindow().getHeight()))){nextCapture=System.nanoTime()+500_000_000L;return;}
        try{if(stage==0){LegacyRenderer.verifyUiColor();LegacyRenderer.verifyReadback();}LegacyControlsProbe.verifyPause(stage);String name=NAMES[stage];var source=game.getMainRenderTarget();Path screenshot=Path.of("verification","world-"+name+"-1.16.1.png");LegacyRenderer.screenshot(screenshot);RuntimeVerification.inspect(javax.imageio.ImageIO.read(screenshot.toFile()),name,"1.16.1",stage>=6&&stage<=13);if(stage==14||stage==15){boolean visible=NativeBridge.debugProbe()[0]==1;if(visible!=(stage==15))throw new IllegalStateException("Legacy F3 counter visibility is stale");Toolscreen.LOGGER.info("TOOLSCREEN_F3_VISIBILITY_PASS visible={}",visible);}Toolscreen.LOGGER.info("TOOLSCREEN_LEGACY_CAPTURE {} source={}x{}",name,source.width,source.height);stage++;if(stage>=MODES.length){Toolscreen.LOGGER.info("TOOLSCREEN_LEGACY_PASS");game.options.renderDebugCharts=false;game.options.renderDebug=false;closingFrames=3;return;}NativeBridge.set(0,MODES[stage]);if(stage==17)LegacyControlsProbe.beginPause();if(stage==18)LegacyControlsProbe.endPause();if(stage==19)game.pauseGame(false);if(stage==20)game.setScreen(new net.minecraft.client.gui.screens.ChatScreen(""));if(stage==14){game.setScreen(null);game.options.renderDebug=false;game.options.renderDebugCharts=false;}if(stage==15){game.options.renderDebug=true;game.options.renderDebugCharts=true;}if(stage==6)game.setScreen(new ToolscreenScreen(null));if(stage>=7&&stage<=13){int key=stage==7?'2':stage==8?'3':stage==9?'4':stage==10?'5':stage==11?'7':stage==12?'8':'6';NativeBridge.uiKey(key,192,true);NativeBridge.uiKey(key,0,false);}nextCapture=System.nanoTime()+2_000_000_000L;}catch(Throwable failure){Toolscreen.LOGGER.error("TOOLSCREEN_LEGACY_FAIL capture",failure);stage=MODES.length;game.options.renderDebugCharts=false;game.options.renderDebug=false;closingFrames=3;}
    }
}
