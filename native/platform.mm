#include "platform.hpp"
#include "core.hpp"
#include <AppKit/AppKit.h>
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <WebKit/WebKit.h>
#include <ScreenCaptureKit/ScreenCaptureKit.h>
#include <CoreVideo/CoreVideo.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreGraphics/CoreGraphics.h>
#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unistd.h>

namespace {
struct SourceState{
    std::mutex mutex;
    std::vector<uint8_t> pixels;
    int width=0,height=0,sequence=0;
    bool stopped=false;
    std::string error;
};
std::mutex sourceMutex;
std::unordered_map<int,std::shared_ptr<SourceState>> sourceStates;
std::vector<toolscreen::WindowSource> sourceWindows;
std::string windowStatus;
NSMutableDictionary<NSNumber*,SCWindow*>* windowObjects;

std::shared_ptr<SourceState> findSource(int id){std::lock_guard lock(sourceMutex);auto found=sourceStates.find(id);return found==sourceStates.end()?nullptr:found->second;}
bool sourceStopped(const std::shared_ptr<SourceState>& state){std::lock_guard lock(state->mutex);return state->stopped;}
void setSourceError(const std::shared_ptr<SourceState>& state,NSError* error){if(!state)return;std::lock_guard lock(state->mutex);state->error=error?error.localizedDescription.UTF8String:"Source unavailable";}
void storePixels(const std::shared_ptr<SourceState>& state,std::vector<uint8_t> pixels,int width,int height){std::lock_guard lock(state->mutex);if(state->stopped)return;state->pixels=std::move(pixels);state->width=width;state->height=height;state->sequence++;state->error.clear();}
}

@interface TSNativeSource:NSObject<SCStreamOutput,SCStreamDelegate,WKNavigationDelegate>{
@public
    std::shared_ptr<SourceState> state;
    SCStream* stream;
    WKWebView* web;
    NSWindow* browserWindow;
    NSTimer* timer;
    BOOL snapshotPending;
}
-(void)startBrowser:(NSString*)url;
-(void)startWindow:(SCWindow*)window;
-(void)stop;
@end

@implementation TSNativeSource
-(void)startBrowser:(NSString*)url{
    WKWebViewConfiguration* config=[[WKWebViewConfiguration alloc] init];
    config.mediaTypesRequiringUserActionForPlayback=WKAudiovisualMediaTypeAll;
    web=[[WKWebView alloc] initWithFrame:NSMakeRect(0,0,960,540) configuration:config];
    web.navigationDelegate=self;web.underPageBackgroundColor=NSColor.clearColor;
    browserWindow=[[NSWindow alloc] initWithContentRect:NSMakeRect(-20000,-20000,960,540) styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:NO];
    browserWindow.releasedWhenClosed=NO;browserWindow.contentView=web;browserWindow.opaque=NO;browserWindow.backgroundColor=NSColor.clearColor;
    NSURL* address=[NSURL URLWithString:url];
    if(address.isFileURL)[web loadFileURL:address allowingReadAccessToURL:address.URLByDeletingLastPathComponent];
    else if([address.scheme isEqualToString:@"https"]||[address.scheme isEqualToString:@"http"])[web loadRequest:[NSURLRequest requestWithURL:address]];
    else {std::lock_guard lock(state->mutex);state->error="Use an http, https or file URL";return;}
    __weak TSNativeSource* weakSelf=self;
    timer=[NSTimer scheduledTimerWithTimeInterval:1.0/15 repeats:YES block:^(NSTimer*){
        TSNativeSource* strongSelf=weakSelf;
        if(!strongSelf||strongSelf->snapshotPending||strongSelf->web.loading)return;
        strongSelf->snapshotPending=YES;
        WKSnapshotConfiguration* snapshot=[[WKSnapshotConfiguration alloc] init];snapshot.rect=strongSelf->web.bounds;snapshot.snapshotWidth=@960;
        [strongSelf->web takeSnapshotWithConfiguration:snapshot completionHandler:^(NSImage* image,NSError* error){
            strongSelf->snapshotPending=NO;
            if(error||!image){setSourceError(strongSelf->state,error);return;}
            CGImageRef cg=[image CGImageForProposedRect:nil context:nil hints:nil];
            if(!cg)return;
            int width=960,height=540;
            std::vector<uint8_t> pixels(size_t(width)*height*4);
            CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
            CGContextRef drawing=CGBitmapContextCreate(pixels.data(),width,height,8,width*4,space,CGBitmapInfo(kCGImageAlphaPremultipliedLast)|CGBitmapInfo(kCGBitmapByteOrder32Big));
            CGColorSpaceRelease(space);
            if(!drawing)return;
            CGContextDrawImage(drawing,CGRectMake(0,0,width,height),cg);CGContextRelease(drawing);
            for(size_t i=0;i<pixels.size();i+=4){int alpha=pixels[i+3];if(alpha>0&&alpha<255)for(int c=0;c<3;++c)pixels[i+c]=uint8_t(std::min(255,int(pixels[i+c])*255/alpha));}
            storePixels(strongSelf->state,std::move(pixels),width,height);
        }];
    }];
    [[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}
-(void)webView:(WKWebView*)view didFailNavigation:(WKNavigation*)navigation withError:(NSError*)error{(void)view;(void)navigation;setSourceError(state,error);}
-(void)webView:(WKWebView*)view didFailProvisionalNavigation:(WKNavigation*)navigation withError:(NSError*)error{(void)view;(void)navigation;setSourceError(state,error);}
-(void)startWindow:(SCWindow*)window{
    SCContentFilter* filter=[[SCContentFilter alloc] initWithDesktopIndependentWindow:window];
    SCStreamConfiguration* config=[[SCStreamConfiguration alloc] init];
    double factor=std::min(2.0,1920.0/std::max(window.frame.size.width,1.0));
    config.width=std::max(1,int(window.frame.size.width*factor));config.height=std::max(1,int(window.frame.size.height*factor));
    config.pixelFormat=kCVPixelFormatType_32BGRA;config.minimumFrameInterval=CMTimeMake(1,30);config.queueDepth=3;config.showsCursor=NO;config.capturesAudio=NO;config.ignoreShadowsSingleWindow=YES;
    config.colorSpaceName=kCGColorSpaceSRGB;
    stream=[[SCStream alloc] initWithFilter:filter configuration:config delegate:self];
    NSError* error=nil;
    dispatch_queue_t queue=dispatch_queue_create("dev.riye.toolscreen.window-capture",DISPATCH_QUEUE_SERIAL);
    if(![stream addStreamOutput:self type:SCStreamOutputTypeScreen sampleHandlerQueue:queue error:&error]){setSourceError(state,error);return;}
    [stream startCaptureWithCompletionHandler:^(NSError* failure){if(failure)setSourceError(self->state,failure);}];
}
-(void)stream:(SCStream*)capture didStopWithError:(NSError*)error{(void)capture;setSourceError(state,error);}
-(void)stream:(SCStream*)capture didOutputSampleBuffer:(CMSampleBufferRef)sample ofType:(SCStreamOutputType)type{
    (void)capture;if(type!=SCStreamOutputTypeScreen||!CMSampleBufferIsValid(sample))return;
    CVPixelBufferRef pixel=CMSampleBufferGetImageBuffer(sample);if(!pixel)return;
    CVPixelBufferLockBaseAddress(pixel,kCVPixelBufferLock_ReadOnly);
    int width=int(CVPixelBufferGetWidth(pixel)),height=int(CVPixelBufferGetHeight(pixel));size_t stride=CVPixelBufferGetBytesPerRow(pixel);
    const uint8_t* input=static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pixel));
    std::vector<uint8_t> output(size_t(width)*height*4);
    if(input)for(int y=0;y<height;++y)for(int x=0;x<width;++x){size_t source=size_t(y)*stride+x*4,destination=(size_t(y)*width+x)*4;output[destination]=input[source+2];output[destination+1]=input[source+1];output[destination+2]=input[source];output[destination+3]=input[source+3];}
    CVPixelBufferUnlockBaseAddress(pixel,kCVPixelBufferLock_ReadOnly);
    storePixels(state,std::move(output),width,height);
}
-(void)stop{
    {std::lock_guard lock(state->mutex);state->stopped=true;}
    [timer invalidate];timer=nil;
    [web stopLoading];web.navigationDelegate=nil;
    [browserWindow close];browserWindow=nil;web=nil;
    if(stream){NSError* error=nil;[stream removeStreamOutput:self type:SCStreamOutputTypeScreen error:&error];[stream stopCaptureWithCompletionHandler:^(NSError*){}];stream=nil;}
}
@end

namespace {
NSMutableDictionary<NSNumber*,TSNativeSource*>* runningSources(){static NSMutableDictionary<NSNumber*,TSNativeSource*>* instances;static dispatch_once_t once;dispatch_once(&once,^{instances=[NSMutableDictionary dictionary];});return instances;}
}

namespace toolscreen {
void initializePlatform(void*){}
void openImagePicker(bool background){
    dispatch_async(dispatch_get_main_queue(), ^{
        NSOpenPanel* panel=[NSOpenPanel openPanel];
        panel.allowedContentTypes=@[UTTypeImage];
        panel.canChooseDirectories=NO;
        panel.allowsMultipleSelection=NO;
        panel.message=@"Choose an image to overlay in Macless";
        [panel beginWithCompletionHandler:^(NSModalResponse response){
            if(response==NSModalResponseOK&&panel.URL.path){if(background)addBackground(panel.URL.path.UTF8String);else addImage(panel.URL.path.UTF8String);}
        }];
    });
}
const char* clipboardRead(ImGuiContext*){
    static thread_local std::string value;
    @autoreleasepool{
        NSString* text=[[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
        value=text?text.UTF8String:"";
    }
    return value.c_str();
}
void clipboardWrite(ImGuiContext*,const char* text){
    @autoreleasepool{
        NSString* value=[NSString stringWithUTF8String:text?text:""];
        if(!value)return;
        NSPasteboard* clipboard=[NSPasteboard generalPasteboard];
        [clipboard clearContents];
        [clipboard setString:value forType:NSPasteboardTypeString];
    }
}
bool captureAccess(){return CGPreflightScreenCaptureAccess();}
void requestCaptureAccess(){dispatch_async(dispatch_get_main_queue(),^{CGRequestScreenCaptureAccess();});}
void refreshWindows(){
    if(!captureAccess()){std::lock_guard lock(sourceMutex);windowStatus="Enable Screen Recording for Minecraft or Java to capture windows.";return;}
    {std::lock_guard lock(sourceMutex);windowStatus="Refreshing windows...";}
    [SCShareableContent getShareableContentExcludingDesktopWindows:YES onScreenWindowsOnly:NO completionHandler:^(SCShareableContent* content,NSError* error){
        if(error){std::lock_guard lock(sourceMutex);windowStatus=error.localizedDescription.UTF8String;return;}
        dispatch_async(dispatch_get_main_queue(),^{
            windowObjects=[NSMutableDictionary dictionary];std::vector<WindowSource> windows;
            for(SCWindow* window in content.windows){if(window.frame.size.width<20||window.frame.size.height<20)continue;NSString* title=window.title;NSString* app=window.owningApplication.applicationName;if(!title.length)continue;windowObjects[@(window.windowID)]=window;windows.push_back({window.windowID,app?app.UTF8String:"Window",title.UTF8String});}
            std::lock_guard lock(sourceMutex);sourceWindows=std::move(windows);windowStatus=sourceWindows.empty()?"No capturable windows found":"";
        });
    }];
}
std::vector<WindowSource> windowSources(){std::lock_guard lock(sourceMutex);return sourceWindows;}
std::string captureStatus(){std::lock_guard lock(sourceMutex);return windowStatus;}
void startSource(int id,const std::string& path){
    auto state=std::make_shared<SourceState>();
    {std::lock_guard lock(sourceMutex);if(sourceStates.contains(id))return;sourceStates[id]=state;}
    NSString* source=[NSString stringWithUTF8String:path.c_str()];
    dispatch_async(dispatch_get_main_queue(),^{
        if(sourceStopped(state))return;
        TSNativeSource* capture=[[TSNativeSource alloc] init];capture->state=state;runningSources()[@(id)]=capture;
        if([source hasPrefix:@"browser:"])[capture startBrowser:[source substringFromIndex:8]];
        else if([source hasPrefix:@"window:"]){
            NSArray<NSString*>* parts=[[source substringFromIndex:7] componentsSeparatedByString:@"|"];
            bool ownWindow=[parts[0] isEqualToString:@"self"];
            uint32_t windowId=uint32_t([parts[0] longLongValue]);SCWindow* window=windowObjects[@(windowId)];
            NSString* expectedApp=parts.count>=3?parts[1]:nil;NSString* expectedTitle=parts.count>=3?[[parts subarrayWithRange:NSMakeRange(2,parts.count-2)] componentsJoinedByString:@"|"]:nil;
            if(window&&expectedApp&&(![window.owningApplication.applicationName isEqualToString:expectedApp]||![window.title isEqualToString:expectedTitle]))window=nil;
            if(window)[capture startWindow:window];
            else [SCShareableContent getShareableContentExcludingDesktopWindows:YES onScreenWindowsOnly:NO completionHandler:^(SCShareableContent* content,NSError* error){
                if(error){setSourceError(state,error);return;}
                if(sourceStopped(state))return;
                SCWindow* found=nil;for(SCWindow* candidate in content.windows){bool idMatches=candidate.windowID==windowId;bool nameMatches=expectedApp&&[candidate.owningApplication.applicationName isEqualToString:expectedApp]&&[candidate.title isEqualToString:expectedTitle];if(ownWindow&&candidate.owningApplication.processID==getpid()&&candidate.title.length>0&&candidate.frame.size.width>300&&candidate.frame.size.height>200){if(!found||candidate.frame.size.width*candidate.frame.size.height>found.frame.size.width*found.frame.size.height)found=candidate;}else if((idMatches&&!expectedApp)||nameMatches){found=candidate;break;}}
                if(found)dispatch_async(dispatch_get_main_queue(),^{if(!sourceStopped(state))[capture startWindow:found];});else {std::lock_guard lock(state->mutex);state->error="The selected window is no longer available. Choose it again from Sources.";}
            }];
        }
    });
}
void stopSource(int id){
    auto source=findSource(id);if(source){std::lock_guard lock(source->mutex);source->stopped=true;}
    {std::lock_guard lock(sourceMutex);sourceStates.erase(id);}
    dispatch_async(dispatch_get_main_queue(),^{TSNativeSource* capture=runningSources()[@(id)];[capture stop];[runningSources() removeObjectForKey:@(id)];});
}
std::array<int,3> sourceInfo(int id){auto source=findSource(id);if(!source)return {0,0,0};std::lock_guard lock(source->mutex);return {source->width,source->height,source->sequence};}
int copySource(int id,uint8_t* destination,int capacity,int previousSequence){auto source=findSource(id);if(!source||!destination)return 0;std::lock_guard lock(source->mutex);if(source->sequence==previousSequence||capacity!=int(source->pixels.size()))return 0;std::copy(source->pixels.begin(),source->pixels.end(),destination);return source->sequence;}
std::string sourceError(int id){auto source=findSource(id);if(!source)return "";std::lock_guard lock(source->mutex);return source->error;}
}
