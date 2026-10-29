package dev.riye.toolscreen;

import java.io.IOException;
import java.util.Locale;

public record NativePlatform(String os,String architecture,String library) {
    public static NativePlatform current() throws IOException {
        return select(System.getProperty("os.name"),System.getProperty("os.arch"));
    }

    public static NativePlatform select(String system,String cpu) throws IOException {
        String name=system.toLowerCase(Locale.ROOT);
        String os=name.contains("mac")||name.contains("darwin")?"macos":name.contains("win")?"windows":name.contains("linux")?"linux":"";
        String architecture=switch(cpu.toLowerCase(Locale.ROOT)){case "aarch64","arm64"->"arm64";case "amd64","x86_64","x64"->"x64";default->"";};
        if(os.isEmpty()||architecture.isEmpty()||os.equals("windows")&&!architecture.equals("x64"))throw new IOException("Unsupported Macless platform: "+system+" "+cpu);
        return new NativePlatform(os,architecture,os.equals("windows")?"toolscreen.dll":os.equals("macos")?"libtoolscreen.dylib":"libtoolscreen.so");
    }

    public String resource(){return "/natives/"+os+"-"+architecture+"/"+library;}
    public String extension(){return library.substring(library.lastIndexOf('.'));}
}
