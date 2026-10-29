package dev.riye.toolscreen;

import com.sun.net.httpserver.HttpServer;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executors;
import java.awt.image.BufferedImage;
import java.nio.file.Files;
import java.nio.file.Path;
import javax.imageio.ImageIO;

public final class VerificationFixtures {
    private static HttpServer server;
    private VerificationFixtures(){}
    public static void start(){
        if(server!=null)return;
        try{
            server=HttpServer.create(new InetSocketAddress("127.0.0.1",0),0);
            server.setExecutor(Executors.newCachedThreadPool(task->{Thread thread=new Thread(task,"Macless API fixture");thread.setDaemon(true);return thread;}));
            server.createContext("/api/v1/",exchange->{String endpoint=exchange.getRequestURI().getPath();String body=endpoint.endsWith("stronghold")?"{\"resultType\":\"TRIANGULATION\",\"playerPosition\":{\"xInOverworld\":1200,\"zInOverworld\":-300,\"horizontalAngle\":-40},\"predictions\":[{\"chunkX\":100,\"chunkZ\":-50,\"certainty\":0.987,\"overworldDistance\":640},{\"chunkX\":101,\"chunkZ\":-51,\"certainty\":0.013,\"overworldDistance\":665}],\"eyeThrows\":[{\"xInOverworld\":1200,\"zInOverworld\":-300,\"angle\":-40,\"error\":0.002,\"correction\":0,\"type\":\"NORMAL\"}]}":endpoint.endsWith("boat")?"{\"boatState\":\"VALID\",\"boatAngle\":-40.078125}":endpoint.endsWith("blind")?"{\"isBlindModeEnabled\":false}":endpoint.endsWith("divine")?"{\"isDivineModeEnabled\":false}":endpoint.endsWith("version")?"{\"version\":\"fixture-v1\"}":"{\"informationMessages\":[]}";byte[] bytes=body.getBytes(StandardCharsets.UTF_8);exchange.getResponseHeaders().set("Content-Type","application/json");exchange.sendResponseHeaders(200,bytes.length);try(var output=exchange.getResponseBody()){output.write(bytes);}});
            server.start();NativeBridge.setOption("ninja_url","http://127.0.0.1:"+server.getAddress().getPort());
        }catch(Exception failure){throw new IllegalStateException(failure);}
    }
    public static void close(){if(server!=null){server.stop(0);server=null;}}
    public static int[] createMedia(){try{Path directory=Path.of("verification").toAbsolutePath();Files.createDirectories(directory);Path html=directory.resolve("routed-browser.html");Files.writeString(html,"<!doctype html><html><style>html,body{margin:0;width:100%;height:100%}header{position:absolute;inset:0 0 50%;background:#ff4038}footer{position:absolute;inset:50% 0 0;background:#3879ff}</style><header></header><footer></footer></html>");int browser=NativeBridge.addAsset("browser:"+html.toUri());NativeBridge.editAsset(browser,14,1);NativeBridge.editAsset(browser,32,1);Path png=directory.resolve("routed-image.png");BufferedImage image=new BufferedImage(240,100,BufferedImage.TYPE_INT_ARGB);for(int y=0;y<100;y++)for(int x=0;x<240;x++)image.setRGB(x,y,y<50?0xff50d4a4:0xfff0b670);ImageIO.write(image,"PNG",png.toFile());int source=NativeBridge.addAsset(png.toString());for(double[] edit:new double[][]{{5,.65},{6,.60},{7,.3},{8,.2},{14,65535},{26,1},{27,80.0/255},{28,212.0/255},{29,164.0/255},{30,1},{11,.01},{32,2}})NativeBridge.editAsset(source,(int)edit[0],edit[1]);return new int[]{browser,source};}catch(Exception failure){throw new IllegalStateException(failure);}}
}
