package dev.riye.toolscreen;

import java.awt.image.BufferedImage;
import java.nio.ByteBuffer;

public final class RuntimeVerification {
    private static boolean ninja,mirrors;
    private RuntimeVerification(){}
    public static void start(){VerificationFixtures.start();VerificationFixtures.createMedia();}
    public static boolean ready(int width,int height){double[] state=NativeBridge.debugProbe();if(!ninja&&state[4]==2&&state[5]==1&&state[6]==1604&&state[7]==-796&&Math.abs(state[8]-.987)<.00001){ninja=true;Toolscreen.LOGGER.info("TOOLSCREEN_NINJA_PASS x={} z={} confidence={}",state[6],state[7],state[8]);}if(!mirrors&&state[0]==1&&state[1]>0&&state[2]==1){mirrors=true;Toolscreen.LOGGER.info("TOOLSCREEN_MIRRORS_PASS rows={} mapless={}",state[1],state[3]);}return ninja&&mirrors&&width>0&&height>0;}
    public static void inspect(BufferedImage display,String name,String backend,boolean studio){if(display.getWidth()<1||display.getHeight()<1)throw new IllegalStateException("Empty display capture");if(studio)Toolscreen.LOGGER.info("TOOLSCREEN_STUDIO_PASS stage={} backend={}",name,backend);}
    public static BufferedImage rgba(ByteBuffer pixels,int width,int height){BufferedImage image=new BufferedImage(width,height,BufferedImage.TYPE_INT_ARGB);for(int y=0;y<height;y++)for(int x=0;x<width;x++){int offset=(y*width+x)*4;image.setRGB(x,y,0xff000000|((pixels.get(offset)&255)<<16)|((pixels.get(offset+1)&255)<<8)|(pixels.get(offset+2)&255));}return image;}
}
