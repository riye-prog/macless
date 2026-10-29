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

    public static boolean suspended() {
        Minecraft game = Minecraft.getInstance();
        return !ready || failed || game.level == null || (game.gui.screen()!=null&&!(game.gui.screen() instanceof ToolscreenScreen)&&!measurementPaused()) || !NativeBridge.enabled();
    }

    public static int[] viewport() {
        Minecraft game=Minecraft.getInstance();
        NativeBridge.runtimeContext(game.level!=null,game.options.sensitivity().get());
        Window window = game.getWindow();
        return NativeBridge.viewport(window.getWidth(), window.getHeight(), maximumTextureSize, suspended());
    }

    public static boolean hotkey(int scan,int modifiers,int action) {
        Minecraft game = Minecraft.getInstance();
        if (!ready||measurementPaused()) return false;
        var screen=game.gui.screen();
        if(screen instanceof ToolscreenScreen&&(NativeBridge.uiBindingActive()||NativeBridge.uiWantsText())){NativeBridge.releaseHeldInput();return false;}
        boolean gameplay=game.level!=null&&screen==null&&game.isWindowActive();
        boolean editor=screen==null||screen instanceof ToolscreenScreen||screen instanceof net.minecraft.client.gui.screens.TitleScreen;
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
