package dev.riye.toolscreen;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.concurrent.TimeUnit;
import java.util.function.BooleanSupplier;

public final class DesktopJniContractTest {
    private static int checks;
    private static void check(boolean value,String message){if(!value)throw new AssertionError(message);checks++;}
    private static void await(BooleanSupplier condition,String message)throws Exception{long deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(25);boolean ready=condition.getAsBoolean();while(!ready&&System.nanoTime()<deadline){Thread.sleep(20);ready=condition.getAsBoolean();}check(ready,message);}

    public static void main(String[] arguments)throws Exception{
        if(arguments.length<1)throw new IllegalArgumentException("Provide the desktop native library");Path directory=Files.createTempDirectory("macless-jni-");DesktopPlatform.configure(directory);System.load(Path.of(arguments[0]).toAbsolutePath().toString());var initialize=NativeBridge.class.getDeclaredMethod("initialize",String.class);initialize.setAccessible(true);check((boolean)initialize.invoke(null,directory.resolve("settings.conf").toString()),"native initialization");
        check(NativeBridge.snapshot().length>=150,"settings JNI");check(NativeBridge.save()&&NativeBridge.save(),"replace existing settings");NativeBridge.debugCounter("E: 123/456");NativeBridge.debugPie("root/level",new String[]{"blockEntities","tick"},new double[]{100,50,12.5,0x123456,50,50,0xffffff});check(NativeBridge.debugProbe()[0]==1&&NativeBridge.debugProbe()[1]==2,"debug JNI");
        byte[] frame=NativeBridge.uiFrame(1280,720,1f/60,"OpenGL","");check(frame.length>8&&ByteBuffer.wrap(frame).order(ByteOrder.LITTLE_ENDIAN).getInt()>0,"shared ImGui frame");check(NativeBridge.fontAtlas().length>1024,"high resolution font atlas");
        if(arguments.length>1){
            System.setProperty("macless.browser",arguments[1]);Path html=directory.resolve("source.html");Files.writeString(html,"<html><style>html,body{margin:0;background:rgb(16,32,48)}</style></html>");NativeBridge.startSource(100,"browser:"+html.toUri());try{await(()->NativeBridge.sourceInfo(100)[2]>0||!NativeBridge.sourceError(100).isBlank(),"browser callback through JNI");check(NativeBridge.sourceError(100).isBlank(),"browser error: "+NativeBridge.sourceError(100));int[] info=NativeBridge.sourceInfo(100);check(info[0]==960&&info[1]==540,"browser dimensions through JNI");ByteBuffer image=ByteBuffer.allocateDirect(info[0]*info[1]*4);check(NativeBridge.readSource(100,image,0)>0,"browser pixel transfer through JNI");}finally{NativeBridge.stopSource(100);}check(NativeBridge.sourceInfo(100)[2]==0,"browser callback stopped");
        }
        System.out.println("MACLESS_DESKTOP_JNI_PASS checks="+checks);
    }
}
