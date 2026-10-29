package dev.riye.toolscreen;

import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.network.chat.TextComponent;

public final class ToolscreenScreen extends Screen {
    private final Screen previous;
    private boolean focused=true;
    private long lastFrame=System.nanoTime();
    private boolean captured;
    public ToolscreenScreen(Screen previous){super(new TextComponent("Macless"));this.previous=previous;}
    @Override protected void init(){if(Toolscreen.ready){focused=minecraft.isWindowActive();NativeBridge.uiFocus(focused);}}
    @Override public void render(PoseStack pose,int mouseX,int mouseY,float partial){}
    public void frame(){if(!Toolscreen.ready)return;if(focused!=minecraft.isWindowActive()){focused=minecraft.isWindowActive();NativeBridge.uiFocus(focused);}var window=minecraft.getWindow();int[] display=LegacyRenderer.displayInfo();NativeBridge.uiDisplayScale((float)display[0]/display[2]);if(!BuildVariant.VERIFICATION||!LegacyVisualProbe.pointer())NativeBridge.uiMouse((float)(minecraft.mouseHandler.xpos()*display[0]/display[2]),(float)(minecraft.mouseHandler.ypos()*display[1]/display[3]));long now=System.nanoTime();LegacyRenderer.uiPacket=NativeBridge.uiFrame(display[0],display[1],(now-lastFrame)/1.0e9f,"OpenGL",Toolscreen.error);lastFrame=now;if(NativeBridge.uiCloseRequested())minecraft.execute(this::onClose);}
    private void mouse(double x,double y){if(BuildVariant.VERIFICATION&&LegacyVisualProbe.pointer())return;var window=minecraft.getWindow();int[] display=LegacyRenderer.displayInfo();NativeBridge.uiDisplayScale((float)display[0]/display[2]);NativeBridge.uiMouse((float)(minecraft.mouseHandler.xpos()*display[0]/display[2]),(float)(minecraft.mouseHandler.ypos()*display[1]/display[3]));}
    @Override public boolean mouseClicked(double x,double y,int button){mouse(x,y);NativeBridge.uiButton(button,true);return true;}
    @Override public boolean mouseReleased(double x,double y,int button){mouse(x,y);NativeBridge.uiButton(button,false);return true;}
    @Override public boolean mouseDragged(double x,double y,int button,double dx,double dy){mouse(x,y);return true;}
    @Override public boolean mouseScrolled(double x,double y,double delta){NativeBridge.uiScroll(0,(float)delta);return true;}
    @Override public boolean keyPressed(int key,int scan,int modifiers){int hid=PhysicalKeys.fromGlfw(key,scan);int code=PhysicalKeys.sdlCode(hid);int mask=LegacyKeys.modifiers(minecraft.getWindow().getWindow(),modifiers);if(key==256&&!NativeBridge.uiEscapeActive()){onClose();return true;}if(!NativeBridge.uiPhysicalKey(hid,code,mask,true))NativeBridge.uiKey(code,mask,true);return true;}
    @Override public boolean keyReleased(int key,int scan,int modifiers){int hid=PhysicalKeys.fromGlfw(key,scan);int code=PhysicalKeys.sdlCode(hid);int mask=LegacyKeys.modifiers(minecraft.getWindow().getWindow(),modifiers);if(!NativeBridge.uiPhysicalKey(hid,code,mask,false))NativeBridge.uiKey(code,mask,false);return true;}
    @Override public boolean charTyped(char character,int modifiers){if(!NativeBridge.uiBindingActive())NativeBridge.uiText(character);return true;}
    @Override public void onClose(){if(NativeBridge.save())minecraft.setScreen(previous);}
    @Override public void removed(){NativeBridge.uiFocus(false);LegacyRenderer.releasePreview();LegacyRenderer.uiPacket=null;}
    @Override public boolean isPauseScreen(){return false;}
}
