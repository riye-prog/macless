#pragma once
#include <windows.h>
#include <winstring.h>
#include <inspectable.h>
namespace MaclessCapture {
struct SizeInt32{INT32 Width,Height;};
struct IGraphicsCaptureItem;
struct IGraphicsCaptureSession;
struct IGraphicsCaptureSession2;
struct IDirect3D11CaptureFrame;
struct IDirect3D11CaptureFramePool;
struct IDirect3D11CaptureFramePoolStatics2;
struct IDirect3DDevice;
struct IClosable;
struct IGraphicsCaptureItemInterop;
struct IDirect3DDxgiInterfaceAccess;
MIDL_INTERFACE("79c3f95b-31f7-4ec2-a464-632ef5d30760") IGraphicsCaptureItem:public IInspectable{
virtual HRESULT STDMETHODCALLTYPE get_DisplayName(HSTRING*)=0;
virtual HRESULT STDMETHODCALLTYPE get_Size(SizeInt32*)=0;
};
MIDL_INTERFACE("814e42a9-f70f-4ad7-939b-fddcc6eb880d") IGraphicsCaptureSession:public IInspectable{
virtual HRESULT STDMETHODCALLTYPE StartCapture()=0;
};
MIDL_INTERFACE("2c39ae40-7d2e-5044-804e-8b6799d4cf9e") IGraphicsCaptureSession2:public IInspectable{
virtual HRESULT STDMETHODCALLTYPE get_IsCursorCaptureEnabled(unsigned char*)=0;
virtual HRESULT STDMETHODCALLTYPE put_IsCursorCaptureEnabled(unsigned char)=0;
};
MIDL_INTERFACE("fa50c623-38da-4b32-acf3-fa9734ad800e") IDirect3D11CaptureFrame:public IInspectable{
virtual HRESULT STDMETHODCALLTYPE get_Surface(IInspectable**)=0;
virtual HRESULT STDMETHODCALLTYPE get_SystemRelativeTime(INT64*)=0;
virtual HRESULT STDMETHODCALLTYPE get_ContentSize(SizeInt32*)=0;
};
MIDL_INTERFACE("24eb6d22-1975-422e-82e7-780dbd8ddf24") IDirect3D11CaptureFramePool:public IInspectable{
virtual HRESULT STDMETHODCALLTYPE Recreate(IDirect3DDevice*,INT32,INT32,SizeInt32)=0;
virtual HRESULT STDMETHODCALLTYPE TryGetNextFrame(IDirect3D11CaptureFrame**)=0;
virtual HRESULT STDMETHODCALLTYPE add_FrameArrived(IUnknown*,INT64*)=0;
virtual HRESULT STDMETHODCALLTYPE remove_FrameArrived(INT64)=0;
virtual HRESULT STDMETHODCALLTYPE CreateCaptureSession(IGraphicsCaptureItem*,IGraphicsCaptureSession**)=0;
virtual HRESULT STDMETHODCALLTYPE get_DispatcherQueue(IInspectable**)=0;
};
MIDL_INTERFACE("589b103f-6bbc-5df5-a991-02e28b3b66d5") IDirect3D11CaptureFramePoolStatics2:public IInspectable{
virtual HRESULT STDMETHODCALLTYPE CreateFreeThreaded(IDirect3DDevice*,INT32,INT32,SizeInt32,IDirect3D11CaptureFramePool**)=0;
};
MIDL_INTERFACE("a37624ab-8d5f-4650-9d3e-9eae3d9bc670") IDirect3DDevice:public IInspectable{

};
MIDL_INTERFACE("30d5a829-7fa4-4026-83bb-d75bae4ea99e") IClosable:public IInspectable{
virtual HRESULT STDMETHODCALLTYPE Close()=0;
};
MIDL_INTERFACE("3628e81b-3cac-4c60-b7f4-23ce0e0c3356") IGraphicsCaptureItemInterop:public IUnknown{
virtual HRESULT STDMETHODCALLTYPE CreateForWindow(HWND,REFIID,void**)=0;
virtual HRESULT STDMETHODCALLTYPE CreateForMonitor(HMONITOR,REFIID,void**)=0;
};
MIDL_INTERFACE("a9b3d012-3df2-4ee3-b8d1-8695f457d3c1") IDirect3DDxgiInterfaceAccess:public IUnknown{
virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID,void**)=0;
};
}
#ifdef __MINGW32__
__CRT_UUID_DECL(MaclessCapture::IGraphicsCaptureItem,0x79c3f95b,0x31f7,0x4ec2,0xa4,0x64,0x63,0x2e,0xf5,0xd3,0x07,0x60)
__CRT_UUID_DECL(MaclessCapture::IGraphicsCaptureSession,0x814e42a9,0xf70f,0x4ad7,0x93,0x9b,0xfd,0xdc,0xc6,0xeb,0x88,0x0d)
__CRT_UUID_DECL(MaclessCapture::IGraphicsCaptureSession2,0x2c39ae40,0x7d2e,0x5044,0x80,0x4e,0x8b,0x67,0x99,0xd4,0xcf,0x9e)
__CRT_UUID_DECL(MaclessCapture::IDirect3D11CaptureFrame,0xfa50c623,0x38da,0x4b32,0xac,0xf3,0xfa,0x97,0x34,0xad,0x80,0x0e)
__CRT_UUID_DECL(MaclessCapture::IDirect3D11CaptureFramePool,0x24eb6d22,0x1975,0x422e,0x82,0xe7,0x78,0x0d,0xbd,0x8d,0xdf,0x24)
__CRT_UUID_DECL(MaclessCapture::IDirect3D11CaptureFramePoolStatics2,0x589b103f,0x6bbc,0x5df5,0xa9,0x91,0x02,0xe2,0x8b,0x3b,0x66,0xd5)
__CRT_UUID_DECL(MaclessCapture::IDirect3DDevice,0xa37624ab,0x8d5f,0x4650,0x9d,0x3e,0x9e,0xae,0x3d,0x9b,0xc6,0x70)
__CRT_UUID_DECL(MaclessCapture::IClosable,0x30d5a829,0x7fa4,0x4026,0x83,0xbb,0xd7,0x5b,0xae,0x4e,0xa9,0x9e)
__CRT_UUID_DECL(MaclessCapture::IGraphicsCaptureItemInterop,0x3628e81b,0x3cac,0x4c60,0xb7,0xf4,0x23,0xce,0x0e,0x0c,0x33,0x56)
__CRT_UUID_DECL(MaclessCapture::IDirect3DDxgiInterfaceAccess,0xa9b3d012,0x3df2,0x4ee3,0xb8,0xd1,0x86,0x95,0xf4,0x57,0xd3,0xc1)
#endif
