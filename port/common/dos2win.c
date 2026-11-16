/*
  DOS2WIN.C - бэкенд Windows для порта VRS Lila (см dos2win.h).
  Видео: окно Win32 + StretchDIBits 320x200 -> масштабирование с
  сохранением пропорций 8:5 (по умолчанию 640x400).
  Ввод: WM_KEYDOWN/UP -> scan-коды set-1 (+0x80 расширенные) - те же
  scod, что генерировал ISR int 9 в DOS; мышь - Raw Input (дельты) +
  счетчики нажатий (семантика int 33h).
  Звук: PC-спикер (PIT канал 2, режим 3 - меандр) эмулируется waveOut.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>//трекбары (слайдеры) окна Settings
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dos2win.h"

typedef signed long fix;//фиксация запятой 16.16 (как в vrsNN.h)
typedef unsigned long dword;//32-бит (как в vrsNN.h)

#define WINKEY_QUEUE 64
#define PITBASE 1193180.0//частота PIT (в оригинале писали 1191830 - опечатка,
                         //для высоты звука берем верную)

/*------------------------------------------------------------
  состояние
*/
static HWND wnd;
static int quitflag=0;
static int scrnmode=3;//текущий 'видеорежим': 3=текст (как в DOS), 0x13=графика
static unsigned char vram[WINVRAMLEN];
static unsigned char syspalsave[768];//последняя установленная палитра (0..63)
static unsigned char lut[256*4];//LUT для DIB: B,G,R,0
static DWORD starttick;
static unsigned long timems_prev;

//клавиатура
static struct { int scod,hit; } keyq[WINKEY_QUEUE];
static volatile int keyqwr=0,keyqrd=0;
static int anycod=0;
static int keydown[256];//текущее состояние нажатия по scod (для сброса при потере фокуса)
static int pausesent=0;

//мышь
static int mousepresent=0;
static volatile int mousedx=0,mousedy=0;
static volatile int mousebtn=0;
static volatile int mousepress[2]={0,0},mouserelease[2]={0,0};

//окно показа: прямоугольник вывода 320x200 внутри клиента (пропорции 8:5)
static int dispw=640,disph=400;

//звук
static volatile int sndfreq=0;//текущая частота Гц (0=тишина)
static volatile int sndgate=0;//гейт (порт 61h)
static volatile int sndvol=20;//громкость 0..100 (по умолчанию 20%)
static HANDLE sndthread;
static volatile int sndrun=0;
static void (*sndtickcb)(void);//тик секвенсора игры (каждые 4 мс)

//ограничение частоты кадров
static volatile int targetfps=50;//цель 10..90 (0=без ограничения)
static volatile int realfps=0;   //измеренная частота витков

//окно Help (не-модальный попап с текстом help.doc)
#define NO  0
#define YES 1
static HWND helpwnd=NULL;
static HWND setwnd=NULL;    //окно Settings
static int comctlinit=0;    //инициализация общих контролов (трекбары)
static HWND helpedit=NULL;
static const wchar_t helpclass[]=L"LilaHelpWnd";

/*------------------------------------------------------------
  окно
*/
static void ComputeDisplayRect(int cw,int ch)
{
double s;
if (cw*5>ch*8) s=(double)ch/200.0; else s=(double)cw/320.0;
if (s<0.25) s=0.25;
dispw=(int)(320.0*s); disph=(int)(200.0*s);
if (dispw<1) dispw=1; if (disph<1) disph=1;
}

static unsigned char frame32[320*200*4];//конвертированный кадр BGRx
static HDC memdc=NULL;      //второй, невидимый буфер окна (двойная буферизация)
static HBITMAP membmp=NULL; //битмап невидимого буфера
static int memw=0,memh=0;   //текущий размер невидимого буфера

static void PaintWindow(HDC hdc)
{
BITMAPINFO bmi;
RECT rc;
int cw,ch,i;
const unsigned char *c;

GetClientRect(wnd,&rc);
cw=rc.right; ch=rc.bottom;
if (cw<1) cw=1;
if (ch<1) ch=1;

//пересоздание невидимого буфера при изменении размера окна
if (memdc==NULL || memw!=cw || memh!=ch)
  {
  HDC wdc=GetDC(wnd);
  if (membmp) { DeleteObject(membmp); membmp=NULL; }
  if (memdc)  { DeleteDC(memdc);  memdc=NULL; }
  memdc=CreateCompatibleDC(wdc);
  membmp=CreateCompatibleBitmap(wdc,cw,ch);
  SelectObject(memdc,membmp);
  ReleaseDC(wnd,wdc);
  memw=cw; memh=ch;
  PatBlt(memdc,0,0,cw,ch,BLACKNESS);
  }

//полная сборка кадра в невидимом буфере: видимый экран в это время
//продолжает показывать предыдущий кадр
//заливка черным вокруг
PatBlt(memdc,0,0,cw,ch,BLACKNESS);
//конвертация индексированного кадра в 32bpp через LUT палитры
for (i=0; i<WINVRAMLEN; ++i)
  {
  c=lut+vram[i]*4;
  frame32[i*4]  =c[0];
  frame32[i*4+1]=c[1];
  frame32[i*4+2]=c[2];
  frame32[i*4+3]=0;
  }
//центрированный вывод с сохранением пропорций 8:5
memset(&bmi,0,sizeof(bmi));
bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
bmi.bmiHeader.biWidth=320;
bmi.bmiHeader.biHeight=-200;//top-down
bmi.bmiHeader.biPlanes=1;
bmi.bmiHeader.biBitCount=32;
bmi.bmiHeader.biCompression=BI_RGB;
SetStretchBltMode(memdc,COLORONCOLOR);
StretchDIBits(memdc,(cw-dispw)/2,(ch-disph)/2,dispw,disph,
              0,0,320,200,frame32,&bmi,DIB_RGB_COLORS,SRCCOPY);

//мгновенный вывод готового кадра на видимый экран одной операцией
BitBlt(hdc,0,0,cw,ch,memdc,0,0,SRCCOPY);
}

static void PushKey(int scod,int hit)
{
int nw;
nw=(keyqwr+1)%WINKEY_QUEUE;
if (nw==keyqrd) return;//переполнение - теряем (как в DOS)
keyq[keyqwr].scod=scod;
keyq[keyqwr].hit=hit;
keyqwr=nw;
anycod=1;
}

static void ReleaseAllKeys(void)
{
int s;
for (s=0; s<256; ++s)
  if (keydown[s])
    {
    keydown[s]=0;
    PushKey(s,WINKEY_OFF);
    }
pausesent=0;
}

/*------------------------------------------------------------
  меню окна и диалоги About / Help
*/
#define IDM_ABOUT    1001
#define IDM_HELP     1002
#define IDM_SETTINGS 1003

static char about_orig[512]="";//информация об оригинале (ASCII)

static LRESULT CALLBACK HelpWndProc(HWND hw,UINT m,WPARAM wp,LPARAM lp)
{
switch (m)
  {
  case WM_DESTROY:
    helpwnd=NULL;
    helpedit=NULL;
    return 0;
  case WM_SIZE:
    if (helpedit) MoveWindow(helpedit,0,0,LOWORD(lp),HIWORD(lp),TRUE);
    return 0;
  }
return DefWindowProcW(hw,m,wp,lp);
}

void WinSetAbout(const char *originfo)
{
strncpy(about_orig,originfo,511);
about_orig[511]=0;
}

static void ShowAbout(void)
{
wchar_t wtext[2048],wtitle[128];
char text[1024];
int n;
snprintf(text,sizeof(text),
         "%s\n\nПорт на Windows: версия %s (%s)\nПорт сделал: Вадим Мадгазин",
         about_orig,WINPORT_VER,WINPORT_YEAR);
n=MultiByteToWideChar(CP_UTF8,0,text,-1,wtext,2047);
wtext[n]=0;
n=MultiByteToWideChar(CP_UTF8,0,"О программе",-1,wtitle,127);
wtitle[n]=0;
MessageBoxW(wnd,wtext,wtitle,MB_OK|MB_ICONINFORMATION);
}

static void ShowHelp(void)
{
static const char *fallback=
  "Управление (файл help.doc не найден - встроенная сводка):\r\n\r\n"
  "F10 - выход из программы\r\n"
  "Стрелки, Add4/6 - движение, Left/Right - повороты\r\n"
  "Add8/2 - движение вверх-вниз\r\n"
  "Tab - проницаемость панелей (клиппинг)\r\n"
  "P - панель параметров, S - звук вкл/выкл\r\n"
  "F1 - хелп в 3D-пространстве\r\n"
  "1...9 - варианты фона/неба/дали\r\n"
  "< и > - размер экрана рендера (80x50 ... 320x200)\r\n"
  "58: Space - бросок ножа, 0 - космос/земля\r\n"
  "57: Enter - выбор панели, Ins/Del - видимость панелей ПП1";
FILE *f;
int n,size=0,mem=65536,isfile=NO,isutf8=NO;
char *buf;
wchar_t *wbuf=NULL;
long rd;
RECT rc;
  if (helpwnd)
    {
    SetForegroundWindow(helpwnd);
    return;
    }
  //текст хелпа для окна Windows: help.txt (UTF-8 с BOM) - редактируемая
  //копия help.doc; если его нет - help.doc (CP866, как в 3D по F1)
  buf=(char *)malloc(mem);
  if (buf) memset(buf,0,mem);
  f=fopen("help.txt","rb");
  if (f)
    {
    rd=(long)fread(buf,1,mem-1,f);
    fclose(f);
    //проверяем BOM UTF-8
    if (rd>3 && (unsigned char)buf[0]==0xEF &&
        (unsigned char)buf[1]==0xBB && (unsigned char)buf[2]==0xBF)
      {
      memmove(buf,buf+3,rd-3);
      buf[rd-3]=0;
      size=(int)rd-3;
      isfile=YES;
      isutf8=YES;
      }
    }
  if (!isfile)
    {
    f=fopen("help.doc","rb");
    if (f)
      {
      rd=(long)fread(buf,1,mem-1,f);
      fclose(f);
      if (rd>0) { size=(int)rd; isfile=YES; }
      }
    }
  if (!isfile)
    {
    free(buf);
    buf=(char *)fallback;
    size=strlen(fallback);
    }
  else
  if (!isutf8)
    {
    //страницы help.doc разделены FF (12) - заменяем на '.', как в DocLoad()
    for (n=0; n<size; ++n) if (buf[n]==12) buf[n]='.';
    }
  //UTF-8 (help.txt/встроенный текст) или CP866 (help.doc) -> UTF-16
  wbuf=(wchar_t *)malloc((size*4+64)*sizeof(wchar_t));
  if (wbuf==NULL)
    {
    if (isfile) free(buf);
    return;
    }
  n=MultiByteToWideChar(isutf8? CP_UTF8:866,0,buf,size,wbuf,size*4+60);
  if (n<0) n=0;
  wbuf[n]=0;
  if (isfile) free(buf);

//окно хелпа: не-модальный попап с дочерним EDIT и прокруткой
helpwnd=CreateWindowExW(0,helpclass,NULL,
  WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_VISIBLE,
  CW_USEDEFAULT,CW_USEDEFAULT,680,520,wnd,NULL,GetModuleHandle(NULL),NULL);
if (helpwnd)
  {
  wchar_t wtitle[128];
  GetClientRect(helpwnd,&rc);
  helpedit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",NULL,
    WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,
    0,0,rc.right,rc.bottom,helpwnd,NULL,GetModuleHandle(NULL),NULL);
  SetWindowTextW(helpedit,wbuf);
  SendMessageW(helpedit,WM_SETFONT,
    (WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
  n=MultiByteToWideChar(CP_UTF8,0,"Лила - Help (help.doc)",-1,wtitle,127);
  wtitle[n]=0;
  SetWindowTextW(helpwnd,wtitle);
  }
free(wbuf);
}

/*------------------------------------------------------------
  окно Settings: громкость звука + скорость рендера + реальный FPS
*/
#define IDC_VOL     2001
#define IDC_FPS     2002
#define IDC_VOLTXT  2003
#define IDC_FPSTXT  2004
#define IDC_REALTXT 2005
static const wchar_t setclass[]=L"LilaSetWnd";
static HWND voltb=NULL,fpsb=NULL,voltxt=NULL,fpstxt=NULL,realtxt=NULL;

static void SetWideText(HWND h,const char *utf8)
{
wchar_t w[160];
int n=MultiByteToWideChar(CP_UTF8,0,utf8,-1,w,159);
if (n<0) n=0;
w[n]=0;
SetWindowTextW(h,w);
}

static void UpdateSettingsLabels(void)
{
char t[96];
sprintf(t,"Громкость звука: %i%%",WinGetVolume());
SetWideText(voltxt,t);
sprintf(t,"Скорость рендера: %i FPS",WinGetTargetFPS());
SetWideText(fpstxt,t);
sprintf(t,"Реальный FPS: %i  (цель 10..90; если скорость не хватает - реальная ниже)",WinGetFPS());
SetWideText(realtxt,t);
}

static LRESULT CALLBACK SetWndProc(HWND hw,UINT m,WPARAM wp,LPARAM lp)
{
switch (m)
  {
  case WM_HSCROLL://движение слайдеров
    {
    HWND h=(HWND)lp;
    int pos=(int)SendMessageW(h,TBM_GETPOS,0,0);
    int id=GetDlgCtrlID(h);
    if (id==IDC_VOL) WinSetVolume(pos);
    if (id==IDC_FPS) WinSetTargetFPS(pos);
    UpdateSettingsLabels();
    }
    return 0;
  case WM_TIMER://обновление цифры реального FPS
    UpdateSettingsLabels();
    return 0;
  case WM_DESTROY:
    KillTimer(hw,1);
    setwnd=NULL;
    voltb=NULL; fpsb=NULL;
    voltxt=NULL; fpstxt=NULL; realtxt=NULL;
    return 0;
  }
return DefWindowProcW(hw,m,wp,lp);
}

static HWND MakeTrackbar(HWND parent,int id,int x,int y,int w,int h,
                         int vmin,int vmax,int vpos)
{
HWND tb=CreateWindowExW(0,L"msctls_trackbar32",NULL,
  WS_CHILD|WS_VISIBLE|TBS_HORZ|TBS_AUTOTICKS,
  x,y,w,h,parent,(HMENU)(INT_PTR)id,GetModuleHandle(NULL),NULL);
SendMessageW(tb,TBM_SETRANGE,TRUE,MAKELONG(vmin,vmax));
SendMessageW(tb,TBM_SETPOS,TRUE,(LPARAM)vpos);
return tb;
}

static HWND MakeLabel(HWND parent,const char *text,int id,int x,int y,int w,int h)
{
HWND st=CreateWindowExW(0,L"STATIC",NULL,WS_CHILD|WS_VISIBLE,x,y,w,h,
  parent,(HMENU)(INT_PTR)id,GetModuleHandle(NULL),NULL);
SetWideText(st,text);
SendMessageW(st,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
return st;
}

static void ShowSettings(void)
{
wchar_t wtitle[128];
int n;
if (setwnd)
  {
  SetForegroundWindow(setwnd);
  UpdateSettingsLabels();
  return;
  }
if (!comctlinit)
  {
  INITCOMMONCONTROLSEX icc;
  icc.dwSize=sizeof(icc);
  icc.dwICC=ICC_BAR_CLASSES;
  InitCommonControlsEx(&icc);
  comctlinit=1;
  }
setwnd=CreateWindowExW(0,setclass,NULL,
  WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_VISIBLE,
  CW_USEDEFAULT,CW_USEDEFAULT,490,190,wnd,NULL,GetModuleHandle(NULL),NULL);
if (!setwnd) return;
voltxt=MakeLabel(setwnd,"",IDC_VOLTXT,15,18,230,22);
voltb =MakeTrackbar(setwnd,IDC_VOL,250,15,220,32,0,100,WinGetVolume());
fpstxt=MakeLabel(setwnd,"",IDC_FPSTXT,15,73,230,22);
fpsb  =MakeTrackbar(setwnd,IDC_FPS,250,70,220,32,10,90,WinGetTargetFPS());
realtxt=MakeLabel(setwnd,"",IDC_REALTXT,15,125,460,22);
UpdateSettingsLabels();
SetTimer(setwnd,1,500,NULL);
n=MultiByteToWideChar(CP_UTF8,0,"Лила - Settings",-1,wtitle,127);
wtitle[n]=0;
SetWindowTextW(setwnd,wtitle);
}

static LRESULT CALLBACK WndProc(HWND hw,UINT m,WPARAM wp,LPARAM lp)
{
int scan,scod,down,rep;
switch (m)
  {
  case WM_CLOSE:
    quitflag=1;
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_SIZE:
    ComputeDisplayRect(LOWORD(lp),HIWORD(lp));
    InvalidateRect(hw,NULL,FALSE);
    return 0;
  case WM_PAINT:
    {
    PAINTSTRUCT ps;
    BeginPaint(hw,&ps);
    PaintWindow(ps.hdc);
    EndPaint(hw,&ps);
    }
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_COMMAND://выбор пунктов меню Menu -> About / Help / Settings
    if (LOWORD(wp)==IDM_ABOUT)    ShowAbout();
    if (LOWORD(wp)==IDM_HELP)     ShowHelp();
    if (LOWORD(wp)==IDM_SETTINGS) ShowSettings();
    return 0;
  case WM_SETCURSOR:
    if (LOWORD(lp)==HTCLIENT) { SetCursor(NULL); return TRUE; }
    break;
  //клавиатура: WM_SYS* обрабатываем наравне (клавиши с Alt),
  //DefWindowProc для них не зовем - чтобы не вылезало меню и Alt+F4
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    down=1; goto keycode;
  case WM_KEYUP:
  case WM_SYSKEYUP:
    down=0;
  keycode:
    scan=((int)lp>>16)&0xFF;//scan-код set-1 - тот же, что в порту 60h DOS!
    if (wp==VK_PAUSE)
      {
      //в DOS ISR клавиша Pause генерила спецкод scod=0 с hit=KEYOFF
      //один раз на физическое нажатие
      if (down && !pausesent) { pausesent=1; PushKey(0,WINKEY_OFF); }
      if (!down) pausesent=0;
      return 0;
      }
    if (((int)lp)&(1<<24)) scan|=0x80;//расширенная (E0-префикс в DOS)
    rep=(down && (((int)lp)&(1<<30)));//автоповтор - как typematic DOS
    scod=scan&0xFF;
    if (down)
      {
      keydown[scod]=1;
      PushKey(scod,WINKEY_ON);
      }
    else
      {
      keydown[scod]=0;
      PushKey(scod,WINKEY_OFF);
      }
    (void)rep;
    return 0;
  case WM_SYSCHAR://гасим системный символ (Alt+клавиша)
    return 0;
  case WM_KILLFOCUS://все зажатые клавиши отжать - иначе 'залипание'
    ReleaseAllKeys();
    mousebtn=0;
    break;
  //мышь: кнопки (счетчики как fn 05/06 int 33h)
  case WM_LBUTTONDOWN: mousebtn|=1; mousepress[0]++; return 0;
  case WM_LBUTTONUP:   mousebtn&=~1; mouserelease[0]++; return 0;
  case WM_RBUTTONDOWN: mousebtn|=2; mousepress[1]++; return 0;
  case WM_RBUTTONUP:   mousebtn&=~2; mouserelease[1]++; return 0;
  }
return DefWindowProc(hw,m,wp,lp);
}

/*------------------------------------------------------------
  raw input: относительные дельты мыши (микки - как int 33h fn 0x0B)
*/
static void RegisterRawMouse(void)
{
RAWINPUTDEVICE rid;
rid.usUsagePage=1;
rid.usUsage=2;//мышь
rid.dwFlags=0;//только при фокусе окна
rid.hwndTarget=wnd;
if (!RegisterRawInputDevices(&rid,1,sizeof(rid))) return;
}

/*------------------------------------------------------------
  поток звука: 4 буфера по 100 мс, квадрат-меандр
*/
#define SNDRATE 32000
#define TICKSAMPLES (SNDRATE/250)   /*128 сэмплов = ровно 4 мс - тик секвенсора*/
#define SNDBUF  (TICKSAMPLES*25)    /*3200 сэмплов = 100 мс, кратен тику!*/
static HWAVEOUT hwo;
static WAVEHDR whdr[4];
static char snddata[4][SNDBUF];
static double sndphase=0.0;         /*фаза меандра (непрерывна между тиками)*/
static int sndcurfreq=0;            /*частота, на которой сейчас генерим*/

static void FillSndBuffer(char *buf)
{
int i,s,f,amp;
amp=(127*sndvol)/100;/*громкость 0..100: 50% -> 63, 100% -> 127 (как у спикера)*/
for (i=0; i<SNDBUF; ++i)
  {
  //тик секвенсора игры каждые 4 мс - независимо от частоты кадров!
  if ((i%TICKSAMPLES)==0 && sndtickcb) sndtickcb();
  f=sndgate? sndfreq:0;
  //смена делителя = перезагрузка PIT: фаза меандра сбрасывается в 0
  if (f!=sndcurfreq) { sndcurfreq=f; sndphase=0; }
  if (f<=0 || f>(SNDRATE/2)) { buf[i]=(char)128; sndphase=0; continue; }
  sndphase+=(double)f/SNDRATE;
  if (sndphase>=1.0) sndphase-=1.0;
  s=(sndphase<0.5)? amp: -amp;
  buf[i]=(char)(128+s);
  }
}

static DWORD WINAPI SndThread(LPVOID par)
{
int i;
(void)par;
for (i=0; i<4; ++i)
  {
  whdr[i].lpData=snddata[i];
  whdr[i].dwBufferLength=SNDBUF;
  whdr[i].dwFlags=0;
  whdr[i].dwUser=i;
  FillSndBuffer(snddata[i]);
  waveOutPrepareHeader(hwo,&whdr[i],sizeof(WAVEHDR));
  }
for (i=0; i<4; ++i) waveOutWrite(hwo,&whdr[i],sizeof(WAVEHDR));
while (sndrun)
  {
  int found=-1;
  //ожидание завершения любого буфера по флагу WHDR_DONE
  while (sndrun)
    {
    for (i=0; i<4; ++i)
      if (whdr[i].dwFlags & WHDR_DONE) { found=i; break; }
    if (found>=0) break;
    Sleep(2);//процессор отдыхает
    }
  if (found<0) break;
  i=found;
  FillSndBuffer(snddata[i]);
  whdr[i].dwFlags&=~WHDR_DONE;
  waveOutWrite(hwo,&whdr[i],sizeof(WAVEHDR));
  }
return 0;
}

void SoundHWSetup(void)
{
WAVEFORMATEX wf;
int i;
if (waveOutGetNumDevs()<=0) return;
wf.wFormatTag=WAVE_FORMAT_PCM;
wf.nChannels=1;
wf.nSamplesPerSec=SNDRATE;
wf.wBitsPerSample=8;
wf.nBlockAlign=1;
wf.nAvgBytesPerSec=SNDRATE;
if (waveOutOpen(&hwo,WAVE_MAPPER,&wf,(DWORD_PTR)wnd,0,CALLBACK_NULL)!=MMSYSERR_NOERROR)
  return;
sndrun=1;
sndthread=CreateThread(NULL,0,SndThread,NULL,0,NULL);
(void)i;
}

void SoundToneHW(int pitdiv)
{
static int logflag=-1;
if (pitdiv<=2) { sndfreq=0; return; }//стоп-делитель 2 (~600кГц неслышно)
sndfreq=(int)(PITBASE/pitdiv);
if (sndfreq>8000) sndfreq=0;//за пределами слышимости - тишина
if (logflag==-1) logflag=(getenv("LILA_LOG")!=NULL);
if (logflag) printf("TONE div=%i freq=%i Hz\n",pitdiv,sndfreq);
}

void SoundGateHW(int on) { sndgate=on; }

void SoundHWDone(void)
{
if (!sndrun) return;
sndrun=0;
WaitForSingleObject(sndthread,300);
waveOutReset(hwo);
for (int i=0; i<4; ++i) waveOutUnprepareHeader(hwo,&whdr[i],sizeof(WAVEHDR));
waveOutClose(hwo);
sndgate=0; sndfreq=0;
}

/*------------------------------------------------------------
  API
*/
static LONG WINAPI CrashHandler(EXCEPTION_POINTERS *ep)
{
FILE *f=fopen("lila_crash.log","w");
if (f)
  {
  fprintf(f,"EXCEPTION code=0x%08lX addr=%p\n",
          (unsigned long)ep->ExceptionRecord->ExceptionCode,
          ep->ExceptionRecord->ExceptionAddress);
  fprintf(f,"EIP=%08lX EAX=%08lX EBX=%08lX ECX=%08lX EDX=%08lX ESI=%08lX EDI=%08lX\n",
          (unsigned long)ep->ContextRecord->Eip,
          (unsigned long)ep->ContextRecord->Eax,
          (unsigned long)ep->ContextRecord->Ebx,
          (unsigned long)ep->ContextRecord->Ecx,
          (unsigned long)ep->ContextRecord->Edx,
          (unsigned long)ep->ContextRecord->Esi,
          (unsigned long)ep->ContextRecord->Edi);
  fprintf(f,"EBP=%08lX ESP=%08lX\n",
          (unsigned long)ep->ContextRecord->Ebp,
          (unsigned long)ep->ContextRecord->Esp);
  fclose(f);
  }
MessageBoxA(NULL,"Crash! See lila_crash.log","Lila",MB_OK|MB_ICONERROR);
return EXCEPTION_EXECUTE_HANDLER;
}

void WinInit(int argc,char **argv,const char *title)
{
WNDCLASS wc;
WNDCLASSW wch;
RECT rc;
int cw=640,ch=400;
HINSTANCE hinst=GetModuleHandle(NULL);
HMENU mbar,msub;
(void)argc; (void)argv;

SetUnhandledExceptionFilter(CrashHandler);
timeBeginPeriod(1);
starttick=timeGetTime();

//консольное окно НЕ открываем - только окно графики;
//при LILA_LOG - printf в файл lila.log (для отладки),
//ошибки старта показываются через WinErrorBox
if (getenv("LILA_LOG"))
  {
  FILE *f;
  freopen("lila.log","w",stdout);
  setvbuf(stdout,NULL,_IONBF,0);
  f=stdout;
  (void)f;
  }

memset(&wc,0,sizeof(wc));
wc.style=CS_OWNDC;
wc.lpfnWndProc=WndProc;
wc.hInstance=hinst;
wc.hCursor=LoadCursor(NULL,IDC_ARROW);
wc.lpszClassName="LilaVRS";
RegisterClass(&wc);

//класс окна Help
memset(&wch,0,sizeof(wch));
wch.lpfnWndProc=HelpWndProc;
wch.hInstance=hinst;
wch.hCursor=LoadCursor(NULL,IDC_ARROW);
wch.lpszClassName=helpclass;
RegisterClassW(&wch);
//класс окна Settings
memset(&wch,0,sizeof(wch));
wch.lpfnWndProc=SetWndProc;
wch.hInstance=hinst;
wch.hCursor=LoadCursor(NULL,IDC_ARROW);
wch.lpszClassName=setclass;
RegisterClassW(&wch);

//меню окна: Menu -> About / Help / Settings (без мнемоник - не мешать Alt-клавишам игры)
msub=CreatePopupMenu();
AppendMenuA(msub,MF_STRING,IDM_ABOUT,"About");
AppendMenuA(msub,MF_STRING,IDM_HELP,"Help");
AppendMenuA(msub,MF_SEPARATOR,0,NULL);
AppendMenuA(msub,MF_STRING,IDM_SETTINGS,"Settings...");
mbar=CreateMenu();
AppendMenuA(mbar,MF_POPUP,(UINT_PTR)msub,"Menu");

//клиент 640x400
rc.left=0; rc.top=0; rc.right=cw; rc.bottom=ch;
AdjustWindowRect(&rc,WS_OVERLAPPEDWINDOW|WS_VISIBLE,FALSE);
wnd=CreateWindow("LilaVRS",title,
  WS_OVERLAPPEDWINDOW|WS_VISIBLE,
  CW_USEDEFAULT,CW_USEDEFAULT,
  rc.right-rc.left,rc.bottom-rc.top,
  NULL,mbar,hinst,NULL);
//меню съедает часть клиента - увеличиваем окно наружу на эту высоту
GetClientRect(wnd,&rc);
if ((rc.bottom-rc.top)!=ch)
  {
  RECT rw;
  GetWindowRect(wnd,&rw);
  SetWindowPos(wnd,NULL,0,0,rw.right-rw.left,(rw.bottom-rw.top)+(ch-(rc.bottom-rc.top)),
               SWP_NOMOVE|SWP_NOZORDER);
  GetClientRect(wnd,&rc);
  ComputeDisplayRect(rc.right,rc.bottom);
  }
else
  ComputeDisplayRect(cw,ch);
RegisterRawMouse();
mousepresent=1;//мышь в Windows всегда есть
SoundHWSetup();
}

void WinDone(void)
{
SoundHWDone();
timeEndPeriod(1);
}

int WinQuitRequested(void) { return quitflag; }

void WinPump(void)
{
MSG m;
//NULL - качаем сообщения всех окон (в т.ч. окна Help)
while (PeekMessage(&m,NULL,0,0,PM_REMOVE))
  {
  if (m.message==WM_QUIT) { quitflag=1; return; }
  TranslateMessage(&m);
  DispatchMessage(&m);
  }
}

void WinFrameEnd(void)
/*легкое ограничение частоты витков: цель 10..90 FPS (0=без ограничения);
  в промежутках ожидания - Sleep, чтобы процессор отдыхал*/
{
static dword nextdeadline=0;
static dword framecnt=0,fpsmark=0;
dword t=WinTimeMs();
//измерение реальной частоты витков
++framecnt;
if (t-fpsmark>=1000)
  {
  realfps=(int)((framecnt*1000)/(t-fpsmark));
  framecnt=0;
  fpsmark=t;
  }
//выдерживание периода кадра
if (targetfps>0)
  {
  dword period=1000/(dword)targetfps;
  if (nextdeadline==0) nextdeadline=t+period;
  while (t<nextdeadline) { Sleep(1); t=WinTimeMs(); }
  if (t>=nextdeadline+period)//сильно отстали - ресинхронизация
    nextdeadline=t+period;
  else
    nextdeadline+=period;
  }
else
  nextdeadline=0;
}

int WinGetFPS(void) { return realfps; }
void WinSetTargetFPS(int fps)
{
if (fps<1) fps=0;
if (fps>1000) fps=1000;
targetfps=fps;
}
int WinGetTargetFPS(void) { return targetfps; }
void WinSetVolume(int vol)
{
if (vol<0) vol=0;
if (vol>100) vol=100;
sndvol=vol;
}
int WinGetVolume(void) { return sndvol; }
void WinSoundSetTick(void (*tickcb)(void)) { sndtickcb=tickcb; }
int WinSoundActive(void) { return sndrun; }

unsigned long WinTimeMs(void)
{
return (unsigned long)(timeGetTime()-starttick);
}

void WinWaitKeyBlocking(void)
{
for (;;)
  {
  int scod,hit;
  WinPump();
  if (quitflag) return;
  if (WinGetKey(&scod,&hit)==0) return;
  Sleep(10);
  }
}

int WinGetKey(int *scod,int *hit)
{
if (keyqrd==keyqwr) return -1;
*scod=keyq[keyqrd].scod;
*hit=keyq[keyqrd].hit;
keyqrd=(keyqrd+1)%WINKEY_QUEUE;
return 0;
}

int WinAnyCodFlag(void) { return anycod; }

/*--- мышь ---*/
int MousePresent(void) { return mousepresent; }

void MouseResetSense(void)
{
mousedx=mousedy=0;
mousebtn=0;
mousepress[0]=mousepress[1]=0;
mouserelease[0]=mouserelease[1]=0;
mousepresent=1;
}

void MouseGetDelta(int *dx,int *dy)
{
*dx=mousedx; *dy=mousedy;
mousedx=mousedy=0;
}

int MouseButtons(void) { return mousebtn; }

int MousePressCount(int b)
{
int n;
if (b<0||b>1) return 0;
n=mousepress[b];
mousepress[b]=0;
return n;
}

int MouseReleaseCount(int b)
{
int n;
if (b<0||b>1) return 0;
n=mouserelease[b];
mouserelease[b]=0;
return n;
}

/*--- видео ---*/
unsigned char *WinGetVRAM(void) { return vram; }

void VideoSetMode(int mode)
{
scrnmode=mode;
if (mode==0x13) memset(vram,0,WINVRAMLEN);
InvalidateRect(wnd,NULL,FALSE);
}

int VideoGetMode(void) { return scrnmode; }

void VideoSetBorderColor(int c) { (void)c; }

void VideoSetPalette(const unsigned char *rgb,int startcol,int endcol)
{
int i,r,g,b;
for (i=startcol; i<=endcol; ++i)
  {
  syspalsave[i*3]  =rgb[i*3];
  syspalsave[i*3+1]=rgb[i*3+1];
  syspalsave[i*3+2]=rgb[i*3+2];
  r=rgb[i*3];   r=(r*255+31)/63; if (r>255) r=255;
  g=rgb[i*3+1]; g=(g*255+31)/63; if (g>255) g=255;
  b=rgb[i*3+2]; b=(b*255+31)/63; if (b>255) b=255;
  lut[i*4]  =(unsigned char)b;//DIB 32bpp: B,G,R,x
  lut[i*4+1]=(unsigned char)g;
  lut[i*4+2]=(unsigned char)r;
  lut[i*4+3]=0;
  }
}

void VideoGetPalette(unsigned char *rgb,int startcol,int endcol)
{
int i;
for (i=startcol; i<=endcol; ++i)
  {
  rgb[i*3]  =syspalsave[i*3];
  rgb[i*3+1]=syspalsave[i*3+1];
  rgb[i*3+2]=syspalsave[i*3+2];
  }
}

void VideoShow(void)
{
HDC hdc=GetDC(wnd);
PaintWindow(hdc);
ReleaseDC(wnd,hdc);
#ifdef LILADEBUG
  {
  static int dbgcnt=0;
  int i,nonzero=0;
  if (++dbgcnt==100)
    {
    FILE *f=fopen("vram_dump.pp","wb");
    if (f)
      {
      char tail[16];
      fwrite(vram,1,64000,f);
      fwrite(syspalsave,1,768,f);
      strcpy(tail,"\r\n\r\n -1\r\n  1\r\n");
      fwrite(tail,1,14,f);
      fclose(f);
      printf("vram dumped to vram_dump.pp\n");
      }
    }
  if (dbgcnt%150==0)
    {
    for (i=0; i<WINVRAMLEN; i+=17) if (vram[i]) nonzero++;
    printf("VideoShow #%i nonzero=%i pal9=%i,%i,%i mode=%X wnd=%p\n",
           dbgcnt,nonzero,syspalsave[27],syspalsave[28],syspalsave[29],
           scrnmode,(void*)wnd);
    }
  }
#endif
}

/*--- dos.h время ---*/
void GetDosTime(int *hour,int *minute,int *second,int *hsecond,
                int *year,int *month,int *day,int *dayow)
{
SYSTEMTIME st;
GetLocalTime(&st);
if (hour)    *hour=st.wHour;
if (minute)  *minute=st.wMinute;
if (second)  *second=st.wSecond;
if (hsecond) *hsecond=st.wMilliseconds/10;
if (year)    *year=st.wYear;
if (month)   *month=st.wMonth;
if (day)     *day=st.wDay;
if (dayow)   *dayow=st.wDayOfWeek;
}

/*--- findfirst/findnext (dir.h) ---*/
static HANDLE hfind=INVALID_HANDLE_VALUE;

int findfirst(const char *spec,struct ffblk *ff,int attrib)
{
WIN32_FIND_DATAA fd;
(void)attrib;
if (hfind!=INVALID_HANDLE_VALUE) { FindClose(hfind); hfind=INVALID_HANDLE_VALUE; }
hfind=FindFirstFileA(spec,&fd);
if (hfind==INVALID_HANDLE_VALUE) return -1;
strncpy(ff->ff_name,fd.cFileName,ff_NAMELEN-1);
ff->ff_name[ff_NAMELEN-1]=0;
ff->ff_fsize=fd.nFileSizeLow;
return 0;
}

int findnext(struct ffblk *ff)
{
WIN32_FIND_DATAA fd;
if (hfind==INVALID_HANDLE_VALUE) return -1;
if (!FindNextFileA(hfind,&fd))
  {
  FindClose(hfind);
  hfind=INVALID_HANDLE_VALUE;
  return -1;
  }
strncpy(ff->ff_name,fd.cFileName,ff_NAMELEN-1);
ff->ff_name[ff_NAMELEN-1]=0;
ff->ff_fsize=fd.nFileSizeLow;
return 0;
}

/*--- окно ошибок с конвертацией CP866 -> CP1251 (ANSI системы) ---*/
void WinErrorBox(const char *text)
{
wchar_t wbuf[2048];
char abuf[2048];
int n;
n=MultiByteToWideChar(866,0,text,-1,wbuf,2047);
wbuf[n]=0;
WideCharToMultiByte(1251,0,wbuf,-1,abuf,2047,NULL,NULL);
abuf[2047]=0;
MessageBoxA(wnd,abuf,"Lila",MB_OK|MB_ICONERROR);
}

/*------------------------------------------------------------
  фикс-точка (вместо VRS54A.OBJ) и заполнение памяти
*/
fix fixmul(fix x,fix y)
{
return (fix)(((long long)x*(long long)y)>>16);
}

fix fixdiv(fix x,fix y)
{
long long r;
if (y==0) return 0;
r=((long long)x)<<16;
r/=y;//усечение к нулю - как у DOS idiv
if (r>(long long)0x7FFFFFFF) return (fix)0x7FFFFFFF;//насыщение вместо int0
if (r<(long long)(-0x7FFFFFFF)) return (fix)(-0x7FFFFFFF);
return (fix)r;
}

long lfixmuldiv(long a,fix x,fix y)
{
long long p,r;
if (y==0) return 0;
p=(long long)a*(long long)x;
r=p/y;
if (r>(long long)0x7FFFFFFF) r=0x7FFFFFFF;
if (r<(long long)(-0x7FFFFFFF)) r=-0x7FFFFFFF;
return (long)r;
}

fix fixmulsumdiv(fix a,fix x,fix b,fix y,fix z)
{
long long s;
if (z==0) return 0;
s=(long long)a*(long long)x+(long long)b*(long long)y;
if (s/z>(long long)0x7FFFFFFF) return (fix)0x7FFFFFFF;
if (s/z<(long long)(-0x7FFFFFFF)) return (fix)(-0x7FFFFFFF);
return (fix)(s/z);
}

void MoveDword(void *src,void *dst,int len4)
{
memmove(dst,src,(size_t)len4*4);
}

void SetDword(void *dst,int numbyte4,int val)
{
/*заполнение numbyte4 двойных слов шаблоном val<<16|val;
  все callers передают байтовые val, для них это обычный memset*/
int i;
unsigned int *d=(unsigned int *)dst;
unsigned int pat=((unsigned int)val<<16)|(unsigned int)val;
for (i=0; i<numbyte4; ++i) d[i]=pat;
}

void Setmem(void *dst,int numbyte,int val)
{
/*в оригинале заполнялись слова 16-битным val; все callers передают
  байтовые значения - обычное заполнение байт эквивалентно*/
memset(dst,val,numbyte);
}
