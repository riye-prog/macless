package dev.riye.toolscreen;

public final class BoatEyeMouse {
    public static final double GODSENS=0.02291165;
    public static final double AIM_RATE=.15;
    private double remainderX;
    private double remainderY;
    private double directionX;
    private double directionY;
    private double lastRate=Double.NaN;
    private double lastSensitivity=Double.NaN;
    private boolean lastEyeZoom;

    public record Motion(double sensitivity,double x,double y,boolean precision){}

    public static double gain(double sensitivity){
        double base=sensitivity*(double).6f+(double).2f;
        return base*base*base*8;
    }

    public Motion prepare(double sensitivity,double multiplier,double x,double y,boolean enabled,boolean eyeZoom){
        if(!Double.isFinite(sensitivity)||!Double.isFinite(multiplier)||!Double.isFinite(x)||!Double.isFinite(y)){
            reset();return new Motion(Double.isFinite(sensitivity)?sensitivity:.5,0,0,false);
        }
        if(!enabled){reset();return new Motion(sensitivity,x*multiplier,y*multiplier,false);}
        double rate=eyeZoom?AIM_RATE:gain(sensitivity)*multiplier/gain(GODSENS);
        if(rate!=lastRate||sensitivity!=lastSensitivity||eyeZoom!=lastEyeZoom){reset();lastRate=rate;lastSensitivity=sensitivity;lastEyeZoom=eyeZoom;}
        if(x!=0){double direction=Math.signum(x);if(directionX!=0&&direction!=directionX)remainderX=0;directionX=direction;}
        if(y!=0){double direction=Math.signum(y);if(directionY!=0&&direction!=directionY)remainderY=0;directionY=direction;}
        remainderX+=x*rate;remainderY+=y*rate;
        double outputX=x==0?0:Math.copySign(Math.floor(Math.abs(remainderX)+.5),remainderX);
        double outputY=y==0?0:Math.copySign(Math.floor(Math.abs(remainderY)+.5),remainderY);
        remainderX-=outputX;remainderY-=outputY;
        return new Motion(GODSENS,outputX,outputY,true);
    }

    public void reset(){remainderX=0;remainderY=0;directionX=0;directionY=0;lastRate=Double.NaN;lastSensitivity=Double.NaN;lastEyeZoom=false;}
}
