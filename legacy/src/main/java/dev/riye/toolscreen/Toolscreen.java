package dev.riye.toolscreen;

import net.fabricmc.api.ClientModInitializer;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screens.TitleScreen;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public final class Toolscreen implements ClientModInitializer {
    public static final Logger LOGGER=LogManager.getLogger("toolscreen-native");
    public static boolean ready;
    public static boolean failed;
    public static String error="";
    public static int maximumTextureSize=8192;
    private static boolean measuring;
    @Override public void onInitializeClient(){try{NativeBridge.load();GameplaySettings.initialize();NativeBridge.setOption("minecraft_version","1.16.1");NinjabrainClient.start();ready=true;LOGGER.info("Macless native core loaded for Minecraft 1.16.1");}catch(Throwable failure){failed=true;error=failure.toString();LOGGER.error("Macless could not start",failure);}}
    public static boolean measurementPaused(){Minecraft game=Minecraft.getInstance();return game.screen instanceof net.minecraft.client.gui.screens.PauseScreen pause&&!((dev.riye.toolscreen.mixin.LegacyPauseAccess)pause).toolscreenShowsPauseMenu();}
    public static void updateMeasurementPause(){boolean paused=measurementPaused();if(measuring&&!paused)NativeBridge.releaseHeldInput();measuring=paused;}
    public static boolean suspended(){Minecraft game=Minecraft.getInstance();return !ready||failed||game.level==null||(game.screen!=null&&!(game.screen instanceof ToolscreenScreen)&&!measurementPaused())||!NativeBridge.enabled();}
    public static int[] viewport(){var game=Minecraft.getInstance();NativeBridge.runtimeContext(game.level!=null,game.options.sensitivity);var window=game.getWindow();return NativeBridge.viewport(window.getWidth(),window.getHeight(),maximumTextureSize,suspended());}
    public static boolean hotkey(int scan,int modifiers,int action){Minecraft game=Minecraft.getInstance();if(!ready||measurementPaused())return false;if(game.screen instanceof ToolscreenScreen&&(NativeBridge.uiBindingActive()||NativeBridge.uiWantsText())){NativeBridge.releaseHeldInput();return false;}boolean gameplay=game.level!=null&&game.screen==null&&game.isWindowActive();boolean editor=game.screen==null||game.screen instanceof ToolscreenScreen||game.screen instanceof TitleScreen;int result=NativeBridge.inputEvent(scan,modifiers,action,gameplay,editor);if(result==1){if(game.screen instanceof ToolscreenScreen)game.screen.onClose();else game.setScreen(new ToolscreenScreen(game.screen));return true;}if(result==2){NativeBridge.save();return true;}if(result==3)return true;if(result==4)NativeBridge.save();return false;}
    public static void fail(Throwable failure){if(!failed){failed=true;error=failure.toString();LOGGER.error("Macless renderer failed",failure);}}
}
