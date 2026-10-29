package dev.riye.toolscreen;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.net.http.WebSocket;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.Base64;
import java.util.Comparator;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import javax.imageio.ImageIO;

final class BrowserSource extends DesktopSource {
    private static final Gson JSON=new Gson();
    private static final HttpClient HTTP=HttpClient.newBuilder().connectTimeout(Duration.ofSeconds(4)).build();
    private final String executable,url;
    private final ConcurrentHashMap<Integer,CompletableFuture<JsonObject>> pending=new ConcurrentHashMap<>();
    private final AtomicInteger sequence=new AtomicInteger();
    private volatile WebSocket socket;
    private Path profile;

    BrowserSource(String executable,String url){this.executable=executable;this.url=url;}

    void runSource()throws Exception{
        String scheme=URI.create(url).getScheme();if(!"https".equals(scheme)&&!"http".equals(scheme)&&!"file".equals(scheme))throw new IOException("Use an http, https or file URL");if(executable.isBlank())throw new IOException("Install Chrome, Edge or Chromium, or set Browser executable");
        profile=Files.createTempDirectory("macless-browser-");Path log=profile.resolve("browser.log");
        try{
            process=new ProcessBuilder(executable,"--headless=new","--remote-debugging-port=0","--remote-debugging-address=127.0.0.1","--user-data-dir="+profile,"--window-size=960,540","--force-device-scale-factor=1","--hide-scrollbars","--no-first-run","--no-default-browser-check","--disable-extensions","--disable-sync","--disable-background-networking","--password-store=basic","about:blank").redirectOutput(log.toFile()).redirectErrorStream(true).start();
            Path endpoint=profile.resolve("DevToolsActivePort");long deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(20);while(!Files.exists(endpoint)&&process.isAlive()&&!stopped&&System.nanoTime()<deadline)Thread.sleep(50);if(stopped)return;if(!Files.exists(endpoint))throw new IOException("Browser did not start. Check the Browser executable setting");
            int port=Integer.parseInt(Files.readAllLines(endpoint).get(0));String response=HTTP.send(HttpRequest.newBuilder(URI.create("http://127.0.0.1:"+port+"/json/new?about:blank")).timeout(Duration.ofSeconds(4)).PUT(HttpRequest.BodyPublishers.noBody()).build(),HttpResponse.BodyHandlers.ofString()).body();JsonObject target=JSON.fromJson(response,JsonObject.class);if(!target.has("webSocketDebuggerUrl"))throw new IOException("Browser page unavailable");String address=target.get("webSocketDebuggerUrl").getAsString();
            socket=HTTP.newWebSocketBuilder().connectTimeout(Duration.ofSeconds(4)).buildAsync(URI.create(address),new WebSocket.Listener(){
                private final StringBuilder text=new StringBuilder();
                public void onOpen(WebSocket value){value.request(1);}
                public CompletionStage<?> onText(WebSocket value,CharSequence data,boolean last){if(text.length()+data.length()>16777216){value.abort();return null;}text.append(data);if(last){try{JsonObject message=JSON.fromJson(text.toString(),JsonObject.class);if(message.has("id")){CompletableFuture<JsonObject> future=pending.remove(message.get("id").getAsInt());if(future!=null){if(message.has("error"))future.completeExceptionally(new IOException(message.get("error").toString()));else future.complete(message.getAsJsonObject("result"));}}}catch(RuntimeException failure){failPending(failure);}text.setLength(0);}value.request(1);return null;}
                public void onError(WebSocket value,Throwable failure){failPending(failure);}
                public CompletionStage<?> onClose(WebSocket value,int status,String reason){failPending(new IOException("Browser connection closed"));return null;}
            }).get(5,TimeUnit.SECONDS);
            command("Page.enable",Map.of());command("Emulation.setDeviceMetricsOverride",Map.of("width",960,"height",540,"deviceScaleFactor",1,"mobile",false));command("Emulation.setDefaultBackgroundColorOverride",Map.of("color",Map.of("r",0,"g",0,"b",0,"a",0)));command("Page.navigate",Map.of("url",url));
            int retries=0;
            while(!stopped&&process.isAlive()){
                long started=System.nanoTime();JsonObject result;try{result=command("Page.captureScreenshot",Map.of("format","png","fromSurface",true,"captureBeyondViewport",false,"optimizeForSpeed",true));retries=0;}catch(Exception failure){if(++retries>30)throw failure;Thread.sleep(100);continue;}byte[] png=Base64.getDecoder().decode(result.get("data").getAsString());BufferedImage image=ImageIO.read(new ByteArrayInputStream(png));if(image!=null)store(image);long remaining=TimeUnit.MILLISECONDS.toNanos(67)-(System.nanoTime()-started);if(remaining>0)TimeUnit.NANOSECONDS.sleep(remaining);
            }
        }finally{
            Thread.interrupted();if(socket!=null)socket.abort();if(process!=null){process.destroy();try{if(!process.waitFor(3,TimeUnit.SECONDS))process.destroyForcibly();}catch(InterruptedException interrupted){process.destroyForcibly();Thread.interrupted();}}if(profile!=null)try(var paths=Files.walk(profile)){for(Path path:paths.sorted(Comparator.reverseOrder()).toList())try{Files.deleteIfExists(path);}catch(IOException ignored){}}
        }
    }

    private JsonObject command(String method,Map<String,?> parameters)throws Exception{
        int id=sequence.incrementAndGet();JsonObject message=new JsonObject();message.addProperty("id",id);message.addProperty("method",method);message.add("params",JSON.toJsonTree(parameters));CompletableFuture<JsonObject> response=new CompletableFuture<>();pending.put(id,response);try{socket.sendText(JSON.toJson(message),true).get(5,TimeUnit.SECONDS);return response.get(8,TimeUnit.SECONDS);}catch(Exception failure){throw new IOException(method+": "+failure.getMessage(),failure);}finally{pending.remove(id);}
    }
    private void failPending(Throwable failure){for(var future:pending.values())future.completeExceptionally(failure);pending.clear();}
    public void close(){super.close();if(socket!=null)socket.abort();failPending(new IOException("Source closed"));}
}
