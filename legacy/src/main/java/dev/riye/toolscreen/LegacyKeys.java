package dev.riye.toolscreen;

import org.lwjgl.glfw.GLFW;

public final class LegacyKeys {
    private LegacyKeys(){}
    public static int modifiers(long window,int fallback){int result=0;int[] keys={341,340,342,343,345,344,346,347};int[] masks={64,1,256,1024,128,2,512,2048};for(int i=0;i<keys.length;i++)if(GLFW.glfwGetKey(window,keys[i])==GLFW.GLFW_PRESS)result|=masks[i];if((result&3)==0&&(fallback&1)!=0)result|=1;if((result&192)==0&&(fallback&2)!=0)result|=64;if((result&768)==0&&(fallback&4)!=0)result|=256;if((result&3072)==0&&(fallback&8)!=0)result|=1024;return result;}
}
