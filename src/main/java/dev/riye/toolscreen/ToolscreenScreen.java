package dev.riye.toolscreen;

import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.client.input.CharacterEvent;
import net.minecraft.client.input.KeyEvent;
import net.minecraft.client.input.MouseButtonEvent;
import net.minecraft.network.chat.Component;
import org.lwjgl.sdl.SDLKeycode;

public final class ToolscreenScreen extends Screen {
    private final Screen previous;
    private boolean focused=true;
    private long lastFrame = System.nanoTime();
    private boolean previewCaptured;

    public ToolscreenScreen(Screen previous) {
        super(Component.literal("Macless"));
        this.previous = previous;
    }

    @Override
    protected void init() {
        if (Toolscreen.ready){focused=minecraft.isWindowActive();NativeBridge.uiFocus(focused);}
        UiRenderer.publish(null);
    }

    @Override
    public void extractRenderState(GuiGraphicsExtractor graphics, int mouseX, int mouseY, float partialTick) {
        if (!Toolscreen.ready) return;
        if(focused!=minecraft.isWindowActive()){focused=minecraft.isWindowActive();NativeBridge.uiFocus(focused);}
        var window = minecraft.getWindow();
        NativeBridge.uiDisplayScale((float)window.getWidth()/Math.max(1,window.getScreenWidth()));
        if(!BuildVariant.VERIFICATION||!ModernVisualProbe.pointer())NativeBridge.uiMouse((float)(minecraft.mouseHandler.xpos()*window.getWidth()/window.getScreenWidth()),(float)(minecraft.mouseHandler.ypos()*window.getHeight()/window.getScreenHeight()));
        long now = System.nanoTime();
        float delta = (now - lastFrame) / 1.0e9f;
        lastFrame = now;
        UiRenderer.publish(NativeBridge.uiFrame(window.getWidth(), window.getHeight(), delta, Toolscreen.backend(), Toolscreen.error));
        minecraft.textInputManager().onTextInputFocusChange(this,focused&&NativeBridge.uiWantsText());
        if (NativeBridge.uiCloseRequested()) minecraft.execute(this::onClose);
    }

    @Override
    public boolean mouseClicked(MouseButtonEvent event, boolean doubleClick) {
        if (!Toolscreen.ready) return false;
        updateMouse(event);
        NativeBridge.uiButton(imguiButton(event.button()), true);
        return true;
    }

    @Override
    public boolean mouseReleased(MouseButtonEvent event) {
        if (!Toolscreen.ready) return false;
        updateMouse(event);
        NativeBridge.uiButton(imguiButton(event.button()), false);
        return true;
    }

    @Override
    public boolean mouseDragged(MouseButtonEvent event, double dragX, double dragY) {
        if (Toolscreen.ready) updateMouse(event);
        return true;
    }

    @Override
    public boolean mouseScrolled(double mouseX, double mouseY, double horizontal, double vertical) {
        if (Toolscreen.ready) NativeBridge.uiScroll((float) horizontal, (float) vertical);
        return true;
    }

    @Override
    public boolean keyPressed(KeyEvent event) {
        if (event.keycode() == SDLKeycode.SDLK_ESCAPE && (!Toolscreen.ready || !NativeBridge.uiEscapeActive())) {
            onClose();
            return true;
        }
        if (Toolscreen.ready&&!NativeBridge.uiPhysicalKey(event.key(),event.keycode(),event.modifiers(),true)) NativeBridge.uiKey(event.keycode(), event.modifiers(), true);
        return true;
    }

    @Override
    public boolean keyReleased(KeyEvent event) {
        if (Toolscreen.ready&&!NativeBridge.uiPhysicalKey(event.key(),event.keycode(),event.modifiers(),false)) NativeBridge.uiKey(event.keycode(), event.modifiers(), false);
        return true;
    }

    @Override
    public boolean charTyped(CharacterEvent event) {
        if (Toolscreen.ready&&!NativeBridge.uiBindingActive()) NativeBridge.uiText(event.codepoint());
        return true;
    }

    private void updateMouse(MouseButtonEvent event) {
        if(BuildVariant.VERIFICATION&&ModernVisualProbe.pointer())return;
        var window=minecraft.getWindow();
        NativeBridge.uiDisplayScale((float)window.getWidth()/Math.max(1,window.getScreenWidth()));
        NativeBridge.uiMouse((float)(minecraft.mouseHandler.xpos()*window.getWidth()/Math.max(1,window.getScreenWidth())),(float)(minecraft.mouseHandler.ypos()*window.getHeight()/Math.max(1,window.getScreenHeight())));
    }

    private int imguiButton(int button) {
        return switch (button) {
            case 1 -> 0;
            case 2 -> 2;
            case 3 -> 1;
            default -> button - 1;
        };
    }

    @Override
    public void onClose() {
        if (Toolscreen.ready && !NativeBridge.save()) return;
        minecraft.gui.setScreen(previous);
    }

    @Override
    public void removed() {
        minecraft.textInputManager().stopTextInput(this);
        if (Toolscreen.ready) NativeBridge.uiFocus(false);
        UiRenderer.publish(null);
        UiRenderer.releasePreview();
    }

    @Override
    public boolean isInputCaptured(){return Toolscreen.ready&&(NativeBridge.uiWantsText()||NativeBridge.uiBindingActive());}

    @Override
    public boolean isPauseScreen() {
        return false;
    }
}
