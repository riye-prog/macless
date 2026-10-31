package dev.riye.toolscreen;

public final class BoatEyeContractTest {
    private static int checks;
    private static void check(boolean value,String message){if(!value)throw new AssertionError(message);checks++;}

    public static void main(String[] arguments){
        BoatEyeMouse mouse=new BoatEyeMouse();
        double totalX=0,totalY=0;
        for(int i=0;i<1000;i++){
            var motion=mouse.prepare(.5,20,1,-1,true,true);
            check(motion.sensitivity()==.02291165&&motion.precision(),"uses exact godsens");
            check(motion.x()==Math.rint(motion.x())&&motion.y()==Math.rint(motion.y()),"whole mouse counts");
            totalX+=motion.x();totalY+=motion.y();
        }
        check(totalX==150&&totalY==-150,"slow rate retains small movements on both axes");
        mouse.reset();mouse.prepare(.5,1,10,-10,true,true);
        for(int i=0;i<20;i++){
            var motion=mouse.prepare(.5,1,0,0,true,true);
            check(motion.x()==0&&motion.y()==0,"stationary mouse cannot release a rounding remainder");
        }
        for(double sensitivity:new double[]{0,.02291165,.13,.5,.91,1}){
            for(double multiplier:new double[]{.1,1,1.5,20}){
                mouse.reset();double counts=0;
                for(int i=0;i<1000;i++)counts+=mouse.prepare(sensitivity,multiplier,1,0,true,false).x();
                double target=1000*BoatEyeMouse.gain(sensitivity)*multiplier;
                check(Math.abs(counts*BoatEyeMouse.gain(BoatEyeMouse.GODSENS)-target)<=BoatEyeMouse.gain(BoatEyeMouse.GODSENS)*.500001,"preserves normal turn rate without leaving angle grid");
            }
        }
        mouse.reset();mouse.prepare(.5,1,3,0,true,true);
        check(mouse.prepare(.5,1,-1,0,true,true).x()==0,"direction change clears stale movement");
        check(mouse.prepare(.5,1,0,0,true,false).x()==0,"leaving EyeZoom clears remainder");
        check(mouse.prepare(.5,1,1,0,true,true).x()==0,"entering EyeZoom clears remainder");
        var disabled=mouse.prepare(.37,1.5,8,4,false,true);
        check(!disabled.precision()&&disabled.sensitivity()==.37&&disabled.x()==12&&disabled.y()==6,"disabled feature retains normal input");
        check(mouse.prepare(.5,1,Double.NaN,1,true,true).x()==0,"invalid input cannot move camera");
        double increment=BoatEyeMouse.gain(BoatEyeMouse.GODSENS)*.15D;
        for(boolean modern:new boolean[]{false,true}){
            float angle=0;
            for(int i=0;i<4096;i++){
                int count=(i*37)%31-15;
                double delta=count*BoatEyeMouse.gain(BoatEyeMouse.GODSENS);
                angle+=modern?(float)delta*.15F:(float)(delta*.15D);
                double clipboardAngle=Math.round(angle*100.0)/100.0;
                float decoded=(float)(Math.round(clipboardAngle/increment)*increment);
                check(modern?Math.abs(decoded-angle)<=Math.ulp(Math.max(1,Math.abs(angle))):decoded==angle,"Ninjabrain recovers camera steps within vanilla float precision");
            }
        }
        System.out.println("MACLESS_BOAT_EYE_PASS checks="+checks+" godsens="+BoatEyeMouse.GODSENS+" aimRate="+BoatEyeMouse.AIM_RATE);
    }
}
