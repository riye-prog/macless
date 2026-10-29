package dev.riye.toolscreen;

import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.Arrays;
import java.util.concurrent.TimeUnit;
import java.util.function.BooleanSupplier;

public final class DesktopContractTest {
    private static int checks;
    private static void check(boolean value,String message){if(!value)throw new AssertionError(message);checks++;}
    private static void await(BooleanSupplier condition,String message)throws Exception{long deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(25);boolean ready=condition.getAsBoolean();while(!ready&&System.nanoTime()<deadline){Thread.sleep(20);ready=condition.getAsBoolean();}check(ready,message);}


    public static void main(String[] arguments)throws Exception{
        check(NativePlatform.select("Mac OS X","aarch64").resource().equals("/natives/macos-arm64/libtoolscreen.dylib"),"macOS ARM loader");check(NativePlatform.select("Darwin","x86_64").resource().equals("/natives/macos-x64/libtoolscreen.dylib"),"macOS Intel loader");check(NativePlatform.select("Windows 11","amd64").resource().equals("/natives/windows-x64/toolscreen.dll"),"Windows loader");check(NativePlatform.select("Linux","aarch64").resource().equals("/natives/linux-arm64/libtoolscreen.so"),"Linux ARM loader");check(NativePlatform.select("Linux","x86_64").resource().equals("/natives/linux-x64/libtoolscreen.so"),"Linux x64 loader");
        for(String[] unsupported:new String[][]{{"Plan 9","amd64"},{"Windows 11","arm64"},{"Linux","riscv64"}}){boolean rejected=false;try{NativePlatform.select(unsupported[0],unsupported[1]);}catch(IOException expected){rejected=true;}check(rejected,"unsupported platform fails clearly");}
        check(PhysicalKeys.fromGlfw(65,30,false)==4,"Windows physical A");check(PhysicalKeys.fromGlfw(65,38,false)==4,"Linux physical A");check(PhysicalKeys.fromGlfw(292,61,false)==60,"Windows F3");check(PhysicalKeys.fromGlfw(292,69,false)==60,"Linux F3");check(PhysicalKeys.fromGlfw(65,0,true)==4,"macOS scan mapping retained");check(PhysicalKeys.fromGlfw(161,86,false)==100,"ISO keyboard key");
        if(arguments.length>0){
            Path html=Files.createTempFile("macless-browser-test-",".html");Files.writeString(html,"<html><style>html,body{margin:0;background:rgb(16,32,48)}</style><canvas width=100 height=100></canvas><script>let c=document.querySelector('canvas').getContext('2d');c.fillStyle='rgb(220,40,80)';c.fillRect(0,0,100,100);</script></html>");
            BrowserSource browser=new BrowserSource(arguments[0],html.toUri().toString());try{browser.start();await(()->browser.frame!=null||!browser.error.isEmpty(),"browser produces pixels");check(browser.error.isEmpty(),"browser source error: "+browser.error);await(()->{var frame=browser.frame;if(frame==null)return false;int at=(200*frame.width()+200)*4;return (frame.pixels()[at]&255)==16&&(frame.pixels()[at+1]&255)==32&&(frame.pixels()[at+2]&255)==48;},"browser JavaScript and color capture");check(browser.info()[0]==960&&browser.info()[1]==540,"browser viewport size");int sequence=browser.info()[2];await(()->browser.info()[2]>sequence,"browser source updates");}finally{browser.close();Files.deleteIfExists(html);}check(browser.stopped,"browser closes");
        }
        System.out.println("MACLESS_DESKTOP_PASS checks="+checks);
    }
}
