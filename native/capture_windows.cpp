#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "capture_interop.hpp"
#include <roapi.h>
#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cwchar>
#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using Microsoft::WRL::ComPtr;
namespace Capture=MaclessCapture;
namespace Direct3D=MaclessCapture;
using MaclessCapture::SizeInt32;
using MaclessCapture::IGraphicsCaptureItemInterop;
using MaclessCapture::IDirect3DDxgiInterfaceAccess;

std::string utf8(std::wstring value){if(value.empty())return "";int length=WideCharToMultiByte(CP_UTF8,0,value.data(),int(value.size()),nullptr,0,nullptr,nullptr);std::string result(size_t(length),' ');WideCharToMultiByte(CP_UTF8,0,value.data(),int(value.size()),result.data(),length,nullptr,nullptr);for(char& ch:result)if(ch=='\t'||ch=='\r'||ch=='\n')ch=' ';return result;}
void writeInteger(uint32_t value){uint8_t bytes[]={uint8_t(value>>24),uint8_t(value>>16),uint8_t(value>>8),uint8_t(value)};if(std::fwrite(bytes,1,4,stdout)!=4)throw std::runtime_error("Capture consumer closed");}
void writePixels(const uint8_t* bgra,int width,int height,int stride){int step=std::max(1,int(std::ceil(std::max(width/1920.0,height/1080.0))));int outputWidth=(width+step-1)/step,outputHeight=(height+step-1)/step;std::vector<uint8_t> rgba(size_t(outputWidth)*outputHeight*4);for(int y=0;y<outputHeight;++y)for(int x=0;x<outputWidth;++x){auto source=bgra+size_t(y*step)*stride+x*step*4;auto target=rgba.data()+(size_t(y)*outputWidth+x)*4;target[0]=source[2];target[1]=source[1];target[2]=source[0];target[3]=255;}writeInteger(0x4d434631);writeInteger(uint32_t(outputWidth));writeInteger(uint32_t(outputHeight));writeInteger(uint32_t(rgba.size()));if(std::fwrite(rgba.data(),1,rgba.size(),stdout)!=rgba.size()||std::fflush(stdout)!=0)throw std::runtime_error("Capture consumer closed");}
void check(HRESULT result){if(FAILED(result))throw std::runtime_error("Windows Graphics Capture unavailable");}
struct RuntimeString{HSTRING value=nullptr;explicit RuntimeString(const wchar_t* text){check(WindowsCreateString(text,UINT32(std::wcslen(text)),&value));}~RuntimeString(){WindowsDeleteString(value);}};
void closeInspectable(IUnknown* value){if(!value)return;ComPtr<MaclessCapture::IClosable> close;if(SUCCEEDED(value->QueryInterface(IID_PPV_ARGS(&close))))close->Close();}

bool captureGraphics(HWND window){
    ComPtr<Capture::IGraphicsCaptureItem> item;ComPtr<Capture::IDirect3D11CaptureFramePool> pool;ComPtr<Capture::IGraphicsCaptureSession> session;bool started=false;
    try{
        check(RoInitialize(RO_INIT_MULTITHREADED));RuntimeString itemClass(L"Windows.Graphics.Capture.GraphicsCaptureItem");ComPtr<IGraphicsCaptureItemInterop> interop;check(RoGetActivationFactory(itemClass.value,IID_PPV_ARGS(&interop)));check(interop->CreateForWindow(window,__uuidof(Capture::IGraphicsCaptureItem),reinterpret_cast<void**>(item.GetAddressOf())));
        ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));ComPtr<IDXGIDevice> dxgi;check(device.As(&dxgi));ComPtr<IInspectable> inspectable;auto createDevice=reinterpret_cast<HRESULT(WINAPI*)(IDXGIDevice*,IInspectable**)>(GetProcAddress(GetModuleHandleW(L"d3d11.dll"),"CreateDirect3D11DeviceFromDXGIDevice"));if(!createDevice)throw std::runtime_error("Windows Graphics Capture unavailable");check(createDevice(dxgi.Get(),&inspectable));ComPtr<Direct3D::IDirect3DDevice> direct;check(inspectable.As(&direct));
        RuntimeString poolClass(L"Windows.Graphics.Capture.Direct3D11CaptureFramePool");ComPtr<Capture::IDirect3D11CaptureFramePoolStatics2> factory;check(RoGetActivationFactory(poolClass.value,IID_PPV_ARGS(&factory)));SizeInt32 size{};check(item->get_Size(&size));auto format=INT32(87);check(factory->CreateFreeThreaded(direct.Get(),format,2,size,&pool));check(pool->CreateCaptureSession(item.Get(),&session));ComPtr<Capture::IGraphicsCaptureSession2> cursor;if(SUCCEEDED(session.As(&cursor)))cursor->put_IsCursorCaptureEnabled(false);check(session->StartCapture());started=true;ComPtr<ID3D11Texture2D> staging;D3D11_TEXTURE2D_DESC allocated{};
        while(IsWindow(window)){
            auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(33);ComPtr<Capture::IDirect3D11CaptureFrame> frame;check(pool->TryGetNextFrame(&frame));if(!frame){std::this_thread::sleep_for(std::chrono::milliseconds(5));continue;}SizeInt32 content{};check(frame->get_ContentSize(&content));ComPtr<IInspectable> surface;check(frame->get_Surface(&surface));ComPtr<IDirect3DDxgiInterfaceAccess> access;check(surface.As(&access));ComPtr<ID3D11Texture2D> texture;check(access->GetInterface(IID_PPV_ARGS(&texture)));D3D11_TEXTURE2D_DESC descriptor{};texture->GetDesc(&descriptor);
            if(content.Width>0&&content.Height>0&&content.Width<=int(descriptor.Width)&&content.Height<=int(descriptor.Height)&&int64_t(content.Width)*content.Height<=16777216){if(!staging||allocated.Width!=descriptor.Width||allocated.Height!=descriptor.Height){allocated=descriptor;allocated.Usage=D3D11_USAGE_STAGING;allocated.BindFlags=0;allocated.CPUAccessFlags=D3D11_CPU_ACCESS_READ;allocated.MiscFlags=0;staging.Reset();check(device->CreateTexture2D(&allocated,nullptr,&staging));}context->CopyResource(staging.Get(),texture.Get());D3D11_MAPPED_SUBRESOURCE mapped{};check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped));try{writePixels(static_cast<uint8_t*>(mapped.pData),content.Width,content.Height,int(mapped.RowPitch));}catch(...){context->Unmap(staging.Get(),0);throw;}context->Unmap(staging.Get(),0);}
            closeInspectable(frame.Get());frame.Reset();if(content.Width>0&&content.Height>0&&(content.Width!=size.Width||content.Height!=size.Height)){size=content;check(pool->Recreate(direct.Get(),format,2,size));}std::this_thread::sleep_until(deadline);
        }
        closeInspectable(session.Get());closeInspectable(pool.Get());RoUninitialize();return true;
    }catch(const std::exception&){closeInspectable(session.Get());closeInspectable(pool.Get());RoUninitialize();if(started)throw;return false;}
}

void captureCompatible(HWND window){
    while(IsWindow(window)){
        auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(33);RECT bounds{};if(!GetWindowRect(window,&bounds))throw std::runtime_error("Window unavailable");int width=bounds.right-bounds.left,height=bounds.bottom-bounds.top;if(width<1||height<1||int64_t(width)*height>16777216)throw std::runtime_error("Invalid window size");HDC source=GetWindowDC(window);if(!source)throw std::runtime_error("Window unavailable");HDC target=CreateCompatibleDC(source);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;void* pixels=nullptr;HBITMAP image=CreateDIBSection(source,&info,DIB_RGB_COLORS,&pixels,nullptr,0);if(!target||!image){if(image)DeleteObject(image);if(target)DeleteDC(target);ReleaseDC(window,source);throw std::runtime_error("Could not allocate capture");}HGDIOBJ previous=SelectObject(target,image);BOOL success=PrintWindow(window,target,2);if(!success)success=BitBlt(target,0,0,width,height,source,0,0,SRCCOPY|CAPTUREBLT);try{if(success)writePixels(static_cast<const uint8_t*>(pixels),width,height,width*4);}catch(...){SelectObject(target,previous);DeleteObject(image);DeleteDC(target);ReleaseDC(window,source);throw;}SelectObject(target,previous);DeleteObject(image);DeleteDC(target);ReleaseDC(window,source);if(!success)throw std::runtime_error("Window capture failed");std::this_thread::sleep_until(deadline);
    }
}

int main(int argc,char** argv){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    try{
        if(argc==2&&std::string(argv[1])=="--list"){
            EnumWindows([](HWND window,LPARAM)->BOOL{if(!IsWindowVisible(window)||GetWindowTextLengthW(window)==0)return TRUE;RECT rect{};GetWindowRect(window,&rect);if(rect.right-rect.left<20||rect.bottom-rect.top<20)return TRUE;std::wstring title(size_t(GetWindowTextLengthW(window))+1,L'\0');int length=GetWindowTextW(window,title.data(),int(title.size()));title.resize(size_t(std::max(0,length)));DWORD pid=0;GetWindowThreadProcessId(window,&pid);std::wstring app=L"Window";HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(process){wchar_t path[32768];DWORD capacity=32768;if(QueryFullProcessImageNameW(process,0,path,&capacity)){app.assign(path,capacity);auto slash=app.find_last_of(L"\\/");if(slash!=std::wstring::npos)app=app.substr(slash+1);}CloseHandle(process);}std::cout<<uint32_t(reinterpret_cast<uintptr_t>(window))<<'\t'<<pid<<'\t'<<utf8(app)<<'\t'<<utf8(title)<<'\n';return TRUE;},0);return 0;
        }
        if(argc!=3||std::string(argv[1])!="--capture")throw std::runtime_error("Expected --list or --capture window");_setmode(_fileno(stdout),_O_BINARY);HWND window=reinterpret_cast<HWND>(uintptr_t(std::stoull(argv[2])));if(!IsWindow(window))throw std::runtime_error("The selected window is unavailable");if(!captureGraphics(window))captureCompatible(window);return 0;
    }catch(const std::exception& failure){std::cerr<<failure.what()<<'\n';return 1;}
}
