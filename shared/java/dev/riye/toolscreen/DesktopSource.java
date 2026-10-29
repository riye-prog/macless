package dev.riye.toolscreen;

import java.awt.image.BufferedImage;
import java.nio.ByteBuffer;

abstract class DesktopSource implements AutoCloseable {
    record Frame(int width,int height,int sequence,byte[] pixels){}
    volatile Frame frame;
    volatile String error="";
    volatile boolean stopped;
    volatile Process process;
    private Thread worker;
    private int sequence;

    void start(){worker=new Thread(()->{try{runSource();}catch(Exception failure){if(!stopped)error=failure.getMessage()==null?failure.getClass().getSimpleName():failure.getMessage();}finally{if(process!=null)process.destroy();}},"macless-layer");worker.setDaemon(true);worker.start();}
    abstract void runSource()throws Exception;
    void store(int width,int height,byte[] pixels){if(stopped||width<1||height<1||(long)width*height>16777216||pixels.length!=(long)width*height*4)return;frame=new Frame(width,height,++sequence,pixels);error="";}
    void store(BufferedImage image){int width=image.getWidth(),height=image.getHeight();int[] pixels=image.getRGB(0,0,width,height,null,0,width);byte[] rgba=new byte[width*height*4];for(int i=0;i<pixels.length;i++){int value=pixels[i],at=i*4;rgba[at]=(byte)(value>>>16);rgba[at+1]=(byte)(value>>>8);rgba[at+2]=(byte)value;rgba[at+3]=(byte)(value>>>24);}store(width,height,rgba);}
    int[] info(){Frame value=frame;return value==null?new int[3]:new int[]{value.width,value.height,value.sequence};}
    int read(ByteBuffer destination,int previous){Frame value=frame;if(value==null||value.sequence==previous||destination.remaining()!=value.pixels.length)return 0;destination.put(value.pixels);return value.sequence;}
    public void close(){stopped=true;if(process!=null)process.destroy();if(worker!=null)worker.interrupt();frame=null;}
}
