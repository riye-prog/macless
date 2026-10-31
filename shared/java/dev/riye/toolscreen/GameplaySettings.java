package dev.riye.toolscreen;

public final class GameplaySettings {
    private GameplaySettings(){}
    public static void initialize(){
        String previous=NativeBridge.option("sensitivity_scope");if(previous.equals("unified-eye-v1"))return;
        double[] values=NativeBridge.snapshot();boolean enabled=values[38]>.5;double normal=values[39];
        if(!previous.equals("all-gameplay")){boolean customized=Math.abs(values[39]-.15)>.00000001||Math.abs(values[40]-.15)>.00000001;if(!enabled&&!customized)normal=1;enabled=enabled||customized;}
        NativeBridge.set(39,normal);NativeBridge.set(38,enabled?1:0);NativeBridge.set(145,1);NativeBridge.setOption("sensitivity_scope","unified-eye-v1");NativeBridge.save();
    }
}
