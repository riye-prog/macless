#include "platform.hpp"
#include "core.hpp"
#include "features.hpp"
#include <jni.h>
#include <algorithm>
#include <string>

namespace {
JavaVM* virtualMachine=nullptr;
jclass services=nullptr;
struct Environment {
    JNIEnv* value=nullptr;
    bool attached=false;
    Environment(){if(!virtualMachine||!services)return;if(virtualMachine->GetEnv(reinterpret_cast<void**>(&value),JNI_VERSION_1_8)==JNI_EDETACHED){attached=virtualMachine->AttachCurrentThread(reinterpret_cast<void**>(&value),nullptr)==JNI_OK;}if(value)value->PushLocalFrame(32);}
    ~Environment(){if(value){if(value->ExceptionCheck())value->ExceptionClear();value->PopLocalFrame(nullptr);}if(attached)virtualMachine->DetachCurrentThread();}
    explicit operator bool()const{return value!=nullptr;}
    jmethodID method(const char* name,const char* signature){return value->GetStaticMethodID(services,name,signature);}
    jstring string(const std::string& text){return value->NewStringUTF(text.c_str());}
    std::string text(jstring source){if(!source||value->ExceptionCheck())return "";const char* bytes=value->GetStringUTFChars(source,nullptr);if(!bytes)return "";std::string result(bytes);value->ReleaseStringUTFChars(source,bytes);return result;}
};
std::string readText(const char* method){Environment env;return env?env.text(static_cast<jstring>(env.value->CallStaticObjectMethod(services,env.method(method,"()Ljava/lang/String;")))):"Desktop services unavailable";}
}

namespace toolscreen {
void initializePlatform(void* environment){auto env=static_cast<JNIEnv*>(environment);env->GetJavaVM(&virtualMachine);auto type=env->FindClass("dev/riye/toolscreen/DesktopPlatform");if(type){services=static_cast<jclass>(env->NewGlobalRef(type));env->DeleteLocalRef(type);}}
void openImagePicker(bool background){Environment env;if(env)env.value->CallStaticVoidMethod(services,env.method("pickImage","(Z)V"),jboolean(background));}
const char* clipboardRead(ImGuiContext*){static thread_local std::string result;result=readText("readClipboard");return result.c_str();}
void clipboardWrite(ImGuiContext*,const char* text){Environment env;if(env)env.value->CallStaticVoidMethod(services,env.method("writeClipboard","(Ljava/lang/String;)V"),env.string(text?text:""));}
bool captureAccess(){Environment env;return env&&env.value->CallStaticBooleanMethod(services,env.method("captureAvailable","()Z"));}
void requestCaptureAccess(){refreshWindows();}
void refreshWindows(){Environment env;if(env)env.value->CallStaticVoidMethod(services,env.method("refreshWindows","()V"));}
std::vector<WindowSource> windowSources(){Environment env;std::vector<WindowSource> result;if(!env)return result;auto values=static_cast<jobjectArray>(env.value->CallStaticObjectMethod(services,env.method("windows","()[Ljava/lang/String;")));if(!values||env.value->ExceptionCheck())return result;for(jsize i=0;i<env.value->GetArrayLength(values);++i){auto item=static_cast<jstring>(env.value->GetObjectArrayElement(values,i));auto text=env.text(item);env.value->DeleteLocalRef(item);auto first=text.find('\t'),second=text.find('\t',first==std::string::npos?0:first+1);if(first==std::string::npos||second==std::string::npos)continue;try{result.push_back({uint32_t(std::stoul(text.substr(0,first))),text.substr(first+1,second-first-1),text.substr(second+1)});}catch(const std::exception&){}}return result;}
std::string captureStatus(){return readText("captureStatus");}
void startSource(int id,const std::string& path){Environment env;if(env)env.value->CallStaticVoidMethod(services,env.method("startSource","(ILjava/lang/String;)V"),jint(id),env.string(path));}
void stopSource(int id){Environment env;if(env)env.value->CallStaticVoidMethod(services,env.method("stopSource","(I)V"),jint(id));}
std::array<int,3> sourceInfo(int id){Environment env;std::array<int,3> result{};if(!env)return result;auto values=static_cast<jintArray>(env.value->CallStaticObjectMethod(services,env.method("sourceInfo","(I)[I"),jint(id)));if(values&&!env.value->ExceptionCheck()&&env.value->GetArrayLength(values)==3){jint data[3];env.value->GetIntArrayRegion(values,0,3,data);std::copy(data,data+3,result.begin());}return result;}
int copySource(int id,uint8_t* destination,int capacity,int previousSequence){Environment env;if(!env||!destination||capacity<=0)return 0;auto buffer=env.value->NewDirectByteBuffer(destination,capacity);return env.value->CallStaticIntMethod(services,env.method("readSource","(ILjava/nio/ByteBuffer;I)I"),jint(id),buffer,jint(previousSequence));}
std::string sourceError(int id){Environment env;return env?env.text(static_cast<jstring>(env.value->CallStaticObjectMethod(services,env.method("sourceError","(I)Ljava/lang/String;"),jint(id)))):"Desktop services unavailable";}
}
