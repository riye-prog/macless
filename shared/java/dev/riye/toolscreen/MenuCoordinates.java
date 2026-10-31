package dev.riye.toolscreen;

public record MenuCoordinates(int displayWidth,int displayHeight,int sourceWidth,int sourceHeight,int guiWidth,int guiHeight) {
    public double offsetX(){return displayWidth/2-sourceWidth/2;}
    public double offsetY(){return displayHeight/2-sourceHeight/2;}
    public double x(double scaled){return deltaX(scaled)-offsetX()*guiWidth/sourceWidth;}
    public double y(double scaled){return deltaY(scaled)-offsetY()*guiHeight/sourceHeight;}
    public double deltaX(double scaled){return scaled*displayWidth/sourceWidth;}
    public double deltaY(double scaled){return scaled*displayHeight/sourceHeight;}
    public double rawX(double value,int windowWidth){return x(value*guiWidth/windowWidth);}
    public double rawY(double value,int windowHeight){return y(value*guiHeight/windowHeight);}
    public double windowX(double value,int windowWidth){return rawX(value,windowWidth)*windowWidth/guiWidth;}
    public double windowY(double value,int windowHeight){return rawY(value,windowHeight)*windowHeight/guiHeight;}
}
