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
    public static boolean resizingSuspended(){return !ready||failed||!NativeBridge.enabled();}
    public static boolean resizeShortcutsAllowed(){
        Minecraft game=Minecraft.getInstance();var screen=game.screen;
        if(!game.isWindowActive()||screen instanceof ToolscreenScreen)return false;
        if(screen==null)return game.level!=null;
        if(screen instanceof net.minecraft.client.gui.screens.ChatScreen||screen instanceof net.minecraft.client.gui.screens.inventory.BookEditScreen||screen instanceof net.minecraft.client.gui.screens.inventory.SignEditScreen)return false;
        net.minecraft.client.gui.components.events.GuiEventListener focused=screen;
        for(int depth=0;focused!=null&&depth<32;depth++){
            if(focused instanceof net.minecraft.client.gui.components.EditBox)return false;
            if(focused instanceof net.minecraft.client.gui.components.events.ContainerEventHandler container)focused=container.getFocused();else break;
        }
        return true;
    }
    public static MenuCoordinates menuCoordinates(){
        var game=Minecraft.getInstance();var screen=game.screen;var window=game.getWindow();
        if(resizingSuspended()||screen==null||screen instanceof ToolscreenScreen||window.getWidth()<1||window.getHeight()<1)return null;
        int[] source=viewport();return new MenuCoordinates(window.getWidth(),window.getHeight(),source[0],source[1],window.getGuiScaledWidth(),window.getGuiScaledHeight());
    }
    public static void updateMenuLayout(){
        if(!ready)return;var game=Minecraft.getInstance();var screen=game.screen;
        if(screen==null||screen instanceof ToolscreenScreen)return;
        int width=game.getWindow().getGuiScaledWidth(),height=game.getWindow().getGuiScaledHeight();
        if(screen.width!=width||screen.height!=height)screen.resize(game,width,height);
    }
    public static int[] viewport(){var game=Minecraft.getInstance();NativeBridge.runtimeContext(game.level!=null,game.options.sensitivity);var window=game.getWindow();return NativeBridge.viewport(window.getWidth(),window.getHeight(),maximumTextureSize,resizingSuspended());}
    public static boolean hotkey(int scan,int modifiers,int action){Minecraft game=Minecraft.getInstance();if(!ready||measurementPaused())return false;if(game.screen instanceof ToolscreenScreen&&(NativeBridge.uiBindingActive()||NativeBridge.uiWantsText())){NativeBridge.releaseHeldInput();return false;}boolean gameplay=resizeShortcutsAllowed();boolean editor=gameplay||game.screen==null||game.screen instanceof ToolscreenScreen;int result=NativeBridge.inputEvent(scan,modifiers,action,gameplay,editor);if(result==1){if(game.screen instanceof ToolscreenScreen)game.screen.onClose();else game.setScreen(new ToolscreenScreen(game.screen));return true;}if(result==2){NativeBridge.save();return true;}if(result==3)return true;if(result==4)NativeBridge.save();return false;}
    public static void fail(Throwable failure){if(!failed){failed=true;error=failure.toString();LOGGER.error("Macless renderer failed",failure);}}
}
