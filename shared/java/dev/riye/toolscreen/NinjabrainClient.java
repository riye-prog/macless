package dev.riye.toolscreen;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

public final class NinjabrainClient {
    private static ScheduledExecutorService executor;
    private static long nextPoll;
    private static volatile boolean running;
    private static String lastError="";

    private NinjabrainClient(){}

    public static synchronized void start(){
        if(executor!=null)return;
        running=true;executor=Executors.newSingleThreadScheduledExecutor(task->{Thread thread=new Thread(task,"Macless Ninjabrain API");thread.setDaemon(true);return thread;});
        executor.scheduleWithFixedDelay(NinjabrainClient::poll,0,100,TimeUnit.MILLISECONDS);
    }

    private static void poll(){
        try{
            double[] settings=NativeBridge.snapshot();
            if(settings[134]<.5){report("Ninjabrain integration is disabled");return;}
            long now=System.nanoTime();if(now<nextPoll)return;nextPoll=now+(long)settings[143]*1_000_000;
            String base=NativeBridge.option("ninja_url").trim();while(base.endsWith("/"))base=base.substring(0,base.length()-1);
            URI address=URI.create(base);String host=address.getHost();
            if(host==null||!(host.equalsIgnoreCase("localhost")||host.equals("127.0.0.1")||host.equals("::1")||host.equals("[::1]"))||!(address.getScheme().equals("http")||address.getScheme().equals("https"))){report("Use Ninjabrain Bot's local HTTP API address");return;}
            String stronghold=get(base,"stronghold",true);
            String payload="{\"stronghold\":"+stronghold+",\"boat\":"+get(base,"boat",false)+",\"blind\":"+get(base,"blind",false)+",\"divine\":"+get(base,"divine",false)+",\"messages\":"+get(base,"information-messages",false)+",\"version\":"+get(base,"version",false)+"}";
            if(running){NativeBridge.ninjaUpdate(payload,"");lastError="";}
        }catch(Exception failure){report("Ninjabrain Bot is offline. Start it and enable its local API.");}
    }

    private static String get(String base,String endpoint,boolean required)throws IOException{
        HttpURLConnection connection=(HttpURLConnection)URI.create(base+"/api/v1/"+endpoint).toURL().openConnection();
        connection.setConnectTimeout(350);connection.setReadTimeout(600);connection.setRequestProperty("Accept","application/json");
        try{
            if(connection.getResponseCode()!=200){if(required)throw new IOException("Ninjabrain API returned "+connection.getResponseCode());return "{}";}
            try(InputStream input=connection.getInputStream();ByteArrayOutputStream result=new ByteArrayOutputStream()){
                byte[] buffer=new byte[4096];int count;while((count=input.read(buffer))!=-1){if(result.size()+count>65536)throw new IOException("Ninjabrain response exceeded the size limit");result.write(buffer,0,count);}return result.toString(StandardCharsets.UTF_8);
            }
        }finally{connection.disconnect();}
    }

    private static void report(String error){if(running&&!lastError.equals(error)){lastError=error;NativeBridge.ninjaUpdate("",error);}}

    public static synchronized void close(){running=false;if(executor!=null){executor.shutdownNow();executor=null;}nextPoll=0;}
}
