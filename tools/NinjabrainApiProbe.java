import com.sun.net.httpserver.HttpServer;
import java.lang.reflect.Proxy;
import java.net.InetSocketAddress;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.concurrent.Executors;
import ninjabrainbot.io.api.ApiV1HttpHandler;
import ninjabrainbot.io.preferences.IPreferenceSource;
import ninjabrainbot.io.preferences.NinjabrainBotPreferences;
import ninjabrainbot.model.datastate.DataState;
import ninjabrainbot.model.domainmodel.DomainModel;
import ninjabrainbot.model.environmentstate.EnvironmentState;
import ninjabrainbot.model.information.InformationMessageList;
import ninjabrainbot.model.datastate.endereye.EnderEyeThrowFactory;
import ninjabrainbot.model.datastate.common.LimitedPlayerPosition;

public final class NinjabrainApiProbe {
    public static void main(String[] arguments)throws Exception{
        IPreferenceSource source=(IPreferenceSource)Proxy.newProxyInstance(IPreferenceSource.class.getClassLoader(),new Class<?>[]{IPreferenceSource.class},(proxy,method,values)->method.getName().startsWith("get")?values[1]:null);
        DomainModel domain=new DomainModel();NinjabrainBotPreferences preferences=new NinjabrainBotPreferences(source);EnvironmentState environment=new EnvironmentState(domain,preferences);DataState data=new DataState(domain,environment);InformationMessageList messages=new InformationMessageList();var executor=Executors.newCachedThreadPool();ApiV1HttpHandler handler=new ApiV1HttpHandler(data,domain,messages,executor);HttpServer server=HttpServer.create(new InetSocketAddress("127.0.0.1",0),0);server.createContext("/api/v1/",handler);server.setExecutor(executor);server.start();domain.finishInitialization();EnderEyeThrowFactory factory=new EnderEyeThrowFactory(preferences,data.boatDataState());domain.acquireWriteLock();try{data.getThrowList().add(factory.createEnderEyeThrowFromLimitedPlayerPosition(new LimitedPlayerPosition(1200,-300,-140.837,0)));data.getThrowList().add(factory.createEnderEyeThrowFromLimitedPlayerPosition(new LimitedPlayerPosition(1250,-300,-144.478,0)));data.playerPosition().set(new LimitedPlayerPosition(1250,-300,-144.478,0));}finally{domain.releaseWriteLock();}Path directory=Path.of(arguments[0]);Files.createDirectories(directory);HttpClient client=HttpClient.newHttpClient();
        for(String endpoint:new String[]{"stronghold","boat","blind","divine","information-messages","version"}){HttpResponse<String> response=client.send(HttpRequest.newBuilder(URI.create("http://127.0.0.1:"+server.getAddress().getPort()+"/api/v1/"+endpoint)).build(),HttpResponse.BodyHandlers.ofString());if(response.statusCode()!=200)throw new IllegalStateException(endpoint+" returned "+response.statusCode());Files.writeString(directory.resolve(endpoint+".json"),response.body());System.out.println("NINJABRAIN_ACTUAL_API_PASS "+endpoint);}
        server.stop(0);handler.dispose();environment.dispose();executor.shutdownNow();System.exit(0);
    }
}
