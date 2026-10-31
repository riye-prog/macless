package dev.riye.toolscreen;

public final class MenuCoordinatesTest {
    private static int checks;
    private static void equal(double actual,double expected){if(Math.abs(actual-expected)>1e-8)throw new AssertionError(actual+" != "+expected);checks++;}

    public static void main(String[] arguments){
        for(int[] display:new int[][]{{1920,1080},{2880,1800},{1921,1081}}){
            for(int[] source:new int[][]{display,{384,1080},{1920,540},{384,16384},{2560,1440}}){
                for(int scale:new int[]{1,2,3,4}){
                    var menu=new MenuCoordinates(display[0],display[1],source[0],source[1],(source[0]+scale-1)/scale,(source[1]+scale-1)/scale);
                    for(int retina:new int[]{1,2}){
                        int windowWidth=display[0]/retina,windowHeight=display[1]/retina;
                        for(double fraction:new double[]{0,.1,.5,.9,1}){
                            double pixelX=menu.offsetX()+source[0]*fraction,pixelY=menu.offsetY()+source[1]*fraction;
                            double x=pixelX*windowWidth/display[0],y=pixelY*windowHeight/display[1];
                            equal(menu.rawX(x,windowWidth),fraction*menu.guiWidth());equal(menu.rawY(y,windowHeight),fraction*menu.guiHeight());
                            equal(menu.windowX(x,windowWidth)*menu.guiWidth()/windowWidth,fraction*menu.guiWidth());equal(menu.windowY(y,windowHeight)*menu.guiHeight()/windowHeight,fraction*menu.guiHeight());
                            equal(menu.deltaX(3.5*menu.guiWidth()/windowWidth),3.5*display[0]*menu.guiWidth()/windowWidth/source[0]);
                        }
                    }
                }
            }
        }
        var thin=new MenuCoordinates(1920,1080,384,1080,384,1080);
        equal(thin.rawX(0,1920),-768);equal(thin.rawX(1920,1920),1152);
        System.out.println("MACLESS_MENU_COORDINATES_PASS checks="+checks);
    }
}
