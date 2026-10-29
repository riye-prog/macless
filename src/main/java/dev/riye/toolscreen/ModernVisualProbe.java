package dev.riye.toolscreen;

import com.mojang.blaze3d.pipeline.RenderTarget;
import com.mojang.renderpearl.api.textures.GpuTextureView;
import dev.riye.toolscreen.mixin.MouseAccess;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.client.Minecraft;
import net.minecraft.client.Screenshot;
import net.minecraft.client.gui.components.debug.DebugScreenEntries;
import net.minecraft.client.gui.components.debug.DebugScreenEntryStatus;
import net.minecraft.client.input.MouseButtonEvent;
import net.minecraft.client.input.MouseButtonInfo;

public final class ModernVisualProbe {
    private static boolean opened,capture;
    private static int frames,acknowledgementFrames;
    private static String previous="",acknowledgement="";
    private static float pointerX=-1,pointerY=-1;
    private static int scan(int code){for(int key=1;key<=511;++key)if(PhysicalKeys.sdlCode(key)==code)return key;return 0;}
    public static boolean pointer(){if(!Boolean.getBoolean("toolscreen.visual")||pointerX<0)return false;NativeBridge.uiMouse(pointerX,pointerY);return true;}
    private static final Path DIRECTORY=Path.of("verification");
    private ModernVisualProbe(){}
    private static final class CaptureTarget extends RenderTarget {
        CaptureTarget(GpuTextureView view){super("Macless visual verification",view.texture().getFormat(),null);colorTexture=view.texture();colorTextureView=view;width=colorTexture.getWidth(0);height=colorTexture.getHeight(0);}
    }
    public static void tick(){
        if(!Boolean.getBoolean("toolscreen.visual"))return;
        Minecraft game=Minecraft.getInstance();
        try{
            Files.createDirectories(DIRECTORY);
            if(!opened){opened=true;game.options.pauseOnLostFocus=false;game.debugEntries.setOverlayVisible(true);game.debugEntries.setStatus(DebugScreenEntries.ENTITY_RENDER_STATS,DebugScreenEntryStatus.IN_OVERLAY);if(!game.getDebugOverlay().showProfilerChart())game.getDebugOverlay().toggleProfilerChart();game.gui.setScreen(new ToolscreenScreen(null));}
            Path input=DIRECTORY.resolve("visual-input");
            if(Files.exists(input)){
                String value=Files.readString(input).trim();
                if(!value.equals(previous)){
                    previous=value;NativeBridge.uiFocus(true);String command=value.substring(value.indexOf('|')+1);String[] parts=command.split(" ");
                    switch(parts[0]){
                        case "resize" -> game.getWindow().setWindowed(Integer.parseInt(parts[1]),Integer.parseInt(parts[2]));
                        case "mouse" -> {pointerX=Float.parseFloat(parts[1]);pointerY=Float.parseFloat(parts[2]);var window=game.getWindow();((MouseAccess)game.mouseHandler).toolscreenCursorX(pointerX*window.getScreenWidth()/window.getWidth());((MouseAccess)game.mouseHandler).toolscreenCursorY(pointerY*window.getScreenHeight()/window.getHeight());NativeBridge.uiMouse(pointerX,pointerY);}
                        case "button" -> {if(game.gui.screen() instanceof ToolscreenScreen screen){var window=game.getWindow();double x=game.mouseHandler.xpos()*window.getGuiScaledWidth()/Math.max(1,window.getScreenWidth()),y=game.mouseHandler.ypos()*window.getGuiScaledHeight()/Math.max(1,window.getScreenHeight());int button=Integer.parseInt(parts[1]);var event=new MouseButtonEvent(x,y,new MouseButtonInfo(button==0?1:button==1?3:2,0));if(Integer.parseInt(parts[2])!=0)screen.mouseClicked(event,false);else screen.mouseReleased(event);}}
                        case "scroll" -> NativeBridge.uiScroll(Float.parseFloat(parts[1]),Float.parseFloat(parts[2]));
                        case "key" -> {if(game.gui.screen() instanceof ToolscreenScreen screen){var event=new net.minecraft.client.input.KeyEvent(scan(Integer.parseInt(parts[1])),Integer.parseInt(parts[1]),Integer.parseInt(parts[2]));if(Integer.parseInt(parts[3])!=0)screen.keyPressed(event);else screen.keyReleased(event);}}
                        case "text" -> game.keyboardHandler.textInput(game.getWindow().handle(),command.substring(5));
                        case "set" -> NativeBridge.set(Integer.parseInt(parts[1]),Double.parseDouble(parts[2]));
                        case "mode" -> NativeBridge.set(0,Integer.parseInt(parts[1]));
                        case "game" -> game.gui.setScreen(null);
                        case "studio" -> game.gui.setScreen(new ToolscreenScreen(null));
                        case "profiler" -> {boolean enabled=parts[1].equals("on");if(game.getDebugOverlay().showProfilerChart()!=enabled)game.getDebugOverlay().toggleProfilerChart();}
                        case "profile" -> game.getDebugOverlay().getProfilerPieChart().profilerPieChartKeyPress(Integer.parseInt(parts[1]));
                        case "capture" -> capture=true;
                        case "probe" -> Files.writeString(DIRECTORY.resolve("visual-probe.json"),java.util.Arrays.toString(NativeBridge.uiProbe(Integer.parseInt(parts[1]))));
                        case "state" -> Files.writeString(DIRECTORY.resolve("visual-state.json"),"{\"textFocused\":"+NativeBridge.uiWantsText()+",\"sdlTextInput\":"+org.lwjgl.sdl.SDLKeyboard.SDL_TextInputActive(game.getWindow().handle())+",\"settings\":"+java.util.Arrays.toString(NativeBridge.snapshot())+",\"mode\":"+(int)NativeBridge.snapshot()[0]+",\"width\":"+game.getWindow().getWidth()+",\"height\":"+game.getWindow().getHeight()+",\"profiler\":"+java.util.Arrays.toString(NativeBridge.debugProbe())+"}");
                        case "profileinfo" -> {var chart=game.getDebugOverlay().getProfilerPieChart();var pathField=chart.getClass().getDeclaredField("profilerTreePath");var resultsField=chart.getClass().getDeclaredField("profilerPieChartResults");pathField.setAccessible(true);resultsField.setAccessible(true);var results=(net.minecraft.util.profiling.ProfileResults)resultsField.get(chart);String path=(String)pathField.get(chart);var rows=results==null?java.util.List.<net.minecraft.util.profiling.ResultField>of():results.getTimes(path);StringBuilder output=new StringBuilder(path.replace('\u001e','/'));for(int i=1;i<rows.size();++i)output.append('\n').append(i).append(' ').append(rows.get(i).name);Files.writeString(DIRECTORY.resolve("visual-profiler.txt"),output.toString());}
                        default -> {}
                    }
                    acknowledgement=value;acknowledgementFrames=3;
                }
            }
            if(!acknowledgement.isEmpty()&&--acknowledgementFrames<=0){Files.writeString(DIRECTORY.resolve("visual-ack"),acknowledgement);acknowledgement="";}
            if(++frames==30)capture=true;
            if(Files.exists(DIRECTORY.resolve("visual-close")))game.stop();
        }catch(Exception failure){Toolscreen.LOGGER.error("Macless visual verification failed",failure);}
    }
    public static void observe(GpuTextureView frame){
        if(!opened||!capture)return;capture=false;
        Screenshot.takeScreenshot(new CaptureTarget(frame),image->{try(image){Files.createDirectories(DIRECTORY);image.writeToFile(DIRECTORY.resolve("visual-window.png"));Toolscreen.LOGGER.info("TOOLSCREEN_VISUAL_READY backend={}",Toolscreen.backend());}catch(Exception failure){Toolscreen.LOGGER.error("Macless visual capture failed",failure);}});
    }
}
