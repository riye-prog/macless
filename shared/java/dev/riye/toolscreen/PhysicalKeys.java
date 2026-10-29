package dev.riye.toolscreen;

public final class PhysicalKeys {
    private static final int[] MAC={4,22,7,9,11,10,29,27,6,25,100,5,20,26,8,21,28,23,30,31,32,33,35,34,46,38,36,45,37,39,48,18,24,47,12,19,40,15,13,52,14,51,49,54,56,17,16,55,43,44,53,42,0,41,231,227,225,57,226,224,229,230,228,257,108,99,0,85,0,87,0,83,128,129,127,84,88,0,86,109,110,103,98,89,90,91,92,93,94,95,111,96,97,137,135,133,62,63,64,60,65,66,145,68,144,104,107,105,0,67,101,69,0,106,73,74,75,76,61,77,59,78,58,80,79,81,82,0};
    private static final String SYSTEM=System.getProperty("os.name").toLowerCase(java.util.Locale.ROOT);
    private static final boolean MACOS=SYSTEM.contains("mac")||SYSTEM.contains("darwin");
    private PhysicalKeys(){}
    public static int fromGlfw(int key,int scan){return fromGlfw(key,scan,MACOS);}
    static int fromGlfw(int key,int scan,boolean macos){
        if(macos&&scan>=0&&scan<MAC.length&&MAC[scan]>0)return MAC[scan];
        if(key>=65&&key<=90)return key-65+4;if(key>=49&&key<=57)return key-49+30;if(key==48)return 39;
        if(key>=290&&key<=301)return key-290+58;if(key>=302&&key<=313)return key-302+104;
        if(key>=320&&key<=329)return key==320?98:key-320+88;
        return switch(key){case 161->100;case 162->50;case 32->44;case 39->52;case 44->54;case 45->45;case 46->55;case 47->56;case 59->51;case 61->46;case 91->47;case 92->49;case 93->48;case 96->53;case 256->41;case 257->40;case 258->43;case 259->42;case 260->73;case 261->76;case 262->79;case 263->80;case 264->81;case 265->82;case 266->75;case 267->78;case 268->74;case 269->77;case 280->57;case 281->71;case 282->83;case 283->70;case 284->72;case 330->99;case 331->84;case 332->85;case 333->86;case 334->87;case 335->88;case 336->103;case 340->225;case 341->224;case 342->226;case 343->227;case 344->229;case 345->228;case 346->230;case 347->231;case 348->101;default->0;};
    }
    public static int macScan(int hid){for(int i=0;i<MAC.length;i++)if(MAC[i]==hid)return i;return -1;}
    public static int sdlCode(int hid){
        if(hid>=4&&hid<=29)return 'a'+hid-4;if(hid>=30&&hid<=38)return '1'+hid-30;if(hid==39)return '0';
        return switch(hid){case 40->13;case 41->27;case 42->8;case 43->9;case 44->32;case 45->45;case 46->61;case 47->91;case 48->93;case 49->92;case 51->59;case 52->39;case 53->96;case 54->44;case 55->46;case 56->47;case 76->127;default->1073741824|hid;};
    }
    public static int glfwCode(int hid){
        if(hid>=4&&hid<=29)return 'A'+hid-4;if(hid>=30&&hid<=38)return '1'+hid-30;if(hid==39)return '0';if(hid>=58&&hid<=69)return 290+hid-58;if(hid>=104&&hid<=115)return 302+hid-104;if(hid>=89&&hid<=97)return 321+hid-89;
        return switch(hid){case 100->161;case 50->162;case 40->257;case 41->256;case 42->259;case 43->258;case 44->32;case 45->45;case 46->61;case 47->91;case 48->93;case 49->92;case 51->59;case 52->39;case 53->96;case 54->44;case 55->46;case 56->47;case 57->280;case 70->283;case 71->281;case 72->284;case 73->260;case 74->268;case 75->266;case 76->261;case 77->269;case 78->267;case 79->262;case 80->263;case 81->264;case 82->265;case 83->282;case 84->331;case 85->332;case 86->333;case 87->334;case 88->335;case 98->320;case 99->330;case 101->348;case 103->336;case 224->341;case 225->340;case 226->342;case 227->343;case 228->345;case 229->344;case 230->346;case 231->347;default->-1;};
    }
    public static int sdlModifiers(int glfw){return ((glfw&1)!=0?1:0)|((glfw&2)!=0?64:0)|((glfw&4)!=0?256:0)|((glfw&8)!=0?1024:0);}
}
