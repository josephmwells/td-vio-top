#ifndef FILE_VENTUZ_VIOAPI_VIO_H
#define FILE_VENTUZ_VIOAPI_VIO_H

#ifndef _D3D9_H_
struct IDirect3DDevice9Ex;
struct IDirect3DSurface9;
#endif

#ifdef VIOAPI_EXPORTS
#define VIOAPI_CALL __declspec(dllexport)
#else
#define VIOAPI_CALL __declspec(dllimport)
#endif


#ifdef __cplusplus

struct ID3D11Device;
struct IDirect3DDevice9Ex;

namespace VioApi {
extern "C" {
#endif

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

typedef int vHandle;
typedef long long int vint64;
typedef unsigned int vuint;
typedef unsigned char vuint8;

/****************************************************************************/

enum vError
{
    VE_Ok                           =  0,   // OK
    VE_Unspecified                  =  1,   // unspecified error
    VE_NoFrame                      =  2,   // no frame to lock yet
    VE_OutOfMemory                  =  3,   // malloc() failed
    VE_NoConnection                 =  4,   // no connection to ventuz
    VE_BadHandle                    =  5,   // the handle is illegal or associated stream has been closed
    VE_BadParameter                 =  6,   // for instance: value not part of enum, out of range, ...
    VE_BadSize                      =  7,   // vOpenPara::SizeX or vOpenPara::SizeY are wrong
    VE_RequireContext               =  8,   // vOpenPara::DXDev or vOpenPara::GLContext must be specified
    VE_AlreadyOpen                  =  9,   // stream already open
    VE_Locked                       = 10,   // stream is locked, and should be unlocked
    VE_NotLocked                    = 11,   // stream should be locked but isn't
    VE_DirectX                      = 12,   // some directx error
    VE_OpenGl                       = 13,   // some opengl error
    VE_NotImplemented               = 14,   // feature not yet available
    VE_NotLicensed                  = 15,   // license does not cover the feature
    VE_BadPacking                   = 16,   // color or depth packing is not compatible with transfer mode
    VE_AncNotFound                  = 17,   // considering fourcc and index arguments, AncFromVentuz() coud not find a anc blob.
    VE_AncOverflow                  = 18,   // AncToVentuz() : too many anc blobs, or the anc blobs are too large
    VE_BadVioVersion                = 19,   // can not understand the vio protocol version.
    VE_ConnectionBroken             = 20,   // VioConnection is broken (Server crashed.) Call vExit and start over.
    VE_IncompatibleBuffer           = 21,   // vBuffer does not match stream
    VE_Mapped                       = 22,   // vBuffer is mapped but should not.
    VE_NotMapped                    = 23,   // vBuffer is not mapped but should be.
    VE_AsyncWait                    = 24,   // async operation not finished, try again later
};

enum vMode
{
    VM_Off = 0,
    VM_ToVentuz = 1,                        // Send Frames to Ventuz
    VM_FromVentuz = 2,                      // Receive Frames fromVen
    VM_Bidirectional = 3,                   // for used-allocated buffers (do only used for vCreateBuffer(), do not use for vOpen().
};

enum vTransferMode
{
    VTM_Cpu = 1,                            // cpu memory
    VTM_Direct3D9 = 2,                      // gpu memory DirectX surface
    VTM_OpenGl = 3,                         // gpu memory opengl surface
    VTM_Direct3D11 = 4,                     // gpu memory DirectX11 surface
};

enum vColorPacking
{
    VCP_RGBA_8 = 0,                         // RGBA
    VCP_BGRA_8 = 1,                         // BGRA
    VCP_RGBA_F32 = 2,                       // BGRA 32 bit float per channel
    // UYV only for memory transfers from Ventuz
    VCP_UYVY_8  = 3,                        // 2 pixel in 1x32 bit: UYVY
    VCP_YUYV_8  = 4,                        // 2 pixel in 1x32 bit: YVYU
    VCP_UYVY_10 = 5,                        // 6 pixel in 4x32 bit: UYV YUY VYU YVY
    VCP_YUKYVK_10 = 6,                      // 2 pixel in 2x32 bit: YUK YVK
};

enum vDepthPacking
{
    VDP_Off = 0,                            // no depth buffer
    VDP_F32 = 1,                            // float 32 bit
    VDP_U16 = 2,                            // uint 16 bit
    VDP_BGRA8 = 3,                          // depth = r/0xff + g/0xffff + b/0xffffff, for DX11
};

enum vColorSpace                            // colorspace, for future extension to YUV
{
    VCS_RGB = 0,
    VCS_BT601 = 1,
    VCS_BT709 = 2,
};

enum vStreamFlags                           // flags that fit nowhere else
{
    VSF_None                    = 0x0000,   // none
    VSF_DoubleBuffer            = 0x0001,   // add one latency for better performance. Only for VTM_CPU & VM_ToVentuz
    VSF_CpuUsingDx9             = 0x0002,   // use dx9 instead of default dx11 for VTM_Cpu
    VSF_Interlaced              = 0x0004,   // create interlaced shader. Only available for CPU transfers
    VSF_UserBuffers             = 0x0008,   // buffers allocated by user (vBuffer)
    VSF_Async                   = 0x0010,   // lock and unlock are async
};

enum vLockFlags
{
    VLF_None                    = 0x0000,
    VLF_FillBuffers             = 0x0001,   // automatically fill buffers without calling vFillBuffers(). default on.

    VLF_InterlaceMask           = 0x0700,   // mask for interlaced modes. only available for CPU transfers. only used when VSF_Interlaced is set.
    VLF_InterlaceField0         = 0x0000,   // transfer field 0 in field-by-field buffer and do not copy to memory
    VLF_InterlaceField1         = 0x0100,   // transfer field 1 in field-by-field buffer and do copy to memory
    VLF_InterlaceInterleaved0   = 0x0200,   // transfer field 0 in interleaved buffer and do not copy to memory
    VLF_InterlaceInterleaved1   = 0x0300,   // transfer field 1 in interleaved buffer and do copy to memory
    VLF_InterlaceFieldMask      = 0x0100,
    VLF_InterlaceInterleavedMask= 0x0200,
    VLF_InterlaceFieldFromBuffer = 0x0400,  // FromVentuz : ignore VLF_InterlaceFieldMask and use vFrame::FrameCount & 1
};

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

struct vOpenPara                            // parameters used for vOpen()
{
    int Channel;                            // channel 0..1. This is independent from direction.
    enum vMode Mode;                        // direction of data : to or from ventuz. Must match channel.
    enum vTransferMode Transfer;            // transfer mode (cpu / d3d / gl)
    enum vColorPacking Color;               // color buffer
    enum vDepthPacking Depth;               // depth buffer (can be disabled)
    enum vColorSpace ColorSpace;            // currently unused, set to VCS_RGB
    int SizeX;                              // Size of buffer
    int SizeY;                              // output size must match ventuz settings, inputs are autodetect on ventuz side
    int ExtraBuffers;                       // extra delay, only for CPU Output Streams running on DX11
    enum vStreamFlags Flags;                // more flags
};

struct vFrame                               // returned by vLockFrame()
{
    // VTM_Cpu

    unsigned char *ColorBuffer;             // data pointer for color, not used for VP_DirectX or VP_OpenGl
    unsigned char *DepthBuffer;             // data pointer for depth, not used for VP_DirectX or VP_OpenGl
    int ColorStride;                        // bytes from one line to the next, including potential padding
    int DepthStride;
    int ColorBytesPerLine;                  // for copying YUV buffers
    int DepthBytesPerLine;                  // for copying YUV buffers
    int Lines;                              // for copying YUV buffers

    // VTM_Direct3D9

    struct IDirect3DSurface9 *DxColorSurf;
    struct IDirect3DSurface9 *DxDepthSurf;

    // VTM_OpenGl

    vuint GlColorName;
    vuint GlDepthName;

    // common

    vint64 ClusterClock;                    // ventuz global timing
    vint64 FrameCount;                      // count for expected frames (frames send + frames dropped)
    vint64 DropCount;                       // count for dropped frames

    // VTM_Direct3D11

    struct ID3D11Texture2D *DX11ColorTex;
    struct ID3D11RenderTargetView *DX11ColorTargetView;
    struct ID3D11ShaderResourceView *DX11ColorShaderView;

    struct ID3D11Texture2D *DX11DepthTex;
    struct ID3D11RenderTargetView *DX11DepthTargetView;
    struct ID3D11ShaderResourceView *DX11DepthShaderView;

    // SDI Timecode from Ventuz

    vuint8 TimecodeFps;              // frames per seconds, or 0 for no timecode
    vuint8 TimecodeDrop;             // Indicate dropframe-timecode
    vuint8 TimecodeHour; 
    vuint8 TimecodeMinute; 
    vuint8 TimecodeSecond; 
    vuint8 TimecodeFrame; 
};

struct vBuffer                              // you can manage the buffers yourself - so you are not fucked when Vio stops - only for CPU buffer now
{
    // user

    vint64 UserFieldCount;                  // timecode from client side

    // copy of creation parameters - do not change

    int SizeX;
    int SizeY;
    int ColorBytesPerLine;
    int DepthBytesPerLine;
    enum vMode Mode;
    enum vTransferMode Transfer;
    enum vColorPacking Color;
    enum vDepthPacking Depth;

    // resources and internal state

    struct IDirect3DSurface9 *Color9;
    struct ID3D11Texture2D *Color11;
    struct IDirect3DSurface9 *Depth9;
    struct ID3D11Texture2D *Depth11;

    bool Mapped;
};

struct vInfo                                // returned by vGetInfo()
{
    // same as open parameter

    int Channel;
    enum vMode Mode;
    enum vTransferMode Transfer;
    enum vColorPacking Color;
    enum vDepthPacking Depth;
    enum vStreamFlags Flags;
    int SizeX;
    int SizeY;
    int Delay;                              // only valid for cpu streams

    // information provided by ventuz about the stream

    int FrameRateNum;                       // framerate expected by ventuz
    int FrameRateDen;                       // example: num=60000, den=1001.
    int Synchrone;                          // will stall if frame is not ready

    // extension

};

/****************************************************************************/
/***                                                                      ***/
/***   Common Anc Blobs                                                   ***/
/***                                                                      ***/
/****************************************************************************/

#define vFOURCC(a, b, c, d)  ( ((a&255)<<0)|((b&255)<<8)|((c&255)<<16)|((d&255)<<24) )

enum vAncId
{
    vANC_CameraMatrix           = vFOURCC('C','A','M','R'), // 4x4 matrix, camera space to world space
    vANC_ProjectionMatrix       = vFOURCC('P','R','O','J'), // 4x4 matrix, world space to screen space
};

/****************************************************************************/
/***                                                                      ***/
/***   Functions                                                          ***/
/***                                                                      ***/
/****************************************************************************/

/// <summary>
/// Initialize Library
/// </summary>
/// <param name="DXDev">optional Direct3D 9Ex device</param>
/// <param name="windowHandle">When using VTM_OpenGL and no DX9 device is provided, this window is used to create one. If this is zero, a dummy window is created automatically</param>
/// <returns>VE_Ok, VE_OutOfMemory, VE_NoConnection, VE_NotImplemented</returns>
/// <para>
/// If the library is already initialized, this call is ignored silently
/// </para>
/// <para>
/// Connecting over network will not be implemented in the first version of this SDK.
/// </para>
VIOAPI_CALL enum vError vInit(struct IDirect3DDevice9Ex *DXDev,struct ID3D11Device *DXDev11,void *windowHandle);

/// <summary>
/// Free resources
/// </summary>
/// <returns>VE_Ok, VE_AlreadyOpen </returns>
/// <para>
/// Before calling this, all streams must be closed, otherwise VE_AlreadyOpen is 
/// returned.
/// </para>
/// <para>
/// If the library is already de-initialized, or was never initialized, this call
/// is ignored silently.
/// </para>
VIOAPI_CALL enum vError vExit();

/// <summary>
/// Find on which adapter ventuz currently runs. Will fail if Ventuz is not running
/// </summary>
/// <returns>master adapter ordinal of Ventuz, or -1 in case of error</returns>
/// <para>
/// This can be called before calling vInit()/vInit2() or after calling vExit().
/// </para>
VIOAPI_CALL int vGetVentuzAdapter();

/// <summary>
/// convert error code into string
/// </summary>
/// <<param name="error">error code</param>
/// <returns>constant string representing the error</returns>
VIOAPI_CALL const char *vGetErrorString(enum vError error);

/// <summary>
/// Open a stream
/// </summary>
/// <param name="para">(in) parameters for stream creation</param>
/// <param name="hnd">(out) receives handle to stream</param>
/// <returns><list>
/// <item>VE_Ok, VE_DirectX, VE_OpenGL, VE_OutOfMemory</item>
/// <item>VE_BadParameter: channel number or enum is out of range. </item>
/// <item>VE_RequireContext: gl/dx context/device is required for this packing mode but was not specifired in vInit()</item>
/// <item>VE_BadSize: SizeX/SizeY is out of range. for 422 packing, SizeX must be divisiable by two</item>
/// <item>VE_AlreadyOpen: The requested channel is already open</item>
/// <item>VE_NotImplemented: VM_FromVentuz is implemented in the dummy implementation, but will not be implemented in the first version of the real SDK</item>
/// <item>VE_NotLicensed: License does not cover this kind of stream.</item>
/// <item>VE_BadPacking: packing incompatible with transfer mode / direction</item>
/// <item>VE_AsyncWait: when VSF_Async is set, vOpen() will never return VE_Ok, but in case of success
///       VE_AsyncWait will be return, indicating you need to call vOpenContinueAsync()</item>
/// </list></returns>
VIOAPI_CALL enum vError vOpen(struct vOpenPara *para,vHandle *hnd);

/// <summary>
/// Open a stream, Async
/// </summary>
/// <param name="hnd">handele to stream</param>
/// <returns>same as vOpen().</returns>
/// <para>When VSF_Async was set, vOpen will return VE_AsyncWait in case of success.
/// Now you must call this function until it stops returning VE_AsyncWait to
/// complete the process in a nonblocking way.</para>
VIOAPI_CALL enum vError vOpenContinueAsync(vHandle hnd);

/// <summary>
/// Closes a stream
/// </summary>
/// <param name="hnd">handele to stream</param>
/// <returns>VE_Ok, VE_BadHandle </returns>
VIOAPI_CALL enum vError vClose(vHandle hnd);

/// <summary>
/// Get information about a stream
/// </summary>
/// <param name="hnd">handele to stream</param>
/// <param name="info">(out) receive information</param>
/// <returns>VE_Ok, VE_BadHandle </returns>
VIOAPI_CALL enum vError vGetInfo(vHandle hnd,struct vInfo *info);

/// <summary>
/// Tests if a new frame is ready for locking
/// </summary>
/// <param name="hnd">handele to stream</param>
/// <returns>VE_Ok, VE_BadHandle, VE_NoFrame </returns>
/// <para>
/// VE_NoFrame is returned when no frame is ready. When a device/context is required
/// but not set, VE_NoFrame is returned because vLockFrame() would fail.
/// </para>
VIOAPI_CALL enum vError vHasFrame(vHandle hnd);

/// <summary>
/// Lock frame. Must be matches with vUnlockFrame(). Data of frame may only be
/// accessed between vLockFrame() or vLockFrameEx() and vUnlockFrame()
/// </summary>
/// <param name="hnd">handle to stream</param>
/// <param name="frame">(out) receive information about the buffers of the locked frame</param>
/// <returns><list>
/// <item>VE_Ok, VE_DirectX, VE_OpenGL</item>
/// <item>VE_RequireContext: when vResetGfx was called, you can not lock frames. </item>
/// <item>VE_Locked: Unlock before locking again</item>
/// </list></returns>
/// <para>
///
/// </para>
VIOAPI_CALL enum vError vLockFrame(vHandle hnd,struct vFrame *frame);

/// <summary>
/// Lock frame. Must be matches with vUnlockFrame(). Data of frame may only be
/// accessed between vLockFrame() or vLockFrameEx() and vUnlockFrame()
/// </summary>
/// <param name="hnd">handle to stream</param>
/// <param name="frame">(out) receive information about the buffers of the locked frame</param>
/// <param name="fillBuffers">if set to false color/z-buffers are not immediately filled during this call. call vFillBuffers before accessing buffers.</param>
/// <returns><list>
/// <item>VE_Ok, VE_DirectX, VE_OpenGL</item>
/// <item>VE_RequireContext: when vResetGfx was called, you can not lock frames. </item>
/// <item>VE_Locked: Unlock before locking again</item>
/// </list></returns>
/// <para>
///
/// </para>
VIOAPI_CALL enum vError vLockFrameEx(vHandle hnd,struct vFrame *frame, bool fillBuffers);

/// <summary>
/// Lock frame. Must be matches with vUnlockFrame(). Data of frame may only be
/// accessed between vLockFrame() or vLockFrameEx() and vUnlockFrame()
/// </summary>
/// <param name="hnd">handle to stream</param>
/// <param name="frame">(out) receive information about the buffers of the locked frame</param>
/// <param name="flags">flags.</param>
/// <returns><list>
/// <item>VE_Ok, VE_DirectX, VE_OpenGL</item>
/// <item>VE_RequireContext: when vResetGfx was called, you can not lock frames. </item>
/// <item>VE_Locked: Unlock before locking again</item>
/// </list></returns>
/// <para>
///
/// </para>
VIOAPI_CALL enum vError vLockFrameEx2(vHandle hnd,struct vFrame *frame, vLockFlags flags);

/// <summary>
/// Lock frame. Must be matches with vUnlockFrame(). Data of frame may only be
/// accessed between vLockFrame() or vLockFrameEx() and vUnlockFrame()
/// This call allows to use your own buffer, not one provided by the Vio stream.
/// This means that the buffer will remain valid even after vClose() or vExit(),
/// as long as the same DirectX device was used to create the buffers.
/// </summary>
/// <param name="hnd">handle to stream</param>
/// <param name="frame">(out) receive information about the buffers of the locked frame</param>
/// <param name="flags">flags.</param>
/// <param name="buffer">Buffer created with vCreateBuffer()</param>
/// <returns><list>
/// <item>VE_Ok, VE_DirectX, VE_OpenGL</item>
/// <item>VE_RequireContext: when vResetGfx was called, you can not lock frames. </item>
/// <item>VE_Locked: Unlock before locking again</item>
/// </list></returns>
/// <para>
///
/// </para>
VIOAPI_CALL enum vError vLockFrameUserBuffer(vHandle hnd,struct vFrame *frame, vLockFlags flags,struct vBuffer *buffer);

/// <summary>
/// Does the transfer of color/z-buffer data if it was explicitely forbidden in vLockFrameEx()
/// </summary>
/// <param name="hnd">(int) handle to stream</param>
/// <param name="frame">(in) the frame to fill buffers from</param>
/// <returns><list>
/// <item>VE_Ok</item>
/// <item>VE_BadHandle : vHandle is unknown</item>
/// </list></returns>
VIOAPI_CALL vError vFillBuffers(vHandle hnd,struct vFrame *frame);

/// <summary>
/// Does the part of the transfer usually done in Unlock.
/// </summary>
/// <param name="hnd">(int) handle to stream</param>
/// <returns><list>
/// <item>VE_Ok</item>
/// <item>VE_BadHandle : vHandle is unknown</item>
/// </list></returns>
/// <para>
/// When using multiple streams, it is best first lock all buffers, then call vFillBuffers() for all 
/// buffers, and last call vUnlock() for all buffers. This makes sure that there will be only one
/// GPU stall instead of two.
/// </para>
VIOAPI_CALL vError vFillBuffersEnd(vHandle hnd);

/// <summary>
/// while locked, one packet of Anc can be send to Ventuz
/// </summary>
/// <param name="hnd">handele to stream</param>
/// <param name="id">anc fourcc code</param>
/// <param name="data">(in) pointer to anc</param>
/// <param name="size">size of anc, in bytes</param>
/// <returns>VE_Ok, VE_BadHandle, VE_NotLocked, VE_AncOverflow </returns>
/// <para>
/// The packet is send when unlocking. The data is not copied, the provided data-pointer must remain valid until unlocking.
/// </para>
VIOAPI_CALL enum vError vAncToVentuz(vHandle hnd,vuint fourcc,void *data,int size);

/// <summary>
/// while locked, one packet of anc can be send to Ventuz
/// </summary>
/// <param name="hnd">handele to stream</param>
/// <param name="id">anc fourcc code</param>
/// <param name="index">index if multiple anc blocks of same fourcc are registered</param>
/// <param name="data">(out) will receive a pointer to the anc-blob</param>
/// <param name="size">(out) actual size of the anc, in bytes</param>
/// <param name="remove">(in) if true, remove this packet. if there are multiple packets for the same anc, the indices get reordered. So to get all packets, just try to get index 0 until nothing is found any more.</param>
/// <returns>VE_Ok, VE_BadHandle, VE_NotLocked, VE_AncNotFound </returns>
/// <para>
/// The packet is received while locking and the buffer is valid until unlocked
/// </para>
VIOAPI_CALL enum vError vAncFromVentuz(vHandle hnd,vuint fourcc,int index,void **buffer,int *size,bool remove);

/// <summary>
/// Unlock frame.
/// </summary>
/// <param name="hnd">handele to stream</param>
/// <returns>VE_Ok, VE_BadHandle, VE_NotLocked </returns>
/// <para>
///
/// </para>
VIOAPI_CALL enum vError vUnlockFrame(vHandle hnd);

/// <summary>
/// set delay for output streams
/// </summary>
/// <param name="hnd">handele to stream</param>
/// <returns>VE_Ok, VE_BadHandle, VE_BadParameter </returns>
/// <para>
/// can be called inside or outside lock.
/// Must allocate extra buffers. 
/// only for output streams.
/// </para>
VIOAPI_CALL enum vError vSetDelay(vHandle hnd, int frames);

/// <summary>
/// Get a message to sign for the licence protection mechanism
/// </summary>
/// <param name="hnd">handele to stream</param>
/// <param name="size">size of the message. must be 1024</param>
/// <param name="buffer">buffer to receive message</param>
/// <returns>VE_Ok, VE_BadHandle, VE_BadSize </returns>
/// <para>
/// dummy implementation uses unreliable time-derived random numbers.
/// </para>
VIOAPI_CALL enum vError vGetProtectionMessage(vHandle hnd,int size,unsigned char *buffer);

/// <summary>
/// Set the signature of the message for the license protection mechanism.
/// This may be done only once. If a wrong signature is set, the stream must
/// be closed and opened again to retry. There is no feedback if the license
/// was set correctly.
/// </summary>
/// <param name="hnd">handele to stream</param>
/// <param name="size">size of the message. must be 4</param>
/// <param name="buffer">buffer to receive message</param>
/// <returns>VE_Ok, VE_BadHandle, VE_BadSize </returns>
/// <para>
/// Dummy implementation uses first 4 bytes of message
/// </para>
VIOAPI_CALL enum vError vSetProtectionSignature(vHandle hnd,int size,const unsigned char *buffer);

/****************************************************************************/

/// <summary>
/// used internally for performance monitoring
/// </summary>
/// <param name="enter">callback called to enter a performance monitoring section</param>
/// <param name="leave">callback called when leaving a performance monitoring section</param>
/// <para>
/// enter and leave will be called to form nesting section for performance monitoring.
/// the caller is supposed to record timestamps and draw colorful bars representing
/// various parts of the processing on CPU.
/// </para>
VIOAPI_CALL void vSetPerfMon(void(*enter)(vuint color), void(*leave)());

/****************************************************************************/

/// <summary>
/// Manually create buffer for use with vLockFrameUserBuffer().
/// </summary>
/// <param name="info">Information about the buffer size, format and others.</param>
/// <param name="buffer (out)">return pointer to buffer</param>
/// <returns><list>
/// <item>VE_Ok</item>
/// <item>VE_DirectX : directx error (possibly out of memory)</item>
/// <item>VE_BadParameter : only VTM_Cpu is allowed.</item>
/// <item>VE_BadPacking : unsupported color or depth packing.</item>
/// </list></returns>
VIOAPI_CALL enum vError vCreateBuffer(const vInfo *info, vBuffer **buffer);

/// <summary>
/// Destroy a buffer.
/// </summary>
/// <param name="buffer">buffer to destroy</param>
/// <returns><list>
VIOAPI_CALL enum vError vDestroyBuffer(vBuffer *buffer);

/// <summary>
/// Begin accessing buffer memory
/// </summary>
/// <param name="buffer">buffer</param>
/// <param name="frame (out)">mapping information</param>
/// <returns><list>
VIOAPI_CALL enum vError vMapBuffer(vBuffer *buffer,vFrame *frame);

/// <summary>
/// End accessing buffer memory
/// </summary>
/// <param name="buffer">buffer</param>
/// <returns><list>
VIOAPI_CALL enum vError vUnmapBuffer(vBuffer *buffer);

/****************************************************************************/

#ifdef __cplusplus
}}   // namesapce; extern "C"
#endif


// <summary>
/// NOP Operation. Does nothing. This one is used to create a dependency to this DLL.
/// (Used for Unity3D which needs to know that this DLL is used and has to be copied in the final package.
/// </summary>
extern "C" __declspec(dllexport) void __cdecl vVentuzVioNOP();

#endif  // FILE_VENTUZ_PROJECTS_VIOAPI_VIO_VIO_H
