// bench3d_app.cpp - Series 80 (Nokia 9300) application shell for the R3D renderer benchmark
#include <eikapp.h>
#include <eikdoc.h>
#include <eikappui.h>
#include <eikenv.h>
#include <coecntrl.h>
#include <fbs.h>
#include <w32std.h>
#include <f32file.h>
#include <e32std.h>
#include <cknenv.h>
#include <skin.h>
#include <hal.h>
#include "r3d.h"

const TUid KUidBench3D = { 0x0E3D9300 };

class CBenchView : public CCoeControl, public MDirectScreenAccess
    {
public:
    void ConstructL(const TRect& aRect);
    ~CBenchView();
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
    void Draw(const TRect& aRect) const;
    static TInt IdleCb(TAny* aThis);
    void Frame();
    void SetActive(TBool aOn);
    void SaveResults();
    // MDirectScreenAccess
    void AbortNow(RDirectScreenAccess::TTerminationReasons aReason);
    void Restart(RDirectScreenAccess::TTerminationReasons aReason);
private:
    TBool StartDirect();
    void StopDirect();
    CDirectScreenAccess* iDsa;
    TUint8* iFb;            // first pixel of the framebuffer (0 = not usable)
    TInt iFbStride, iFbBpp;
    TBool iDsaOn, iFullBlit;
    TUint16* iBack;         // off-screen frame for direct mode
    TInt iBorderCopy;
    CFbsBitmap* iBmp;
    CIdle* iIdle;
    RChunk iChunk;
    R3D* iR;
    TUint32 iKeys;
    TTime iLast;
    TInt iLastRender, iLastBlit;
    TBool i4K, iSaved, iRunning;
    };

static TUint32 KeyBit(TInt aScan)
    {
    switch (aScan)
        {
        // Nokia 9300 joystick: 170 left, 171 right, 172 up, 173 down, 174 press
        case EStdKeyLeftArrow: case EStdKeyDevice6: return KEY_LOOK_L;
        case EStdKeyRightArrow: case EStdKeyDevice7: return KEY_LOOK_R;
        case EStdKeyUpArrow: case EStdKeyDevice8: return KEY_LOOK_U;
        case EStdKeyDownArrow: case EStdKeyDevice9: return KEY_LOOK_D;
        case 'W': return KEY_FWD;
        case 'S': return KEY_BACK;
        case 'A': return KEY_STRAFE_L;
        case 'D': return KEY_STRAFE_R;
        case EStdKeyLeftShift: case EStdKeyRightShift: return KEY_RUN;
        case EStdKeySpace: return KEY_JUMP;
        case EStdKeyEnter: case EStdKeyDevice3: case EStdKeyDeviceA: return KEY_FIRE;
        default: return 0;
        }
    }

void CBenchView::ConstructL(const TRect& aRect)
    {
    CreateWindowL();
    SetRect(aRect);
    iBmp = new (ELeave) CFbsBitmap;
    TDisplayMode mode = Window().DisplayMode();
    i4K = (mode == EColor4K);
    User::LeaveIfError(iBmp->Create(TSize(OUT_W, OUT_H), i4K ? EColor4K : EColor64K));
    TInt need = R3D::MemNeeded() + sizeof(R3D) + 64 + OUT_W * OUT_H * 2 + 64;
    User::LeaveIfError(iChunk.CreateLocal(need, need));
    TUint8* base = iChunk.Base();
    Mem::FillZ(base, need);
    iR = (R3D*)base;
    iBack = (TUint16*)(base + ((R3D::MemNeeded() + sizeof(R3D) + 64 + 63) & ~63));
    iR->Init(base + ((sizeof(R3D) + 63) & ~63));
    iIdle = CIdle::NewL(CActive::EPriorityIdle);
    // framebuffer for direct output
    TInt addr = 0, off = 0, w = 0, h = 0;
    iFbBpp = 0; iFbStride = 0;
    if (HAL::Get(HALData::EDisplayMemoryAddress, addr) == KErrNone &&
        HAL::Get(HALData::EDisplayOffsetToFirstPixel, off) == KErrNone &&
        HAL::Get(HALData::EDisplayOffsetBetweenLines, iFbStride) == KErrNone &&
        HAL::Get(HALData::EDisplayBitsPerPixel, iFbBpp) == KErrNone &&
        HAL::Get(HALData::EDisplayXPixels, w) == KErrNone &&
        HAL::Get(HALData::EDisplayYPixels, h) == KErrNone &&
        addr && (iFbBpp == 16 || iFbBpp == 12) && w >= OUT_W && h >= OUT_H && iFbStride >= OUT_W * 2 && !(iFbStride & 3))
        {
        iFb = (TUint8*)addr + off;
        if ((TUint32)iFb & 3) iFb = 0;
        }
    iR->SetDirectInfo(0, iFbBpp, iFbStride);
    if (iFb) iDsa = CDirectScreenAccess::NewL(iCoeEnv->WsSession(), *iCoeEnv->ScreenDevice(), Window(), *this);
    ActivateL();
    SetActive(ETrue);
    }

CBenchView::~CBenchView()
    {
    if (iIdle) iIdle->Cancel();
    delete iIdle;
    if (iDsa) iDsa->Cancel();
    delete iDsa;
    delete iBmp;
    iChunk.Close();
    }

void CBenchView::SetActive(TBool aOn)
    {
    if (aOn && !iRunning)
        {
        iLast.UniversalTime();
        iIdle->Start(TCallBack(IdleCb, this));
        iRunning = ETrue;
        }
    else if (!aOn && iRunning)
        {
        iIdle->Cancel();
        iRunning = EFalse;
        iKeys = 0;
        StopDirect();
        }
    }

TInt CBenchView::IdleCb(TAny* aThis)
    {
    ((CBenchView*)aThis)->Frame();
    return ETrue;
    }

void CBenchView::Frame()
    {
    TTime t0;
    t0.UniversalTime();
    TInt dt = t0.MicroSecondsFrom(iLast).Int64().Low();
    iLast = t0;
    iR->ReportTimes(iLastRender, iLastBlit);
    iR->Tick(dt, iKeys);

    TBool direct = iR->WantDirect() && iFb && StartDirect();
    if (!direct) StopDirect();
    iR->SetDirectInfo(direct, iFbBpp, iFbStride);
    if (direct)
        {
        // render the whole frame off-screen (fast cached RAM), then copy it to the screen in one burst
        iR->Render(iBack, OUT_W);
        TTime t1;
        t1.UniversalTime();
        TInt x0 = iR->OutX0(), w = iR->OutW();
        if (iR->iBorderFrames > 0 || iBorderCopy > 0) { x0 = 0; w = OUT_W; }
        iBorderCopy = iR->iBorderFrames;
        TInt y;
        for (y = 0; y < OUT_H; y++)
            {
            TUint16* src = iBack + y * OUT_W + x0;
            TUint16* dst = (TUint16*)(iFb + y * iFbStride) + x0;
            if (iFbBpp == 12)
                {
                TInt x;
                for (x = 0; x < w; x++)
                    {
                    TUint16 c = src[x];
                    dst[x] = (TUint16)(((c >> 4) & 0xF00) | ((c >> 3) & 0xF0) | ((c >> 1) & 0xF));
                    }
                }
            else Mem::Copy(dst, src, w * 2);
            }
        iDsa->ScreenDevice()->Update();
        TTime t2;
        t2.UniversalTime();
        iLastRender = t1.MicroSecondsFrom(t0).Int64().Low();
        iLastBlit = t2.MicroSecondsFrom(t1).Int64().Low();
        User::ResetInactivityTime();
        if (iR->BenchRunning() == 2 && !iSaved) { SaveResults(); iSaved = ETrue; }
        if (iR->BenchRunning() == 1) iSaved = EFalse;
        return;
        }
    iBmp->LockHeap();
    TUint16* p = (TUint16*)iBmp->DataAddress();
    TInt stride = CFbsBitmap::ScanLineLength(OUT_W, i4K ? EColor4K : EColor64K) / 2;
    iR->Render(p, stride);
    if (i4K)
        {
        TInt n;
        for (n = 0; n < OUT_H * stride; n++)
            {
            TUint16 c = p[n];
            p[n] = (TUint16)(((c >> 4) & 0xF00) | ((c >> 3) & 0xF0) | ((c >> 1) & 0xF));
            }
        }
    iBmp->UnlockHeap();
    TTime t1;
    t1.UniversalTime();
    if (iR->iNative && !iFullBlit)
        {
        // blit only the 320x200 centre window
        TRect r(iR->OutX0(), 0, iR->OutX0() + iR->OutW(), OUT_H);
        Window().Invalidate(r);
        ActivateGc();
        Window().BeginRedraw(r);
        SystemGc().BitBlt(r.iTl, iBmp, r);
        Window().EndRedraw();
        DeactivateGc();
        }
    else DrawNow();
    iFullBlit = iR->iBorderFrames > 0;
    iCoeEnv->WsSession().Flush();
    TTime t2;
    t2.UniversalTime();
    iLastRender = t1.MicroSecondsFrom(t0).Int64().Low();
    iLastBlit = t2.MicroSecondsFrom(t1).Int64().Low();
    User::ResetInactivityTime();
    if (iR->BenchRunning() == 2 && !iSaved) { SaveResults(); iSaved = ETrue; }
    if (iR->BenchRunning() == 1) iSaved = EFalse;
    }

TBool CBenchView::StartDirect()
    {
    if (iDsaOn) return ETrue;
    if (!iDsa) return EFalse;
    TRAPD(err, iDsa->StartL());
    if (err != KErrNone) return EFalse;
    // only draw directly when our window is fully visible
    RRegion* reg = iDsa->DrawingRegion();
    if (!reg || reg->Count() != 1 || reg->BoundingRect().Width() < OUT_W || reg->BoundingRect().Height() < OUT_H)
        {
        iDsa->Cancel();
        return EFalse;
        }
    iDsaOn = ETrue;
    return ETrue;
    }

void CBenchView::StopDirect()
    {
    if (iDsaOn) { iDsa->Cancel(); iDsaOn = EFalse; DrawDeferred(); }
    }

void CBenchView::AbortNow(RDirectScreenAccess::TTerminationReasons /*aReason*/)
    {
    iDsaOn = EFalse;
    }

void CBenchView::Restart(RDirectScreenAccess::TTerminationReasons /*aReason*/)
    {
    // re-started lazily on the next frame
    }

void CBenchView::SaveResults()
    {
    TBuf8<1024> buf;
    buf.SetLength(buf.MaxLength());
    TInt len = iR->ResultsText((char*)buf.Ptr(), buf.MaxLength());
    buf.SetLength(len);
    RFs& fs = iCoeEnv->FsSession();
    RFile f;
    _LIT(KFile, "C:\\bench3d.txt");
    if (f.Replace(fs, KFile, EFileWrite) == KErrNone)
        {
        f.Write(buf);
        f.Close();
        }
    }

void CBenchView::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    gc.BitBlt(TPoint(0, 0), iBmp);
    }

TKeyResponse CBenchView::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    if (aType == EEventKeyDown)
        {
        iKeys |= KeyBit(aKeyEvent.iScanCode);
        iR->SetDebugKey(aKeyEvent.iScanCode);
        }
    else if (aType == EEventKeyUp)
        iKeys &= ~KeyBit(aKeyEvent.iScanCode);
    else if (aType == EEventKey)
        {
        if (aKeyEvent.iCode == EKeyEscape)
            {
            iEikonEnv->EikAppUi()->HandleCommandL(EEikCmdExit);
            return EKeyWasConsumed;
            }
        iR->KeyPress(aKeyEvent.iCode);
        }
    return EKeyWasConsumed;
    }

// ---------------------------------------------------------------- app framework
class CBenchAppUi : public CEikAppUi
    {
public:
    void ConstructL();
    ~CBenchAppUi();
    void HandleCommandL(TInt aCommand);
    void HandleForegroundEventL(TBool aForeground);
private:
    CBenchView* iView;
    };

void CBenchAppUi::ConstructL()
    {
    BaseConstructL();
    CknEnv::Skin().SetAppViewType(ESkinAppViewNoCbaNoToolband);
    iView = new (ELeave) CBenchView;
    iView->ConstructL(ApplicationRect());
    AddToStackL(iView);
    }

CBenchAppUi::~CBenchAppUi()
    {
    if (iView)
        {
        RemoveFromStack(iView);
        delete iView;
        }
    }

void CBenchAppUi::HandleCommandL(TInt aCommand)
    {
    if (aCommand == EEikCmdExit) Exit();
    }

void CBenchAppUi::HandleForegroundEventL(TBool aForeground)
    {
    CEikAppUi::HandleForegroundEventL(aForeground);
    if (iView) iView->SetActive(aForeground);
    }

class CBenchDocument : public CEikDocument
    {
public:
    CBenchDocument(CEikApplication& aApp) : CEikDocument(aApp) {}
    CEikAppUi* CreateAppUiL() { return new (ELeave) CBenchAppUi; }
    };

class CBenchApplication : public CEikApplication
    {
    CApaDocument* CreateDocumentL() { return new (ELeave) CBenchDocument(*this); }
    TUid AppDllUid() const { return KUidBench3D; }
    };

EXPORT_C CApaApplication* NewApplication() { return new CBenchApplication; }
GLDEF_C TInt E32Dll(TDllReason) { return KErrNone; }
