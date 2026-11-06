// hrimfell_app.cpp - Series 80 (Nokia 9300) application shell for Hrimfell
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
#include <es_sock.h>
#include <in_sock.h>
#include <apgcli.h>
#include "game.h"

const TUid KUidHrimfell = { 0x0E3D9301 };
_LIT(KSaveFile, "C:\\System\\Apps\\Hrimfell\\hrimfell.sav");
_LIT(KLandFile, "?:\\System\\Apps\\Hrimfell\\hrimfell.land");     // Skyrim's land (installed on any drive)

// the land file's reader for the game (R3D::LoadLand): aCtx is the open RFile
static int AppLandRead(void* aCtx, u8* aBuf, int aLen)
    {
    TPtr8 p(aBuf, 0, aLen);
    if (((RFile*)aCtx)->Read(p) != KErrNone) return 0;
    return p.Length();
    }
// the game's other data files (riverwood.har ...): beside the program, on whichever drive it was installed to;
// aCtx is the file server session
static int AppDataRead(void* aCtx, const char* aName, int aPos, u8* aBuf, int aLen)
    {
    RFs& fs = *(RFs*)aCtx;
    TBuf<96> name;
    _LIT(KDir, "?:\\System\\Apps\\Hrimfell\\");
    TInt d, k;
    RFile f;
    for (d = 0; d < 4; d++)
        {
        name.Copy(KDir);
        name[0] = (TText)"CEDZ"[d];
        for (k = 0; aName[k] && name.Length() < name.MaxLength(); k++) name.Append((TChar)(TUint8)aName[k]);
        if (f.Open(fs, name, EFileRead | EFileShareReadersOnly) == KErrNone) break;
        }
    if (d == 4) return 0;
    TInt pos = aPos, got = 0;
    if (f.Seek(ESeekStart, pos) == KErrNone)
        {
        TPtr8 p(aBuf, 0, aLen);
        if (f.Read(p) == KErrNone) got = p.Length();
        }
    f.Close();
    return got;
    }
// (the land is read from again as the player goes: the file stays open, and is read from where the tile is)
static int AppLandSeek(void* aCtx, int aPos)
    {
    TInt pos = aPos;
    return ((RFile*)aCtx)->Seek(ESeekStart, pos) == KErrNone;
    }

// ---------------------------------------------------------------- the update from the local server (v0.4.58)
// The menu's "Update from server": http://192.168.137.1:8000/skyrim.sis (the PC's server on the USB link) read with a
// plain HTTP/1.0 GET into a file on the drive the game is installed on (System\Temp\skyrim.sis), handed to the
// system installer, and the game closes so its files can be replaced. (v0.4.60) As fast as the link: the socket's
// receive window asked for 64 KB (the stack's 8 KB stalls the PC while the phone writes); the body received straight
// into the free tail of one of two 64 KB buffers while the other is written to the file asynchronously, so the
// network and the flash overlap; no write smaller than 64 KB but the last, no Flush between, no SetSize. The game
// keeps running meanwhile; progress every MB as the game's messages.
_LIT8(KUpdHost, "192.168.137.1");
const TInt KUpdPort = 8000;
_LIT8(KUpdPath, "/skyrim.sis");
_LIT(KUpdTemp, "?:\\System\\Temp\\");
_LIT(KUpdName, "skyrim.sis");
const TInt KUpdBuf = 65536;
class CUpdater;
// (v0.4.64) the update's own screen: the game stopped while it runs (its loop took the phone, and the USB link's
// keep-alive with it), the download's lines shown, Esc cancelling back to the game
class MUpdShow
    {
public:
    virtual void UpdLine(const TDesC8& aText) = 0;
    virtual void UpdDone() = 0;            // the installer has the package: the game ends
    };
// the file's writes, one at a time, in the background
class CUpdWriter : public CActive
    {
public:
    CUpdWriter(CUpdater& aOwner) : CActive(EPriorityStandard), iOwner(aOwner) { CActiveScheduler::Add(this); }
    ~CUpdWriter() { Cancel(); }
    void Write(RFile& aFile, const TDesC8& aData) { aFile.Write(aData, iStatus); SetActive(); }
    TInt iErr;
private:
    void RunL();
    void DoCancel() {}           // (a file write cannot be cancelled: it completes)
    CUpdater& iOwner;
    };
class CUpdater : public CActive
    {
public:
    CUpdater(Game* aGame, MUpdShow* aShow) : CActive(EPriorityStandard), iGame(aGame), iShow(aShow), iTail(0, 0) { CActiveScheduler::Add(this); }
    void Abort() { if (Busy()) { CloseAll(); if (iFsOpen && iPath.Length()) iFs.Delete(iPath); } }
    ~CUpdater()
        {
        Cancel();
        if (iWriter && iWriter->IsActive()) User::WaitForRequest(iWriter->iStatus);
        delete iWriter;
        CloseAll();
        delete iBuf[0]; delete iBuf[1]; delete iRecv;
        if (iServOpen) iServ.Close();
        if (iFsOpen) iFs.Close();
        }
    TBool Busy() const { return iState != EIdle; }
    void Start(TChar aDrive)
        {
        if (Busy()) { Say("Update: already downloading."); return; }
        TRAPD(err, StartL(aDrive));
        if (err != KErrNone) Fail(err, "could not start");
        }
    void Written(TInt aErr)
        {
        // (the background write finished: a full buffer waiting for it goes now)
        if (aErr != KErrNone) { Fail(aErr, "could not write the file (is the drive full?)"); return; }
        if (iState == EWaitWrite) { SwapAndWrite(); Recv(); }
        else if (iState == EFinishing) Finish();
        }
    TBool iDone;                // the file is complete and the installer was started: the game closes
private:
    enum TState { EIdle, EConnecting, ESending, EHeaders, EReceiving, EWaitWrite, EFinishing };
    void Say(const char* aText) { iShow->UpdLine(TPtrC8((const TUint8*)aText)); }
    void SayF(const char* aFmt, TInt a, TInt b)
        {
        TBuf8<64> t;
        TPtrC8 f((const TUint8*)aFmt);
        t.Format(f, a, b);
        iShow->UpdLine(t);
        }
    void StartL(TChar aDrive)
        {
        if (!iFsOpen) { User::LeaveIfError(iFs.Connect()); iFsOpen = ETrue; }
        if (!iServOpen) { User::LeaveIfError(iServ.Connect()); iServOpen = ETrue; }
        if (!iRecv) iRecv = HBufC8::NewL(16384);
        if (!iBuf[0]) iBuf[0] = HBufC8::NewL(KUpdBuf);
        if (!iBuf[1]) iBuf[1] = HBufC8::NewL(KUpdBuf);
        if (!iWriter) iWriter = new (ELeave) CUpdWriter(*this);
        iPath.Copy(KUpdTemp); iPath[0] = (TText)aDrive;
        iFs.MkDirAll(iPath);
        iPath.Append(KUpdName);
        User::LeaveIfError(iFile.Replace(iFs, iPath, EFileWrite)); iFileOpen = ETrue;
        User::LeaveIfError(iSock.Open(iServ, KAfInet, KSockStream, KProtocolInetTcp)); iSockOpen = ETrue;
        // a big receive window: the PC keeps sending while the phone writes (before Connect)
        if (iSock.SetOpt(KSoTcpRecvWinSize, KSolInetTcp, 65536) != KErrNone) iSock.SetOpt(KSORecvBuf, KSOLSocket, 65536);
        TInetAddr addr;
        TBuf<32> host; host.Copy(KUpdHost);
        User::LeaveIfError(addr.Input(host));
        addr.SetPort(KUpdPort);
        iHead.Zero(); iBuf[0]->Des().Zero(); iBuf[1]->Des().Zero(); iCur = 0;
        iGot = 0; iTotal = -1; iShown = 0; iDone = EFalse; iEof = EFalse;
        iState = EConnecting;
        iSock.Connect(addr, iStatus);
        SetActive();
        Say("Update: connecting to 192.168.137.1:8000 ...");
        }
    void DoCancel() { if (iSockOpen) iSock.CancelAll(); }
    void RunL()
        {
        TInt err = iStatus.Int();
        TBool eof = err == KErrEof && (iState == EHeaders || iState == EReceiving);
        if (err != KErrNone && !eof) { Fail(err, iState == EConnecting ? "no server (is it running?)" : "the connection broke"); return; }
        switch (iState)
            {
            case EConnecting:
                iReq.Zero();
                iReq.Append(_L8("GET ")); iReq.Append(KUpdPath);
                iReq.Append(_L8(" HTTP/1.0\r\nHost: ")); iReq.Append(KUpdHost);
                iReq.Append(_L8("\r\nConnection: close\r\n\r\n"));
                iState = ESending;
                iSock.Write(iReq, iStatus);
                SetActive();
                break;
            case ESending:
                iState = EHeaders;
                { TPtr8 d = iRecv->Des(); d.Zero(); iSock.RecvOneOrMore(d, 0, iStatus, iLen); SetActive(); }
                break;
            case EHeaders:
                {
                TPtr8 d = iRecv->Des();
                if (eof || d.Length() == 0) { Fail(KErrEof, "the server closed before sending the file"); return; }
                Headers(d);
                if (iState != EHeaders) { if (iState == EReceiving) Next(); return; }
                d.Zero(); iSock.RecvOneOrMore(d, 0, iStatus, iLen); SetActive();
                break;
                }
            case EReceiving:
                {
                TPtr8 cur = iBuf[iCur]->Des();
                cur.SetLength(cur.Length() + iTail.Length());
                iGot += iTail.Length();
                Progress();
                if (eof || iTail.Length() == 0) { iEof = ETrue; iState = EFinishing; if (!iWriter->IsActive()) Finish(); return; }
                Next();
                break;
                }
            default: break;
            }
        }
    TInt RunError(TInt aError) { Fail(aError, "could not write the file (is the drive full?)"); return KErrNone; }
    // the status line and headers (to the empty line); the body's first bytes into the first buffer
    void Headers(const TDesC8& aData)
        {
        TInt before = iHead.Length();
        iHead.Append(aData.Left(Min(aData.Length(), iHead.MaxLength() - before)));
        TInt end = iHead.Find(_L8("\r\n\r\n"));
        if (end < 0) { if (iHead.Length() == iHead.MaxLength()) Fail(KErrCorrupt, "the server's reply is not HTTP"); return; }
        if (iHead.Length() < 12 || iHead.Mid(9, 3) != _L8("200")) { Fail(KErrNotFound, "no skyrim.sis on the server"); return; }
        TBuf8<2048> lower; lower.Copy(iHead.Left(end)); lower.LowerCase();
        TInt cl = lower.Find(_L8("content-length:"));
        if (cl >= 0)
            {
            TLex8 lex(lower.Mid(cl + 15)); lex.SkipSpace();
            TInt v = 0; if (lex.Val(v) == KErrNone && v > 0) iTotal = v;
            }
        TPtrC8 body = aData.Mid(Max(0, end + 4 - before));
        iBuf[0]->Des().Copy(body);
        iGot = body.Length();
        iState = EReceiving;
        }
    // the next read: into the free tail of the current buffer; a full one goes to the file first
    void Next()
        {
        if (iBuf[iCur]->Length() >= KUpdBuf)
            {
            if (iWriter->IsActive()) { iState = EWaitWrite; return; }       // (the other buffer still being written)
            SwapAndWrite();
            }
        Recv();
        }
    void SwapAndWrite()
        {
        iWriter->Write(iFile, *iBuf[iCur]);
        iCur ^= 1;
        iBuf[iCur]->Des().Zero();
        iState = EReceiving;
        }
    void Recv()
        {
        TPtr8 cur = iBuf[iCur]->Des();
        iTail.Set((TUint8*)cur.Ptr() + cur.Length(), 0, cur.MaxLength() - cur.Length());
        iSock.RecvOneOrMore(iTail, 0, iStatus, iLen);
        SetActive();
        }
    void Progress()
        {
        if (iGot - iShown < 256 * 1024) return;
        iShown = iGot;
        if (iTotal > 0) SayF("Update: %d of %d KB", iGot / 1024, iTotal / 1024);
        else SayF("Update: %d KB%c", iGot / 1024, ' ');
        }
    // the last buffer written (after the one being written), the file closed, the installer started
    void Finish()
        {
        TInt err = iWriter->iErr;
        if (err == KErrNone && iBuf[iCur]->Length()) err = iFile.Write(*iBuf[iCur]);
        iBuf[iCur]->Des().Zero();
        CloseAll();
        if (err != KErrNone) { Fail(err, "could not write the file (is the drive full?)"); return; }
        if (iGot == 0 || (iTotal > 0 && iGot != iTotal)) { Fail(KErrEof, "the download was cut short"); return; }
        SayF("Update: %d KB read. Installing ...%c", iGot / 1024, ' ');
        RApaLsSession ls;
        err = ls.Connect();
        if (err == KErrNone)
            {
            TThreadId tid;
            err = ls.StartDocument(iPath, tid);
            ls.Close();
            }
        if (err != KErrNone) { SayF("Update: the installer did not start (%d)%c", err, ' '); Say("The file is in System\\Temp\\skyrim.sis."); return; }
        iDone = ETrue;          // (the view closes the game: a running program cannot be replaced)
        iShow->UpdDone();
        }
    void CloseAll()
        {
        Cancel();               // (a read still waiting: given up, its completion taken here)
        if (iSockOpen) { iSock.Close(); iSockOpen = EFalse; }
        if (iWriter && iWriter->IsActive()) User::WaitForRequest(iWriter->iStatus);
        if (iFileOpen) { iFile.Close(); iFileOpen = EFalse; }
        iState = EIdle;
        }
    void Fail(TInt aErr, const char* aWhy)
        {
        CloseAll();
        if (iFsOpen && iPath.Length()) iFs.Delete(iPath);
        TBuf8<96> t; t.Copy(_L8("Update failed: ")); t.Append(TPtrC8((const TUint8*)aWhy)); t.AppendFormat(_L8(" (%d)"), aErr);
        iShow->UpdLine(t);
        }
    Game* iGame;
    MUpdShow* iShow;
    RFs iFs; RSocketServ iServ; RSocket iSock; RFile iFile;
    TBool iFsOpen, iServOpen, iSockOpen, iFileOpen, iEof;
    TState iState;
    TBuf<64> iPath;
    TBuf8<160> iReq;
    TBuf8<2048> iHead;
    HBufC8* iRecv;              // (the reply's first bytes: its headers)
    HBufC8* iBuf[2]; TInt iCur; // (the body: one filling, the other being written)
    TPtr8 iTail;                // (the filling one's free part: what the socket reads into)
    CUpdWriter* iWriter;
    TSockXfrLength iLen;
    TInt iGot, iTotal, iShown;
    };
void CUpdWriter::RunL() { iErr = iStatus.Int(); iOwner.Written(iErr); }

// ---------------------------------------------------------------- the benchmark's log to the PC (v0.4.82)
// The menu's Benchmark ends with its log in the game's memory: saved to C:\Data\Hrimfell\bench.txt and POSTed to the
// PC's local server as Maps 9300's log is (http://192.168.137.1:8000/results?name=hrimfell_bench: the server keeps it
// in its uploads folder). The game goes on meanwhile; the outcome is one of its messages.
_LIT(KBenchDir, "C:\\Data\\Hrimfell\\");
_LIT(KBenchFile, "C:\\Data\\Hrimfell\\bench.txt");
class CBenchSender : public CActive
    {
public:
    CBenchSender(Game* aGame) : CActive(EPriorityStandard), iGame(aGame) { CActiveScheduler::Add(this); }
    ~CBenchSender() { Cancel(); CloseSock(); delete iBody; if (iServOpen) iServ.Close(); }
    void Send(const TDesC8& aLog)
        {
        if (iState != EIdle) { iGame->Msg("Benchmark: still sending the one before"); return; }
        delete iBody;
        iBody = HBufC8::New(aLog.Length() + 256);
        if (!iBody) { iGame->Msg("Benchmark: no memory to send it"); return; }
        TPtr8 b = iBody->Des();
        b.Format(_L8("POST /results?name=hrimfell_bench HTTP/1.0\r\nHost: 192.168.137.1:8000\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %d\r\nConnection: close\r\n\r\n"), aLog.Length());
        b.Append(aLog);
        if (!iServOpen) { if (iServ.Connect() != KErrNone) { Fail(KErrNotReady); return; } iServOpen = ETrue; }
        if (iSock.Open(iServ, KAfInet, KSockStream, KProtocolInetTcp) != KErrNone) { Fail(KErrNotReady); return; }
        iSockOpen = ETrue;
        TInetAddr addr;
        TBuf<32> host; host.Copy(KUpdHost);
        addr.Input(host);
        addr.SetPort(KUpdPort);
        iState = EConnecting;
        iSock.Connect(addr, iStatus);
        SetActive();
        iGame->Msg("Benchmark: sending the log to the PC...");
        }
private:
    enum TState { EIdle, EConnecting, ESending, EReply };
    void DoCancel() { if (iSockOpen) iSock.CancelAll(); }
    void RunL()
        {
        TInt err = iStatus.Int();
        if (err != KErrNone && !(iState == EReply && err == KErrEof && iReply.Length())) { Fail(err); return; }
        switch (iState)
            {
            case EConnecting: iState = ESending; iSock.Write(*iBody, iStatus); SetActive(); break;
            case ESending: iState = EReply; iReply.Zero(); iSock.RecvOneOrMore(iReply, 0, iStatus, iLen); SetActive(); break;
            case EReply:
                {
                CloseSock();
                if (iReply.Length() >= 12 && iReply.Mid(9, 3) == _L8("200")) iGame->Msg("Benchmark: sent to the PC (uploads)");
                else
                    {
                    char m[64]; TPtr8 t((TUint8*)m, 0, 60);
                    t.Copy(_L8("Benchmark: the PC answered "));
                    t.Append(iReply.Left(Min(iReply.Length(), 24)));
                    t.Append(TChar(0));
                    TInt k; for (k = 0; m[k]; k++) if (m[k] == '\r' || m[k] == '\n') { m[k] = 0; break; }
                    iGame->Msg(m);
                    }
                break;
                }
            default: break;
            }
        }
    TInt RunError(TInt aError) { Fail(aError); return KErrNone; }
    void Fail(TInt aErr)
        {
        CloseSock();
        char m[64]; TPtr8 t((TUint8*)m, 0, 60);
        t.Format(_L8("Benchmark: not sent (%d) - is the server on?"), aErr);
        t.Append(TChar(0));
        iGame->Msg(m);
        iGame->Msg("The log is in C:\\Data\\Hrimfell\\bench.txt");
        }
    void CloseSock() { if (iSockOpen) { iSock.CancelAll(); iSock.Close(); iSockOpen = EFalse; } iState = EIdle; }
    Game* iGame;
    RSocketServ iServ; RSocket iSock;
    TBool iServOpen, iSockOpen;
    TState iState;
    HBufC8* iBody;
    TBuf8<64> iReply;
    TSockXfrLength iLen;
    };

class CGameView : public CCoeControl, public MDirectScreenAccess, public MUpdShow
    {
public:
    void UpdLine(const TDesC8& aText);
    void UpdDone();
    void ConstructL(const TRect& aRect);
    ~CGameView();
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
    void Draw(const TRect& aRect) const;
    static TInt IdleCb(TAny* aThis);
    void Frame();
    void SetActive(TBool aOn);
    void AbortNow(RDirectScreenAccess::TTerminationReasons aReason);
    void Restart(RDirectScreenAccess::TTerminationReasons aReason);
private:
    TBool StartDirect();
    void StopDirect();
    void HandleRequests();
    CFbsBitmap* iBmp;
    CIdle* iIdle;
    RChunk iChunk;
    Game* iGame;
    TUint32 iKeys;
    TTime iLast;
    TInt iLastRender, iLastBlit;
    TBool i4K, iRunning, iReady;
    CDirectScreenAccess* iDsa;
    TUint8* iFb;
    TInt iFbStride, iFbBpp;
    TBool iDsaOn, iFullBlit;
    TUint16* iBack;
    TInt iBorderCopy;
    TInt iInitPhase;
    TInt iChunkSize;
    TInt iRamTick;
    RFile iLand;                // the land file: open while the game runs
    CUpdater* iUpdater;         // (v0.4.58) the menu's update from the local server
    CBenchSender* iBenchSender; // (v0.4.82) the benchmark's log to the PC
    TBool iUpdating;            // (v0.4.64) its screen up, the game stopped
    TBuf<96> iUpdText[4];
    TBool iLandOpen;
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
        case 'E': return KEY_USE;
        case EStdKeyLeftShift: case EStdKeyRightShift: return KEY_RUN;
        case EStdKeySpace: return KEY_JUMP;
        case EStdKeyEnter: case EStdKeyDevice3: case EStdKeyDeviceA: return KEY_FIRE;
        case EStdKeyLeftCtrl: case EStdKeyRightCtrl: return KEY_CROUCH;
        default: return 0;
        }
    }

void CGameView::ConstructL(const TRect& aRect)
    {
    CreateWindowL();
    SetRect(aRect);
    iBmp = new (ELeave) CFbsBitmap;
    TDisplayMode mode = Window().DisplayMode();
    i4K = (mode == EColor4K);
    User::LeaveIfError(iBmp->Create(TSize(OUT_W, OUT_H), i4K ? EColor4K : EColor64K));
    TInt gameMem = Game::MemNeeded() + ((sizeof(Game) + 63) & ~63);
    TInt need = gameMem + OUT_W * OUT_H * 2 + 128;
    iChunkSize = need;
    User::LeaveIfError(iChunk.CreateLocal(need, need));
    TUint8* base = iChunk.Base();
    Mem::FillZ(base, need);
    iGame = (Game*)base;
    iGame->iDataRead = AppDataRead; iGame->iDataCtx = &iCoeEnv->FsSession();     // riverwood.har ...
    iGame->iDeferLoads = 1;                     // (v0.4.74) a loading screen while an area's file is read
    iBack = (TUint16*)(base + ((gameMem + 63) & ~63));
    iIdle = CIdle::NewL(CActive::EPriorityIdle);
    TInt addr = 0, off = 0, w = 0, h = 0;
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
    if (iFb) iDsa = CDirectScreenAccess::NewL(iCoeEnv->WsSession(), *iCoeEnv->ScreenDevice(), Window(), *this);
    ActivateL();
    SetActive(ETrue);
    }


CGameView::~CGameView()
    {
    if (iIdle) iIdle->Cancel();
    delete iIdle;
    if (iDsa) iDsa->Cancel();
    delete iDsa;
    delete iBmp;
    if (iLandOpen) iLand.Close();
    delete iUpdater;
    delete iBenchSender;
    iChunk.Close();
    }

void CGameView::SetActive(TBool aOn)
    {
    if (iUpdating && aOn) return;           // (v0.4.64: not while the update has the phone)
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

TInt CGameView::IdleCb(TAny* aThis)
    {
    CGameView* v = (CGameView*)aThis;
    if (!v->iReady)
        {
        // build the world in stages: even passes draw the stage about to run, odd passes run it
        TInt phase = v->iInitPhase++;
        if (!(phase & 1)) { v->DrawNow(); return ETrue; }
        TInt step = phase / 2;
        TUint8* base = v->iChunk.Base();
        TBool open = EFalse;
        if (step == 4 && !v->iLandOpen)
            {
            // the land: on the drive it was installed to (C:, then the memory card)
            RFs& fs = v->iCoeEnv->FsSession();
            TBuf<64> name(KLandFile);
            TInt d;
            for (d = 0; d < 4 && !open; d++)
                {
                name[0] = (TText)"CEDZ"[d];
                if (v->iLand.Open(fs, name, EFileRead | EFileShareReadersOnly) == KErrNone) open = ETrue;
                }
            if (open)
                {
                v->iLandOpen = ETrue;
                v->iGame->iLandRead = AppLandRead; v->iGame->iLandSeek = AppLandSeek; v->iGame->iLandCtx = &v->iLand;
                }
            }
        v->iGame->InitStep(base + ((sizeof(Game) + 63) & ~63), step);
        if (step + 1 >= Game::INIT_STEPS)
            {
            v->iReady = ETrue;
            v->iLast.UniversalTime();
            }
        return ETrue;
        }
    v->Frame();
    return ETrue;
    }

TBool CGameView::StartDirect()
    {
    if (iDsaOn) return ETrue;
    if (!iDsa) return EFalse;
    TRAPD(err, iDsa->StartL());
    if (err != KErrNone) return EFalse;
    RRegion* reg = iDsa->DrawingRegion();
    if (!reg || reg->Count() != 1 || reg->BoundingRect().Width() < OUT_W || reg->BoundingRect().Height() < OUT_H)
        {
        iDsa->Cancel();
        return EFalse;
        }
    iDsaOn = ETrue;
    return ETrue;
    }

void CGameView::StopDirect()
    {
    if (iDsaOn) { iDsa->Cancel(); iDsaOn = EFalse; DrawDeferred(); }
    }

void CGameView::AbortNow(RDirectScreenAccess::TTerminationReasons /*aReason*/) { iDsaOn = EFalse; }
void CGameView::Restart(RDirectScreenAccess::TTerminationReasons /*aReason*/) {}

void CGameView::HandleRequests()
    {
    if (iUpdater && iUpdater->iDone)
        {
        // (v0.4.58) the installer has the new package: the game closes so it can replace it
        iUpdater->iDone = EFalse;
        iEikonEnv->EikAppUi()->HandleCommandL(EEikCmdExit);
        return;
        }
    TInt req = iGame->iRequest;
    if (!req) return;
    RFs& fs = iCoeEnv->FsSession();
    if (req == 1)
        {
        RFile f;
        TInt ok = 0;
        if (f.Replace(fs, KSaveFile, EFileWrite) == KErrNone)
            {
            TPtrC8 data((const TUint8*)&iGame->iS, sizeof(GameState));
            ok = f.Write(data) == KErrNone;
            f.Close();
            }
        iGame->SaveDone(ok);
        }
    else if (req == 2)
        {
        RFile f;
        TInt ok = 0;
        if (f.Open(fs, KSaveFile, EFileRead) == KErrNone)
            {
            TPtr8 data((TUint8*)&iGame->iLoad, sizeof(GameState));
            ok = f.Read(data, sizeof(GameState)) == KErrNone && data.Length() == (TInt)sizeof(GameState);
            f.Close();
            }
        iGame->LoadDone(ok);
        }
    else if (req == 3)
        {
        iGame->iRequest = 0;
        iEikonEnv->EikAppUi()->HandleCommandL(EEikCmdExit);
        }
    else if (req == 5)
        {
        // (v0.4.82) the benchmark's log: into a file, then to the PC
        iGame->iRequest = 0;
        TPtrC8 log((const TUint8*)iGame->iBenchLog, iGame->iBenchLen);
        fs.MkDirAll(KBenchDir);
        RFile f;
        if (f.Replace(fs, KBenchFile, EFileWrite) == KErrNone) { f.Write(log); f.Close(); }
        if (!iBenchSender) iBenchSender = new CBenchSender(iGame);
        if (iBenchSender) iBenchSender->Send(log);
        }
    else if (req == 4)
        {
        iGame->iRequest = 0;
        if (!iUpdater) iUpdater = new CUpdater(iGame, this);
        if (!iUpdater) return;
        // the game stopped (its loop, its screen): the phone's time all the download's
        SetActive(EFalse);
        iUpdating = ETrue;
        TInt k;
        for (k = 0; k < 4; k++) iUpdText[k].Zero();
        DrawNow();
        // (onto the drive the game is on: the card's room, not the phone's)
        TFileName app = iEikonEnv->EikAppUi()->Application()->AppFullName();
        iUpdater->Start(app.Length() ? (TChar)app[0] : (TChar)'C');
        }
    }

void CGameView::Frame()
    {
    TTime t0;
    t0.UniversalTime();
    TInt dt = t0.MicroSecondsFrom(iLast).Int64().Low();
    iLast = t0;
    iGame->ReportTimes(iLastRender, iLastBlit, dt);
    if (iRamTick-- <= 0)
        {
        iRamTick = 30;
        TInt f = 0, t = 0;
        HAL::Get(HALData::EMemoryRAMFree, f);
        HAL::Get(HALData::EMemoryRAM, t);
        iGame->iRamFreeKB = f / 1024; iGame->iRamTotalKB = t / 1024; iGame->iRamUsedKB = iChunkSize / 1024;
        }
    iGame->Tick(dt, iKeys);
    HandleRequests();
    R3D* r = iGame->iR;
    TBool direct = iGame->WantDirect() && iFb && StartDirect();
    if (!direct) StopDirect();
    if (direct)
        {
        // render off-screen, then copy the finished frame to the screen in one burst
        iGame->Render(iBack, OUT_W);
        TTime t1;
        t1.UniversalTime();
        TInt x0 = r->OutX0(), w = r->OutW();
        if (r->iBorderFrames > 0 || iBorderCopy > 0) { x0 = 0; w = OUT_W; }
        iBorderCopy = r->iBorderFrames;
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
        return;
        }
    iBmp->LockHeap();
    TUint16* p = (TUint16*)iBmp->DataAddress();
    TInt stride = CFbsBitmap::ScanLineLength(OUT_W, i4K ? EColor4K : EColor64K) / 2;
    iGame->Render(p, stride);
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
    if (r->iNative && !iFullBlit)
        {
        TRect rc(r->OutX0(), 0, r->OutX0() + r->OutW(), OUT_H);
        Window().Invalidate(rc);
        ActivateGc();
        Window().BeginRedraw(rc);
        SystemGc().BitBlt(rc.iTl, iBmp, rc);
        Window().EndRedraw();
        DeactivateGc();
        }
    else DrawNow();
    iFullBlit = r->iBorderFrames > 0;
    iCoeEnv->WsSession().Flush();
    TTime t2;
    t2.UniversalTime();
    iLastRender = t1.MicroSecondsFrom(t0).Int64().Low();
    iLastBlit = t2.MicroSecondsFrom(t1).Int64().Low();
    User::ResetInactivityTime();
    }

void CGameView::UpdLine(const TDesC8& aText)
    {
    // the newest line at the bottom, the older ones above (a progress line replaces the one before it)
    TBuf<96> t; t.Copy(aText.Left(96));
    TInt k;
    TBool progress = t.Find(_L(" of ")) >= 0 || (t.Length() > 8 && t.Right(3) == _L("KB "));
    if (progress && iUpdText[3].Find(_L(" of ")) >= 0) iUpdText[3] = t;
    else { for (k = 0; k < 3; k++) iUpdText[k] = iUpdText[k + 1]; iUpdText[3] = t; }
    if (iUpdating) DrawNow();
    }
static TInt ExitCb(TAny* aView)
    {
    (void)aView;
    CEikonEnv::Static()->EikAppUi()->HandleCommandL(EEikCmdExit);
    return 0;
    }
void CGameView::UpdDone()
    {
    // the installer has the new package: the game ends so it can be replaced (from the idle loop, not from inside
    // the download's own completion - its errors are caught there, and the exit would be with them)
    iIdle->Cancel();
    iIdle->Start(TCallBack(ExitCb, this));
    }
void CGameView::Draw(const TRect& /*aRect*/) const
    {
    CWindowGc& gc = SystemGc();
    if (iUpdating)
        {
        gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
        gc.SetBrushColor(KRgbBlack);
        gc.Clear(Rect());
        gc.UseFont(iEikonEnv->NormalFont());
        gc.SetPenColor(TRgb(230, 212, 112));
        _LIT(KTitle, "HRIMFELL - updating from http://192.168.137.1:8000/skyrim.sis");
        gc.DrawText(KTitle, TPoint(40, 40));
        gc.SetPenColor(TRgb(220, 220, 220));
        TInt k;
        for (k = 0; k < 4; k++) gc.DrawText(iUpdText[k], TPoint(40, 70 + k * 20));
        gc.SetPenColor(TRgb(150, 150, 150));
        _LIT(KEsc, "Esc: cancel the update and go back to the game");
        gc.DrawText(KEsc, TPoint(40, 170));
        gc.DiscardFont();
        return;
        }
    if (!iReady)
        {
        gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
        gc.SetBrushColor(KRgbBlack);
        gc.Clear(Rect());
        gc.UseFont(iEikonEnv->NormalFont());
        gc.SetPenColor(TRgb(230, 212, 112));
        _LIT(KLoading, "HRIMFELL - forging the world...");
        gc.DrawText(KLoading, TPoint(230, 90));
        // which stage is being built, and the memory situation (shows where it stops if anything fails)
        TInt step = iInitPhase / 2;
        TBuf<96> line;
        _LIT(KStep, "Stage %d/%d: ");
        line.Format(KStep, step + 1, (TInt)Game::INIT_STEPS);
        if (iGame && step < Game::INIT_STEPS)
            {
            const char* nm = iGame->InitStepName(step);
            TPtrC8 n8((const TUint8*)nm);
            TBuf<64> n16;
            n16.Copy(n8.Left(60));
            line.Append(n16);
            }
        gc.SetPenColor(TRgb(200, 200, 200));
        gc.DrawText(line, TPoint(230, 112));
        TInt ramFree = 0, ramTotal = 0;
        HAL::Get(HALData::EMemoryRAMFree, ramFree);
        HAL::Get(HALData::EMemoryRAM, ramTotal);
        _LIT(KRam, "RAM free %d KB of %d KB, game uses %d KB");
        line.Format(KRam, ramFree / 1024, ramTotal / 1024, iChunkSize / 1024);
        gc.DrawText(line, TPoint(230, 130));
        gc.DiscardFont();
        return;
        }
    gc.BitBlt(TPoint(0, 0), iBmp);
    }

TKeyResponse CGameView::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
    {
    if (!iReady) return EKeyWasConsumed;
    if (iUpdating)
        {
        // (v0.4.64) only Esc: the download given up, its file deleted, the game again
        if (aType == EEventKey && aKeyEvent.iCode == EKeyEscape)
            {
            if (iUpdater) iUpdater->Abort();
            iUpdating = EFalse;
            DrawNow();
            SetActive(ETrue);
            }
        return EKeyWasConsumed;
        }
    if (aType == EEventKeyDown) { iKeys |= KeyBit(aKeyEvent.iScanCode); iGame->iHitIn |= KeyBit(aKeyEvent.iScanCode); }
    else if (aType == EEventKeyUp) iKeys &= ~KeyBit(aKeyEvent.iScanCode);
    else if (aType == EEventKey)
        {
        TInt c = aKeyEvent.iCode;
        if (c == EKeyEscape) c = CH_ESC;
        else if (c == EKeyTab) c = CH_TAB;
        else if (c == EKeyEnter) c = CH_ENTER;
        if (c < 128) iGame->KeyChar(c);
        }
    return EKeyWasConsumed;
    }

// ---------------------------------------------------------------- app framework
class CGameAppUi : public CEikAppUi
    {
public:
    void ConstructL();
    ~CGameAppUi();
    void HandleCommandL(TInt aCommand);
    void HandleForegroundEventL(TBool aForeground);
private:
    CGameView* iView;
    };

void CGameAppUi::ConstructL()
    {
    BaseConstructL();
    CknEnv::Skin().SetAppViewType(ESkinAppViewNoCbaNoToolband);
    iView = new (ELeave) CGameView;
    iView->ConstructL(ApplicationRect());
    AddToStackL(iView);
    }

CGameAppUi::~CGameAppUi()
    {
    if (iView) { RemoveFromStack(iView); delete iView; }
    }

void CGameAppUi::HandleCommandL(TInt aCommand)
    {
    if (aCommand == EEikCmdExit) Exit();
    }

void CGameAppUi::HandleForegroundEventL(TBool aForeground)
    {
    CEikAppUi::HandleForegroundEventL(aForeground);
    if (iView) iView->SetActive(aForeground);
    }

class CGameDocument : public CEikDocument
    {
public:
    CGameDocument(CEikApplication& aApp) : CEikDocument(aApp) {}
    CEikAppUi* CreateAppUiL() { return new (ELeave) CGameAppUi; }
    };

class CGameApplication : public CEikApplication
    {
    CApaDocument* CreateDocumentL() { return new (ELeave) CGameDocument(*this); }
    TUid AppDllUid() const { return KUidHrimfell; }
    };

EXPORT_C CApaApplication* NewApplication() { return new CGameApplication; }
GLDEF_C TInt E32Dll(TDllReason) { return KErrNone; }
