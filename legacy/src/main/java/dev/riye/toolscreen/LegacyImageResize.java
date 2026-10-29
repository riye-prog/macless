package dev.riye.toolscreen;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import org.lwjgl.stb.STBImageResize;

public final class LegacyImageResize {
    private static final MethodHandle RESIZE=find();
    private LegacyImageResize(){}
    private static MethodHandle find(){try{return MethodHandles.publicLookup().findStatic(STBImageResize.class,"nstbir_resize_uint8_linear",MethodType.methodType(long.class,long.class,int.class,int.class,int.class,long.class,int.class,int.class,int.class,int.class));}catch(NoSuchMethodException|IllegalAccessException ignored){return null;}}
    public static boolean available(){return RESIZE!=null;}
    public static void resize(long source,int width,int height,int stride,long target,int targetWidth,int targetHeight,int channels){try{int layout=channels==2?9:channels;long result=(long)RESIZE.invokeExact(source,width,height,stride,target,targetWidth,targetHeight,0,layout);if(result==0)throw new IllegalStateException("STB image resize failed");}catch(Throwable failure){throw new IllegalStateException("Could not resize the legacy image",failure);}}
}
