package dev.riye.toolscreen.mixin;

import dev.riye.toolscreen.NativeBridge;
import dev.riye.toolscreen.Toolscreen;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.components.debugchart.ProfilerPieChart;
import net.minecraft.util.profiling.ProfileResults;
import net.minecraft.util.profiling.ResultField;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(ProfilerPieChart.class)
public abstract class ProfilerMixin {
    @Shadow private ProfileResults profilerPieChartResults;
    @Shadow private String profilerTreePath;

    @Inject(method="setPieChartResults",at=@At("TAIL"))
    private void toolscreenResults(ProfileResults results,CallbackInfo callback){toolscreenPublish();}

    @Inject(method="profilerPieChartKeyPress",at=@At("TAIL"))
    private void toolscreenNavigation(int key,CallbackInfo callback){toolscreenPublish();}

    @Inject(method="extractRenderState",at=@At("HEAD"),cancellable=true)
    private void toolscreenHideVanilla(GuiGraphicsExtractor graphics,int width,int height,CallbackInfo callback){
        if(Toolscreen.ready&&!Toolscreen.failed&&NativeBridge.enabled())callback.cancel();
    }

    @Unique
    private void toolscreenPublish(){
        if(!Toolscreen.ready||Toolscreen.failed)return;
        if(profilerPieChartResults==null){NativeBridge.debugPie("",new String[0],new double[]{0});return;}
        List<ResultField> values=new ArrayList<>(profilerPieChartResults.getTimes(profilerTreePath));
        if(values.isEmpty()){NativeBridge.debugPie("",new String[0],new double[]{0});return;}
        ResultField current=values.remove(0);int count=Math.min(values.size(),32);
        String[] names=new String[count];double[] numbers=new double[1+count*3];numbers[0]=current.globalPercentage;
        for(int i=0;i<count;i++){ResultField value=values.get(i);names[i]=value.name;numbers[1+i*3]=value.percentage;numbers[2+i*3]=value.globalPercentage;numbers[3+i*3]=value.getColor();}
        NativeBridge.debugPie(profilerTreePath.replace('\u001e','/'),names,numbers);
    }
}
