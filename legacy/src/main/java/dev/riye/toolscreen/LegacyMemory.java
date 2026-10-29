package dev.riye.toolscreen;

import java.nio.Buffer;
import java.nio.ByteBuffer;
import org.lwjgl.system.MemoryUtil;

public final class LegacyMemory {
    private LegacyMemory(){}
    public static void free(ByteBuffer buffer){MemoryUtil.memFree((Buffer)buffer);}
}
