#include "core.hpp"
#include "ui.hpp"
#include "platform.hpp"
#include "features.hpp"
#include "input.hpp"
#include <jni.h>
#include <cstring>

namespace {
std::string javaString(JNIEnv* env,jstring value){if(!value)return "";const char* text=env->GetStringUTFChars(value,nullptr);if(!text)return "";std::string result(text);env->ReleaseStringUTFChars(value,text);return result;}
}

extern "C" {
JNIEXPORT jint JNICALL Java_dev_riye_toolscreen_NativeBridge_addSelectedImage(JNIEnv* env,jclass,jstring path,jboolean background){return background?toolscreen::addBackground(javaString(env,path)):toolscreen::addImage(javaString(env,path));}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_runtimeContext(JNIEnv*,jclass,jboolean inWorld,jdouble baseSensitivity){toolscreen::runtimeContext(inWorld,baseSensitivity);}
JNIEXPORT jlong JNICALL Java_dev_riye_toolscreen_NativeBridge_fontRevision(JNIEnv*,jclass){return toolscreen::fontRevision();}
int ts_ui_probe(int control,float* output);
JNIEXPORT jfloatArray JNICALL Java_dev_riye_toolscreen_NativeBridge_uiProbe(JNIEnv* env,jclass,jint control){float values[11];ts_ui_probe(control,values);auto result=env->NewFloatArray(11);if(result)env->SetFloatArrayRegion(result,0,11,values);return result;}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_uiDisplayScale(JNIEnv*,jclass,jfloat value){toolscreen::uiDisplayScale(value);}
JNIEXPORT jboolean JNICALL Java_dev_riye_toolscreen_NativeBridge_uiWantsText(JNIEnv*,jclass){return toolscreen::uiWantsText();}
JNIEXPORT jboolean JNICALL Java_dev_riye_toolscreen_NativeBridge_uiEscapeActive(JNIEnv*,jclass){return toolscreen::uiEscapeActive();}
JNIEXPORT jboolean JNICALL Java_dev_riye_toolscreen_NativeBridge_enabled(JNIEnv*,jclass){return toolscreen::snapshot()[128]>.5;}
JNIEXPORT jdoubleArray JNICALL Java_dev_riye_toolscreen_NativeBridge_debugProbe(JNIEnv* env,jclass){auto debug=toolscreen::debugState();auto ninja=toolscreen::ninjaState();toolscreen::ProfileRow row;bool mapless=toolscreen::maplessRow(row);double data[10]={debug.counter.empty()?0.0:1.0,double(debug.rows.size()),mapless?1.0:0.0,mapless?row.global:0.0,double(ninja.predictions.size()),ninja.connected?1.0:0.0,ninja.predictions.empty()?0:ninja.predictions[0].x,ninja.predictions.empty()?0:ninja.predictions[0].z,ninja.predictions.empty()?0:ninja.predictions[0].certainty,ninja.predictions.empty()?0:ninja.predictions[0].correction};auto result=env->NewDoubleArray(10);if(result)env->SetDoubleArrayRegion(result,0,10,data);return result;}
JNIEXPORT jbyteArray JNICALL Java_dev_riye_toolscreen_NativeBridge_overlayFrameForOutput(JNIEnv* env,jclass,jint width,jint height,jint viewWidth,jint viewHeight,jint route){auto data=toolscreen::overlayFrame(width,height,viewWidth,viewHeight,route);auto result=env->NewByteArray(jsize(data.size()));if(result)env->SetByteArrayRegion(result,0,jsize(data.size()),reinterpret_cast<const jbyte*>(data.data()));return result;}
JNIEXPORT jfloatArray JNICALL Java_dev_riye_toolscreen_NativeBridge_planForOutput(JNIEnv* env,jclass,jint width,jint height,jint viewWidth,jint viewHeight,jboolean suspended,jdouble seconds,jint route){auto layers=toolscreen::planForOutput(width,height,viewWidth,viewHeight,suspended,seconds,route);std::vector<float> data;for(auto& layer:layers)data.insert(data.end(),layer.begin(),layer.end());auto result=env->NewFloatArray(jsize(data.size()));if(result)env->SetFloatArrayRegion(result,0,jsize(data.size()),data.data());return result;}
JNIEXPORT jint JNICALL Java_dev_riye_toolscreen_NativeBridge_inputEvent(JNIEnv*,jclass,jint scan,jint modifiers,jint action,jboolean gameplay,jboolean editorAllowed){return toolscreen::inputEvent(scan,modifiers,action,gameplay,editorAllowed);}
JNIEXPORT jstring JNICALL Java_dev_riye_toolscreen_NativeBridge_option(JNIEnv* env,jclass,jstring name){return env->NewStringUTF(toolscreen::option(javaString(env,name)).c_str());}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_setOption(JNIEnv* env,jclass,jstring name,jstring value){toolscreen::setOption(javaString(env,name),javaString(env,value));}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_debugClear(JNIEnv*,jclass){toolscreen::clearDebug();}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_debugCounter(JNIEnv* env,jclass,jstring value){toolscreen::updateCounter(javaString(env,value));}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_debugPie(JNIEnv* env,jclass,jstring path,jobjectArray names,jdoubleArray data){int count=env->GetArrayLength(names);if(count>32||env->GetArrayLength(data)!=1+count*3)return;std::vector<double> numbers(size_t(1+count*3));env->GetDoubleArrayRegion(data,0,jint(numbers.size()),numbers.data());std::vector<toolscreen::ProfileRow> rows;for(int i=0;i<count;++i){auto name=static_cast<jstring>(env->GetObjectArrayElement(names,i));rows.push_back({javaString(env,name),numbers[size_t(1+i*3)],numbers[size_t(2+i*3)],toolscreen::profileColor(numbers[size_t(3+i*3)])});env->DeleteLocalRef(name);}toolscreen::updatePie(javaString(env,path),numbers[0],rows);}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_ninjaUpdate(JNIEnv* env,jclass,jstring data,jstring error){toolscreen::ninjaUpdate(javaString(env,data),javaString(env,error));}
JNIEXPORT jboolean JNICALL Java_dev_riye_toolscreen_NativeBridge_uiPhysicalKey(JNIEnv*,jclass,jint scan,jint keycode,jint modifiers,jboolean down){return toolscreen::uiPhysicalKey(scan,keycode,modifiers,down);}
JNIEXPORT jboolean JNICALL Java_dev_riye_toolscreen_NativeBridge_initialize(JNIEnv* env, jclass, jstring path) {
    toolscreen::initializePlatform(env);
    const char* utf = env->GetStringUTFChars(path, nullptr);
    if (!utf) return false;
    bool result = toolscreen::load(utf);
    env->ReleaseStringUTFChars(path, utf);
    return result;
}
JNIEXPORT jdoubleArray JNICALL Java_dev_riye_toolscreen_NativeBridge_snapshot(JNIEnv* env, jclass) {
    auto state = toolscreen::snapshot();
    auto output = env->NewDoubleArray(toolscreen::settingCount);
    if (output) env->SetDoubleArrayRegion(output, 0, toolscreen::settingCount, state.data());
    return output;
}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_set(JNIEnv*, jclass, jint index, jdouble value) {
    toolscreen::set(index, value);
}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_reset(JNIEnv*, jclass) { toolscreen::reset(); }
JNIEXPORT jboolean JNICALL Java_dev_riye_toolscreen_NativeBridge_save(JNIEnv*, jclass) { return toolscreen::save(); }
JNIEXPORT jintArray JNICALL Java_dev_riye_toolscreen_NativeBridge_viewport(JNIEnv* env, jclass, jint width, jint height, jint limit, jboolean suspended) {
    auto size = toolscreen::viewport(width, height, limit, suspended);
    auto output = env->NewIntArray(2);
    if (output) {jint values[]={jint(size[0]),jint(size[1])};env->SetIntArrayRegion(output, 0, 2, values);}
    return output;
}
JNIEXPORT jfloatArray JNICALL Java_dev_riye_toolscreen_NativeBridge_plan(JNIEnv* env, jclass, jint width, jint height, jint viewWidth, jint viewHeight, jboolean suspended, jdouble seconds) {
    auto quads = toolscreen::plan(width, height, viewWidth, viewHeight, suspended, seconds);
    auto output = env->NewFloatArray(jsize(quads.size() * 20));
    if (!output) return nullptr;
    for (size_t i = 0; i < quads.size(); ++i) env->SetFloatArrayRegion(output, jsize(i * 20), 20, quads[i].data());
    return output;
}
JNIEXPORT jbyteArray JNICALL Java_dev_riye_toolscreen_NativeBridge_uiFrame(JNIEnv* env, jclass, jint width, jint height, jfloat delta, jstring backend, jstring error) {
    const char* backendText = env->GetStringUTFChars(backend, nullptr);
    if (!backendText) return nullptr;
    const char* errorText = env->GetStringUTFChars(error, nullptr);
    if (!errorText) {
        env->ReleaseStringUTFChars(backend, backendText);
        return nullptr;
    }
    auto data = toolscreen::uiFrame(width, height, delta, backendText, errorText);
    env->ReleaseStringUTFChars(backend, backendText);
    env->ReleaseStringUTFChars(error, errorText);
    auto output = env->NewByteArray(jsize(data.size()));
    if (output) env->SetByteArrayRegion(output, 0, jsize(data.size()), reinterpret_cast<jbyte*>(data.data()));
    return output;
}
JNIEXPORT jbyteArray JNICALL Java_dev_riye_toolscreen_NativeBridge_fontAtlas(JNIEnv* env, jclass) {
    auto data = toolscreen::fontAtlas();
    auto output = env->NewByteArray(jsize(data.size()));
    if (output) env->SetByteArrayRegion(output, 0, jsize(data.size()), reinterpret_cast<jbyte*>(data.data()));
    return output;
}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_uiMouse(JNIEnv*, jclass, jfloat x, jfloat y) { toolscreen::uiMouse(x, y); }
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_uiButton(JNIEnv*, jclass, jint button, jboolean down) { toolscreen::uiButton(button, down); }
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_uiScroll(JNIEnv*, jclass, jfloat x, jfloat y) { toolscreen::uiScroll(x, y); }
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_uiKey(JNIEnv*, jclass, jint keycode, jint modifiers, jboolean down) { toolscreen::uiKey(keycode, modifiers, down); }
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_uiText(JNIEnv*, jclass, jint codepoint) { toolscreen::uiText(uint32_t(codepoint)); }
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_uiFocus(JNIEnv*, jclass, jboolean focused) { toolscreen::uiFocus(focused); }
JNIEXPORT jboolean JNICALL Java_dev_riye_toolscreen_NativeBridge_uiCloseRequested(JNIEnv*, jclass) { return toolscreen::uiCloseRequested(); }
JNIEXPORT jboolean JNICALL Java_dev_riye_toolscreen_NativeBridge_uiBindingActive(JNIEnv*,jclass){return toolscreen::uiBindingActive();}
JNIEXPORT jint JNICALL Java_dev_riye_toolscreen_NativeBridge_keyEvent(JNIEnv*,jclass,jint keycode,jint modifiers,jint action,jboolean panel){return toolscreen::keyEvent(keycode,modifiers,action,panel);}
JNIEXPORT jdoubleArray JNICALL Java_dev_riye_toolscreen_NativeBridge_sensitivity(JNIEnv* env,jclass){auto value=toolscreen::sensitivity();auto result=env->NewDoubleArray(2);if(result)env->SetDoubleArrayRegion(result,0,2,value.data());return result;}
JNIEXPORT jbyteArray JNICALL Java_dev_riye_toolscreen_NativeBridge_overlayFrame(JNIEnv* env,jclass,jint width,jint height,jint viewWidth,jint viewHeight){auto data=toolscreen::overlayFrame(width,height,viewWidth,viewHeight);auto result=env->NewByteArray(jsize(data.size()));if(result)env->SetByteArrayRegion(result,0,jsize(data.size()),reinterpret_cast<jbyte*>(data.data()));return result;}
JNIEXPORT jobjectArray JNICALL Java_dev_riye_toolscreen_NativeBridge_assetEntries(JNIEnv* env,jclass){auto images=toolscreen::images();int mode=int(toolscreen::snapshot()[0]);std::vector<toolscreen::ImageSource> active;for(auto& image:images)if(image.values[0]>.5&&(int(image.values[14])&(1<<mode)))active.push_back(image);auto result=env->NewObjectArray(jsize(active.size()*2),env->FindClass("java/lang/String"),nullptr);if(!result)return nullptr;for(size_t i=0;i<active.size();++i){auto id=env->NewStringUTF(std::to_string(active[i].id).c_str());auto path=env->NewStringUTF(active[i].path.c_str());env->SetObjectArrayElement(result,jsize(i*2),id);env->SetObjectArrayElement(result,jsize(i*2+1),path);env->DeleteLocalRef(id);env->DeleteLocalRef(path);}return result;}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_startSource(JNIEnv* env,jclass,jint id,jstring path){const char* text=env->GetStringUTFChars(path,nullptr);if(!text)return;toolscreen::startSource(id,text);env->ReleaseStringUTFChars(path,text);}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_stopSource(JNIEnv*,jclass,jint id){toolscreen::stopSource(id);}
JNIEXPORT jintArray JNICALL Java_dev_riye_toolscreen_NativeBridge_sourceInfo(JNIEnv* env,jclass,jint id){auto info=toolscreen::sourceInfo(id);auto result=env->NewIntArray(3);if(result){jint values[]={jint(info[0]),jint(info[1]),jint(info[2])};env->SetIntArrayRegion(result,0,3,values);}return result;}
JNIEXPORT jint JNICALL Java_dev_riye_toolscreen_NativeBridge_readSource(JNIEnv* env,jclass,jint id,jobject buffer,jint sequence){auto data=static_cast<uint8_t*>(env->GetDirectBufferAddress(buffer));auto size=env->GetDirectBufferCapacity(buffer);if(!data||size<=0||size>2147483647)return 0;return toolscreen::copySource(id,data,int(size),sequence);}
JNIEXPORT jstring JNICALL Java_dev_riye_toolscreen_NativeBridge_sourceError(JNIEnv* env,jclass,jint id){return env->NewStringUTF(toolscreen::sourceError(id).c_str());}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_assetStatus(JNIEnv* env,jclass,jint id,jint width,jint height,jstring error,jint canvasWidth,jint canvasHeight){const char* text=env->GetStringUTFChars(error,nullptr);if(!text)return;toolscreen::uiAssetStatus(id,width,height,text,canvasWidth,canvasHeight);env->ReleaseStringUTFChars(error,text);}
JNIEXPORT jint JNICALL Java_dev_riye_toolscreen_NativeBridge_addAsset(JNIEnv* env,jclass,jstring path){const char* text=env->GetStringUTFChars(path,nullptr);if(!text)return -1;int id=toolscreen::addImage(text);env->ReleaseStringUTFChars(path,text);return id;}
JNIEXPORT jint JNICALL Java_dev_riye_toolscreen_NativeBridge_addMirror(JNIEnv* env,jclass,jstring name,jint preset){const char* text=env->GetStringUTFChars(name,nullptr);if(!text)return -1;int index=toolscreen::addMirror(text,preset);env->ReleaseStringUTFChars(name,text);return index;}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_editAsset(JNIEnv*,jclass,jint id,jint field,jdouble value){toolscreen::editImage(id,field,value);}
JNIEXPORT jintArray JNICALL Java_dev_riye_toolscreen_NativeBridge_remapKey(JNIEnv* env,jclass,jint scan,jint key){auto value=toolscreen::remapKey(scan,key);auto result=env->NewIntArray(2);if(result){jint values[]={jint(value[0]),jint(value[1])};env->SetIntArrayRegion(result,0,2,values);}return result;}
JNIEXPORT jboolean JNICALL Java_dev_riye_toolscreen_NativeBridge_captureAvailable(JNIEnv*,jclass){return toolscreen::captureAccess();}
JNIEXPORT void JNICALL Java_dev_riye_toolscreen_NativeBridge_releaseHeldInput(JNIEnv*,jclass){toolscreen::releaseHeldInput();}
}
