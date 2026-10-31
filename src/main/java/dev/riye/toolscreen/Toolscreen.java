package dev.riye.toolscreen;

import com.mojang.blaze3d.platform.Window;
import com.mojang.blaze3d.systems.RenderSystem;
import net.fabricmc.api.ClientModInitializer;
import net.minecraft.client.Minecraft;
import net.minecraft.network.chat.Component;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.lwjgl.sdl.SDLKeycode;

public final class Toolscreen implements ClientModInitializer {
    public static final Logger LOGGER = LoggerFactory.getLogger("toolscreen-native");
    public static volatile boolean ready;
    public static volatile boolean failed;
    public static volatile String error = "";
    public static volatile int maximumTextureSize = 8192;
    private static boolean measuring;
    public static final String[] MODES = {"Fullscreen", "Thin", "Wide", "EyeZoom", "Preemptive"};

    @Override
    public void onInitializeClient() {
        try {
            NativeBridge.load();
            GameplaySettings.initialize();
            NativeBridge.setOption("minecraft_version","26.3");
            NinjabrainClient.start();
            Compositor.register();
            UiRenderer.register();
            ready = true;
            LOGGER.info("Macless native core loaded for Minecraft 26.3");
        } catch (Throwable failure) {
            error = failure.getMessage() == null ? failure.toString() : failure.getMessage();
            failed = true;
            LOGGER.error("Macless could not start", failure);
        }
    }

    public static boolean measurementPaused(){var screen=Minecraft.getInstance().gui.screen();return screen instanceof net.minecraft.client.gui.screens.PauseScreen pause&&!pause.showsPauseMenu();}

    public static void updateMeasurementPause(){boolean paused=measurementPaused();if(measuring&&!paused)NativeBridge.releaseHeldInput();measuring=paused;}

    public static boolean resourcesReady(){return ready&&Minecraft.getInstance().isGameLoadFinished();}

    public static boolean suspended() {
        Minecraft game = Minecraft.getInstance();
        return !resourcesReady() || failed || game.level == null || (game.gui.screen()!=null&&!(game.gui.screen() instanceof ToolscreenScreen)&&!measurementPaused()) || !NativeBridge.enabled();
    }

    public static boolean resizingSuspended(){return !resourcesReady()||failed||!NativeBridge.enabled();}

    public static boolean resizeShortcutsAllowed(){
        Minecraft game=Minecraft.getInstance();var screen=game.gui.screen();
        if(!game.isWindowActive()||screen instanceof ToolscreenScreen)return false;
        if(screen==null)return game.level!=null;
        if(screen instanceof net.minecraft.client.gui.screens.ChatScreen||screen instanceof net.minecraft.client.gui.screens.inventory.BookEditScreen||screen instanceof net.minecraft.client.gui.screens.inventory.AbstractSignEditScreen)return false;
        net.minecraft.client.gui.components.events.GuiEventListener focused=screen;
        for(int depth=0;focused!=null&&depth<32;depth++){
            if(focused instanceof net.minecraft.client.gui.components.EditBox||focused instanceof net.minecraft.client.gui.components.MultiLineEditBox)return false;
            if(focused instanceof net.minecraft.client.gui.components.events.ContainerEventHandler container)focused=container.getFocused();else break;
        }
        return true;
    }

    public static MenuCoordinates menuCoordinates(){
        var game=Minecraft.getInstance();var screen=game.gui.screen();var window=game.getWindow();
        if(resizingSuspended()||screen==null||screen instanceof ToolscreenScreen||window.getWidth()<1||window.getHeight()<1)return null;
        int[] source=viewport();return new MenuCoordinates(window.getWidth(),window.getHeight(),source[0],source[1],window.getGuiScaledWidth(),window.getGuiScaledHeight());
    }

    public static void updateMenuLayout(){
        if(!resourcesReady())return;var game=Minecraft.getInstance();var screen=game.gui.screen();
        if(screen==null||screen instanceof ToolscreenScreen)return;
        int width=game.getWindow().getGuiScaledWidth(),height=game.getWindow().getGuiScaledHeight();
        if(screen.width!=width||screen.height!=height)screen.resize(width,height);
    }

    public static int[] viewport() {
        Minecraft game=Minecraft.getInstance();
        NativeBridge.runtimeContext(game.level!=null,game.options.sensitivity().get());
        Window window = game.getWindow();
        return NativeBridge.viewport(window.getWidth(), window.getHeight(), maximumTextureSize, resizingSuspended());
    }

    public static boolean hotkey(int scan,int modifiers,int action) {
        Minecraft game = Minecraft.getInstance();
        if (!resourcesReady()||measurementPaused()) return false;
        var screen=game.gui.screen();
        if(screen instanceof ToolscreenScreen&&(NativeBridge.uiBindingActive()||NativeBridge.uiWantsText())){NativeBridge.releaseHeldInput();return false;}
        boolean gameplay=resizeShortcutsAllowed();
        boolean editor=gameplay||screen==null||screen instanceof ToolscreenScreen;
        int result=NativeBridge.inputEvent(scan,modifiers,action,gameplay,editor);
        if (result==1) {
            if (game.gui.screen() instanceof ToolscreenScreen) game.gui.screen().onClose();
            else game.gui.setScreen(new ToolscreenScreen(game.gui.screen()));
            return true;
        }
        if(result==2){NativeBridge.save();return true;}
        if(result==3)return true;
        if(result==4)NativeBridge.save();
        return false;
    }

    public static String backend() {
        try {
            return RenderSystem.getDevice().getDeviceInfo().backendName();
        } catch (Throwable failure) {
            return "Starting renderer";
        }
    }

    public static void fail(Throwable failure) {
        failed = true;
        error = failure.getMessage() == null ? failure.toString() : failure.getMessage();
        LOGGER.error("Macless composition stopped; game rendering remains active", failure);
        Minecraft game = Minecraft.getInstance();
        game.execute(() -> {
            if (game.player != null) game.player.sendSystemMessage(Component.literal("Macless paused. See the game log for details."));
        });
    }
}
