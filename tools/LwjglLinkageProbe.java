import java.io.DataInputStream;
import java.lang.invoke.MethodType;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.Set;
import java.util.zip.ZipFile;
import org.lwjgl.system.MemoryUtil;

public final class LwjglLinkageProbe {
    private static final Set<String> methods=new HashSet<>();
    private static final Set<String> fields=new HashSet<>();
    public static void main(String[] arguments)throws Exception{
        try(ZipFile jar=new ZipFile(arguments[0])){var entries=jar.entries();while(entries.hasMoreElements()){var entry=entries.nextElement();if(!entry.getName().endsWith(".class"))continue;try(DataInputStream input=new DataInputStream(jar.getInputStream(entry))){if(input.readInt()!=0xcafebabe)throw new IllegalStateException("Invalid class");input.readUnsignedShort();input.readUnsignedShort();int count=input.readUnsignedShort();Object[] pool=new Object[count];int[] tags=new int[count];for(int i=1;i<count;i++){int tag=tags[i]=input.readUnsignedByte();switch(tag){case 1->pool[i]=input.readUTF();case 3,4->input.skipNBytes(4);case 5,6->{input.skipNBytes(8);i++;}case 7,8,16,19,20->pool[i]=input.readUnsignedShort();case 9,10,11,12,17,18->pool[i]=new int[]{input.readUnsignedShort(),input.readUnsignedShort()};case 15->input.skipNBytes(3);default->throw new IllegalStateException("Unknown constant "+tag);}}for(int i=1;i<count;i++){if(tags[i]<9||tags[i]>11)continue;int[] reference=(int[])pool[i];String owner=(String)pool[(int)pool[reference[0]]];if(!owner.startsWith("org/lwjgl/"))continue;int[] nameType=(int[])pool[reference[1]];String name=(String)pool[nameType[0]],descriptor=(String)pool[nameType[1]];Class<?> type=Class.forName(owner.replace('/','.'),false,LwjglLinkageProbe.class.getClassLoader());String key=owner+"."+name+descriptor;if(tags[i]==9){if(!fields.add(key))continue;Class<?> expected=MethodType.fromMethodDescriptorString("()"+descriptor,type.getClassLoader()).returnType();if(type.getField(name).getType()!=expected)throw new NoSuchFieldException(key);}else{if(!methods.add(key))continue;MethodType expected=MethodType.fromMethodDescriptorString(descriptor,type.getClassLoader());Method method=type.getMethod(name,expected.parameterArray());if(method.getReturnType()!=expected.returnType())throw new NoSuchMethodException(key);}}}}}
        Class<?> memory=Class.forName("dev.riye.toolscreen.LegacyMemory");Method free=memory.getMethod("free",ByteBuffer.class);for(int i=0;i<256;i++){ByteBuffer buffer=MemoryUtil.memAlloc(256+i);buffer.putInt(0,0x12345678);buffer.position(i%128);free.invoke(null,buffer);}free.invoke(null,new Object[]{null});System.out.println("TOOLSCREEN_LWJGL_LINKAGE_PASS methods="+methods.size()+" fields="+fields.size()+" runtime="+org.lwjgl.Version.getVersion());System.out.println("TOOLSCREEN_LWJGL_ALLOCATOR_PASS allocations=256");
    }
}
