package dev.riye.toolscreen;

import java.awt.GraphicsEnvironment;
import java.awt.Toolkit;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.StringSelection;
import java.io.DataInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HexFormat;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import javax.swing.JFileChooser;
import javax.swing.SwingUtilities;
import javax.swing.filechooser.FileNameExtensionFilter;

public final class DesktopPlatform {
    private record Window(long id,long pid,String app,String title){}
    private static final ConcurrentHashMap<Integer,DesktopSource> SOURCES=new ConcurrentHashMap<>();
    private static final AtomicBoolean REFRESHING=new AtomicBoolean();
    private static volatile List<Window> windows=List.of();
    private static volatile String status="";
    private static volatile Path directory;
    private static final long PORTAL=2147483646;

    private DesktopPlatform(){}
    private static void startWorker(String name,Runnable action){Thread worker=new Thread(action,name);worker.setDaemon(true);worker.start();}
    public static void configure(Path path){directory=path;Runtime.getRuntime().addShutdownHook(new Thread(()->{for(var source:SOURCES.values())source.close();},"macless-desktop-close"));}
    public static String readClipboard(){if(wayland())try{Process process=new ProcessBuilder("wl-paste","--no-newline","--type","text").start();if(!process.waitFor(2,TimeUnit.SECONDS)){process.destroyForcibly();return "";}if(process.exitValue()==0)return new String(process.getInputStream().readNBytes(65536),StandardCharsets.UTF_8);}catch(Exception ignored){}try{return (String)Toolkit.getDefaultToolkit().getSystemClipboard().getData(DataFlavor.stringFlavor);}catch(Exception failure){return "";}}
    public static void writeClipboard(String value){if(wayland())try{Process process=new ProcessBuilder("wl-copy","--type","text/plain;charset=utf-8").start();try(var input=process.getOutputStream()){input.write(value.getBytes(StandardCharsets.UTF_8));}if(!process.waitFor(2,TimeUnit.SECONDS))process.destroyForcibly();if(process.exitValue()==0)return;}catch(Exception ignored){}try{Toolkit.getDefaultToolkit().getSystemClipboard().setContents(new StringSelection(value),null);}catch(RuntimeException ignored){}}
    public static void pickImage(boolean background){if(wayland()&&GraphicsEnvironment.isHeadless()){startWorker("macless-image-picker",()->{try{Process process=new ProcessBuilder("zenity","--file-selection","--title=Choose an image","--file-filter=Images | *.png *.jpg *.jpeg *.bmp *.gif *.webp *.svg").start();String result=new String(process.getInputStream().readNBytes(16384),StandardCharsets.UTF_8).strip();if(process.waitFor()==0&&!result.isBlank())NativeBridge.addSelectedImage(result,background);}catch(Exception failure){status="Image picker needs Zenity or XWayland";}});return;}SwingUtilities.invokeLater(()->{JFileChooser picker=new JFileChooser();picker.setDialogTitle("Choose an image");picker.setAcceptAllFileFilterUsed(false);picker.setFileFilter(new FileNameExtensionFilter("Images","png","jpg","jpeg","bmp","gif","webp","svg"));if(picker.showOpenDialog(null)==JFileChooser.APPROVE_OPTION)NativeBridge.addSelectedImage(picker.getSelectedFile().toPath().toAbsolutePath().toString(),background);});}
    private static boolean wayland(){return !System.getenv().getOrDefault("WAYLAND_DISPLAY","").isBlank()||"wayland".equalsIgnoreCase(System.getenv("XDG_SESSION_TYPE"));}
    public static boolean captureAvailable(){return wayland()||!GraphicsEnvironment.isHeadless()&&(System.getProperty("os.name").startsWith("Windows")||!System.getenv().getOrDefault("DISPLAY","").isBlank());}

    public static void refreshWindows(){
        if(!REFRESHING.compareAndSet(false,true))return;status="Refreshing windows...";
        startWorker("macless-window-list",()->{try{
            if(wayland()){windows=List.of(new Window(PORTAL,0,"System picker","Choose a window or screen"));status="Select a source to open the system capture dialog";return;}
            Process process=new ProcessBuilder(helperCommand("--list")).start();ArrayList<Window> result=new ArrayList<>();try(var input=process.inputReader(StandardCharsets.UTF_8)){for(String line;(line=input.readLine())!=null&&result.size()<512;){String[] fields=line.split("\t",4);if(fields.length==4)try{result.add(new Window(Long.parseUnsignedLong(fields[0]),Long.parseLong(fields[1]),fields[2],fields[3]));}catch(NumberFormatException ignored){}}}if(!process.waitFor(8,TimeUnit.SECONDS)){process.destroyForcibly();throw new IOException("Window list timed out");}if(process.exitValue()!=0)throw new IOException(new String(process.getErrorStream().readNBytes(1024),StandardCharsets.UTF_8));windows=List.copyOf(result);status=result.isEmpty()?"No capturable windows found":"";
        }catch(Exception failure){status=message(failure);}finally{REFRESHING.set(false);}});
    }
    public static String[] windows(){return windows.stream().map(window->Long.toUnsignedString(window.id)+"\t"+window.app+"\t"+window.title).toArray(String[]::new);}
    public static String captureStatus(){return status;}

    private static Window selectedWindow(String[] selection){
        long requested="self".equals(selection[0])?0:Long.parseUnsignedLong(selection[0]);
        for(Window candidate:windows)if(requested==0?candidate.pid==ProcessHandle.current().pid():candidate.id==requested&&(selection.length<3||candidate.app.equals(selection[1])&&candidate.title.equals(selection[2])))return candidate;
        if(requested!=0&&selection.length==3)for(Window candidate:windows)if(candidate.app.equals(selection[1])&&candidate.title.equals(selection[2]))return candidate;
        return null;
    }
    public static void startSource(int id,String path){
        if(SOURCES.containsKey(id))return;DesktopSource source;
        if(path.startsWith("browser:"))source=new BrowserSource(browserExecutable(),path.substring(8));else source=new DesktopSource(){
            void runSource()throws Exception{
                if(!path.startsWith("window:"))throw new IOException("Unsupported source");String[] selection=path.substring(7).split("\\|",3);Window chosen=null;
                if(!wayland()){
                    chosen=selectedWindow(selection);
                    if(chosen==null){refreshWindows();long deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(10);while(REFRESHING.get()&&System.nanoTime()<deadline&&!stopped)Thread.sleep(50);chosen=selectedWindow(selection);}
                    if(chosen==null)throw new IOException("The selected window is unavailable. Choose it again");
                }
                process=new ProcessBuilder(helperCommand("--capture",wayland()?"portal":Long.toUnsignedString(chosen.id))).start();startWorker("macless-capture-errors",()->{try{String value=new String(process.getErrorStream().readNBytes(2048),StandardCharsets.UTF_8).trim();if(!value.isBlank()&&!stopped)error=value;}catch(IOException ignored){}});
                try(DataInputStream input=new DataInputStream(process.getInputStream())){while(!stopped){if(input.readInt()!=0x4d434631)throw new IOException("Invalid capture stream");int width=input.readInt(),height=input.readInt(),length=input.readInt();if(width<1||height<1||(long)width*height>16777216||length!=(long)width*height*4)throw new IOException("Invalid capture dimensions");byte[] rgba=input.readNBytes(length);if(rgba.length!=length)throw new IOException("Capture ended");store(width,height,rgba);}}
            }
        };
        if(SOURCES.putIfAbsent(id,source)==null)source.start();
    }
    public static void stopSource(int id){DesktopSource source=SOURCES.remove(id);if(source!=null)source.close();}
    public static int[] sourceInfo(int id){DesktopSource source=SOURCES.get(id);return source==null?new int[3]:source.info();}
    public static int readSource(int id,ByteBuffer buffer,int sequence){DesktopSource source=SOURCES.get(id);return source==null?0:source.read(buffer,sequence);}
    public static String sourceError(int id){DesktopSource source=SOURCES.get(id);return source==null?"":source.error;}

    private static List<String> helperCommand(String... arguments)throws Exception{
        NativePlatform platform=NativePlatform.current();ArrayList<String> command=new ArrayList<>();if(platform.os().equals("windows"))command.add(extract(platform.resource().replace(platform.library(),"macless-capture.exe")).toString());else{command.add(System.getProperty("macless.python","python3"));command.add(extract(wayland()?"/macless/capture_portal.py":"/macless/capture_x11.py").toString());}command.addAll(Arrays.asList(arguments));return command;
    }
    private static synchronized Path extract(String resource)throws Exception{
        if(directory==null)throw new IOException("Desktop services were not initialized");byte[] bytes;try(var input=DesktopPlatform.class.getResourceAsStream(resource)){if(input==null)throw new IOException("Capture helper is missing: "+resource);bytes=input.readAllBytes();}String name=Path.of(resource).getFileName().toString();int dot=name.lastIndexOf('.');String hash=HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(bytes)).substring(0,16);Path cache=directory.resolve("helpers");Files.createDirectories(cache);Path destination=cache.resolve(name.substring(0,dot)+"-"+hash+name.substring(dot));if(!Files.exists(destination)||!MessageDigest.isEqual(Files.readAllBytes(destination),bytes)){Path temporary=Files.createTempFile(cache,"helper-",".tmp");Files.write(temporary,bytes);Files.move(temporary,destination,StandardCopyOption.REPLACE_EXISTING);}return destination;
    }
    static String browserExecutable(){
        String configured=System.getProperty("macless.browser","");if(configured.isBlank())try{configured=NativeBridge.option("browser_executable");}catch(UnsatisfiedLinkError ignored){}if(!configured.isBlank())return configured;
        ArrayList<String> candidates=new ArrayList<>();for(String root:new String[]{System.getenv("PROGRAMFILES"),System.getenv("PROGRAMFILES(X86)"),System.getenv("LOCALAPPDATA")})if(root!=null){candidates.add(root+"/Microsoft/Edge/Application/msedge.exe");candidates.add(root+"/Google/Chrome/Application/chrome.exe");}candidates.addAll(List.of("/usr/bin/chromium","/usr/bin/chromium-browser","/usr/bin/google-chrome","/usr/bin/google-chrome-stable","/opt/google/chrome/chrome","/snap/bin/chromium","/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"));for(String candidate:candidates)if(Files.isRegularFile(Path.of(candidate)))return candidate;return "";
    }
    private static String message(Exception failure){String value=failure.getMessage();return value==null?failure.getClass().getSimpleName():value;}
}
