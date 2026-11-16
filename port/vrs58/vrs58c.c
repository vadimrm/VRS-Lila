/*
    ~Постоянная часть СИ-кода Lila
*/

#include "vrsport.h"
#include    "vrs58.h"
#include "kbcod.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>

#define RANDMUL 0x015a4e35L
//медленный общий датчик, копия rand()
void Srand(word seed){ Seed=seed; }
int Rand(void){ Seed=RANDMUL*Seed+1; return ((int)(Seed>>16))&0x7fff; }
//случайность датчика 2 фиксируется, он только для звезд!
void Srand2(word seed){ Seed2=seed; }
int Rand2(void){ Seed2=RANDMUL*Seed2+1; return ((int)(Seed2>>16))&0x7fff; }

//мой быстрый 16-битный общий датчик...
void Srandi(word seed){ Seed16=seed; }
int Randi(void){Seed16=25741*Seed16+13851;return (int)(0x7fff&(Seed16>>1));}
//быстрый 31/32-битный общий качественный датчик ~Urand
void Srandu(word seed){ Seed32=seed; }
int Randu(void)
{ Seed32=RANDUMUL*Seed32+RANDUADD; return ((int)(Seed32>>16))&0x7fff; }

void AP_Null(void){}//пустая заглушка для 'пропущенных адресов' AP

fix iifixdiv(int x,int y) { return fixdiv(itofix(x),itofix(y)); }
fix iiifixmuldiv(int i,int x,int y)// i*x/y
{ return fixmuldiv(itofix(i),itofix(x),itofix(y)); }

void TablesSetup(void)
//загрузка с диска табличных массивов данных и расчёт
{
_fmode=O_BINARY;
if (LoadSinusFile("sinus.lii")!=OK)
  {
  printf("Error loading sinus.lii file !\n");
  EndProgram(101);
  }

if (LoadKeyboardFile("keyboard.lii")!=OK)
  {
  printf("Error loading keyboard.lii file !\n");
  EndProgram(102);
  }

if (LoadFontFile("font.lii")!=OK)
  {
  printf("Error loading font.lii file !\n");
  EndProgram(103);
  }
else ConvertFontMode();

if (LoadSyspalFile("syspal.lii")!=OK) SyspalSetup();

ComputGradcol();
}

word Write(int handle,void far *buf,word len)
{
return (word)write(handle,buf,len);
}

void SetScreenMode(int mode)
//установка видео-моды mode с очисткой экрана (порт: окно Win32)
{
VideoSetMode(mode);
actscrnmode=mode;
}

int GetScreenMode(void)
//возвращает текущую видео-моду экрана
{
return VideoGetMode();
}

void SetBorderColor(int bclr)
//цвет рамки экрана дисплея (в окне не имеет смысла)
{
VideoSetBorderColor(bclr);
}

void NewInterruptHandlers(void)
//порт: вместо перехвата irq - ввод/время уже обеспечивает бэкенд;
//здесь только сброс системного времени
{
vrstime0=vrstime1=0;
}

void OldInterruptHandlers(void)
//возврат старых обработчиков (порт: пусто)
{
}

int LoadKeyboardFile(char *fname)
{
int hnd;
hnd=open(fname,O_RDONLY|O_BINARY,S_IREAD);
if (hnd==-1) return ERR;//нет такого файла
Read(hnd,scod2kbcod,sizeof(scod2kbcod));
close(hnd);
return OK;
}

int LoadSinusFile(char *fname)
{
int hnd;
hnd=open(fname,O_RDONLY|O_BINARY,S_IREAD);
if (hnd==-1) return ERR;//нет такого файла
Read(hnd,Fsin,sizeof(Fsin));
close(hnd);
return OK;
}

int LoadFontFile(char *fname)
//загрузка с диска текстового фонта 8x8
{
int hnd;
hnd=open(fname,O_RDONLY|O_BINARY,S_IREAD);
if (hnd==-1) return ERR;//нет такого файла
Read(hnd,fnt.buffer,2048);
close(hnd);
return OK;
}

void SetMSGBufferToEmpty(void)
//стирание msg-буфера
{
msgwrite=msgread=0;
}

void SetAllAPAddress(void)
//установка внутренних адресов всех AP
{
int n;//адресный номер AP

for (n=1; n<=numap; ++n)
  {
  //стирание msg-буфера
  msgwrite=msgread=0;
  //запись одного мессага
  AddIntMSG(ADDR_VRS,ADDR_ALL,MSG_AP_SET_ADDR,n);
  //вызов очередной AP
  AP[n].taskadress();
  }
}

dword VRSTime(void)
//дает время, без сбоев из-за volatile vrs_time;
{
dword time;
disable();
time=vrs_time;
enable();
return time;
}

int MSGBufNotFull(void)
//если в msg-буфере есть место, возвращает OK, иначе ERR
{
int msgwr;
msgwr=msgwrite;
//следующая позиция записи msg-буфера будет
if (++msgwr>=MSGBUF) msgwr=0;
//если следующая запись наедет на начало кольца msg-буфера
if (msgwr==msgread) return ERR;
return OK;
}

int GetIntMSG(int poz)
//возвращает целое число, стандартно составленное из полей data[0]/[1]
//сообщения из msg-буфера в его позиции poz
{
//data[0]=младший байт, data[1]=старший байт
return ( (((int)msg[poz].data[1])<<8) + (int)msg[poz].data[0] );
}

int OpenRead(char *fname,long offset,void far *buf,word len)
//загрузка куска ( < 64 kb ) файла в RAM
{
int hnd;
hnd=open(fname,O_RDONLY|O_BINARY,S_IREAD);
if (hnd==-1) return ERR;//нет такого файла;
lseek(hnd,offset,SEEK_SET);
Read(hnd,buf,len);
close(hnd);
return OK;
}

word Read(int handle,void far *buf,word len)
{
return (word)read(handle,buf,len);
}

int AddIntMSG(int srcaddr,int dstaddr,word message,int intdata)
//добавление в msg-буфер одного сообщения, для которого пара
  //полей data[0]/[1] представляют собой одно целое число!
{
if (AddShortMSG(srcaddr,dstaddr,message)!=OK) return ERR;
if (--msgwrite<0) msgwrite=MSGBUF-1;//шаг назад
msg[msgwrite].data[0]=intdata&0xFF;//data[0]=младший байт intdata
msg[msgwrite].data[1]=intdata>>8;  //data[1]=старший байт intdata
if (++msgwrite>=MSGBUF) msgwrite=0;//шаг вперед
return OK;
}

int Add2byteMSG(int srcaddr,int dstaddr,word message,int data0,int data1)
//--//-- data0/1: два байта
{
if (AddShortMSG(srcaddr,dstaddr,message)!=OK) return ERR;
if (--msgwrite<0) msgwrite=MSGBUF-1;
msg[msgwrite].data[0]=(byte)data0;
msg[msgwrite].data[1]=(byte)data1;
if (++msgwrite>=MSGBUF) msgwrite=0;
return OK;
}

int AddShortMSG(int srcaddr,int dstaddr,word message)
//добавление в msg-буфер одного короткого сообщения, для которого
  //поля data[] не нужны;
{
if (MSGBufNotFull()==ERR)
  {
  printf("No space for MSG !\n");
  return ERR;//сообщение не помещено!
  }
msg[msgwrite].time=VRSTime();
msg[msgwrite].count=0;
msg[msgwrite].srcaddr=srcaddr;
msg[msgwrite].dstaddr=dstaddr;
msg[msgwrite].cmd=message;
//следующая позиция записи msg-буфера
if (++msgwrite>=MSGBUF) msgwrite=0;
return OK;
}

void OverShortMSG(int msgnum,int srcaddr,int dstaddr,word message)
//запись в msg-буфер сообщения поверх старого номер msgnum
{
msg[msgnum].time=VRSTime();
msg[msgnum].count=0;
msg[msgnum].srcaddr=srcaddr;
msg[msgnum].dstaddr=dstaddr;
msg[msgnum].cmd=message;
}

void AllPANdataSetup(void)
{
int n;
for (n=0; n<MAXPAN; ++n) ZeroPANhead(n);
}

void AllMEMdataSetup(void)
{
int n;
for (n=0; n<MAXMEM; ++n) ZeroMEMhead(n);
}

void WaitKeyPress(void)
//ждет нажатия любой клавиши
{
anycodflag=0; while (anycodflag==0) ;
}

d3poz fixXYZang2D3poz(fix x,fix y,fix z,angle ang)
//преобразует вектор с fix компонентами в d3poz, угол в angle!
{
d3poz p;
p.x0=x;
p.y0=y;
p.z0=z;
p.angx=ang;
return p;
}

d3poz XYZang2D3poz(int x,int y,int z,angle ang)
//преобразует вектор с целыми компонентами в d3poz, угол в angle!
{
return fixXYZang2D3poz(itofix(x),itofix(y),itofix(z),ang);
}

d3poz fixXYZgrad2D3poz(fix x,fix y,fix z,int grad)
//преобразует вектор с fix компонентами в d3poz,
//угол grad в градусах !
{
return fixXYZang2D3poz(x,y,z,grad2ang(grad));
}

d3poz XYZgrad2D3poz(int x,int y,int z,int grad)
//преобразует вектор с целыми компонентами в d3poz,
//угол grad в градусах !
{
return XYZang2D3poz(x,y,z,grad2ang(grad));
}

d3poz DeltaD3poz(d3poz poz1,d3poz poz0)
//возвращает разность двух позиций (poz1-poz0)
{
return ConvD3pozFromEarth2AP(poz1,poz0);
}

d3poz ShiftD3poz(d3poz shift,d3poz poz)
//сдвиг D3-позиции poz на вектор shift
{
return ConvD3pozFromAP2Earth(shift,poz);
}

d3poz ConvD3pozFromAP2Earth(d3poz pn,d3poz ap)
//преобразование координат (панели) из системы AP в систему земли
//pn координаты панели в системе AP
//ap координаты AP в системе земли;
{
d3poz pear;//координаты панели в системе земли

fsin=fsinus(ap.angx);
fcos=fcosin(ap.angx);
pear.x0=fixmul(fcos,pn.x0)-fixmul(fsin,pn.y0)+ap.x0;
pear.y0=fixmul(fsin,pn.x0)+fixmul(fcos,pn.y0)+ap.y0;
pear.z0=pn.z0+ap.z0;
pear.angx=pn.angx+ap.angx;
return pear;
}

d3poz ConvD3pozFromEarth2AP(d3poz pea,d3poz ap)
//преобразование координат (панели) из системы земли в систему AP
//pea координаты панели в системе земли
//ap  координаты AP в системе земли;
{
d3poz pap;//координаты панели в системе AP

fsin=fsinus(ap.angx);
fcos=fcosin(ap.angx);
pap.x0=fixmul(fcos,pea.x0-ap.x0)+fixmul(fsin,pea.y0-ap.y0);
pap.y0=fixmul(fcos,pea.y0-ap.y0)-fixmul(fsin,pea.x0-ap.x0);
pap.z0=pea.z0-ap.z0;
pap.angx=pea.angx-ap.angx;
return pap;
}

fix fcosin(angle ang)
{
return fsinus(ang+ANG90G);//т.к. cos(a)=sin(a+90гр)
}

fix fsinus(angle ang)
//используем 1/4 диапазона sin, от 0 до PI/2 включительно!
{
int neg=0;
fix f;
if (ang>=ANG180G) { ang-=ANG180G; neg=1; }
if (ang> ANG90G)    ang=ANG180G-ang;
f=Fsin[ang>>2];
return (neg==0)? f:-f;
}

angle grad2ang(int grad)
//приводит +-аргумент к диапазону 0...359 градусов и
//возвращает угол в лидусах, единицах угла VRS Lila
{
long lgrad;
angle ang;
lgrad=(360+grad%360)%360;
ang=(angle)((lgrad<<16)/360);
//округляем угол !
return ANGSTEP*(ang/ANGSTEP);
}

void ConvertFontMode(void)
//преобразование моды развертки фонта из 1 в 2
//
//мода 1: 1 байт дает 1 гориз. строку бит слева (msb) направо (lsb),
//        8 байт дают 8 строк сверху вниз...
//byte pattern[256][8];//[ASCII-код][номер строки]
//
//мода 2: 1 байт дает 1 вертик. столбец бит снизу (msb) вверх (lsb),
//        8 байт дают 8 столбцов слева направо
//byte pattern[256][8];//[ASCII-код][номер столбца]
{
int a,b,c,d,i1,i2,pat1[8],pat2[8];

for (a=0; a<256; ++a)
  {
  for (i1=0; i1<8; ++i1) pat1[i1]=fnt.pattern[a][i1];
  b=128;
  for (i2=0; i2<8; ++i2)
    {
    pat2[i2]=0;
    for (i1=0; i1<8; ++i1)
      {
      c=pat1[i1]&b;
      d= (c>0)? 1:0;
      pat2[i2]+=(d<<i1);
      }
    b=b>>1;
    }
  for (i2=0; i2<8; ++i2) fnt.pattern[a][i2]=pat2[i2];
  }
}

void SetYbuf(int ydist)
//заполнение y-буфера профилем фона с расстоянием ydist
{
//константа заполнения 2-байтная!
SetRAM(memory[PEN.ymem].rampntr,PEN.ramlen,ydist,2);
}

void MouseReset(void)
//порт int33 fn 0x00: проверка наличия мыши
{
mousepresent= MousePresent()? YES:NO;
}

void MouseSense(void)
//порт int33 fn 0x1A: стандартизация чувствительности
{
MouseResetSense();
}

void MouseStatus(void)
//определяет все параметры мыши (порт int33 fn 0x0B/03/05/06)
{
int mkeys;

MouseGetDelta(&mousedx,&mousedy);//счетчики перемещения
mkeys=MouseButtons();            //бит 0 (1) == 1: левая (правая) клавиша
mouseb0on =MousePressCount(0);   //число нажатий b0 с последнего опроса
mouseb0off=MouseReleaseCount(0);
mouseb1on =MousePressCount(1);
mouseb1off=MouseReleaseCount(1);
mouseb0=mkeys&1;
mouseb1=(mkeys&2)>>1;
mousedy=-mousedy;//так более логично!
}

//ПОРТ: вместо ISR int 9 - перекачка событий клавиатуры из очереди
//бэкенда Windows в кольцевой буфер kbbuf (те же scod-коды!)
void FillKbbufFromWin(void)
{
for (;;)
  {
  int scod,hit;
  if (WinGetKey(&scod,&hit)!=0) break;//очередь пуста
  anycodflag=1;//принят какой-то код...
  if (kbbufvalid!=YES) break;//буфер не готов
  //запись кода в буфер нижнего уровня
  kbbuf[kbwrite].t0=vrstime0;
  kbbuf[kbwrite].t1=vrstime1;
  kbbuf[kbwrite].scod=scod;//уже с +128 для расширенных, как в ISR
  kbbuf[kbwrite].hit=hit;//WINKEY_ON=KEYON, WINKEY_OFF=KEYOFF
  //установка позиции для записи в буфер следующего кода клавиши
  if (++kbwrite>=KBBUF) kbwrite=0;
  //если следующая запись наедет на начало кольца кодов
  if (kbwrite==kbread)
    //возврат на шаг для затирания конца вместо начала
    if (--kbwrite<0) kbwrite=KBBUF-1;
    //можно дать звуковой сигнал ошибки переполнения буфера !
  }
}

void MSGDel(int num)
//стираем сообщение в позиции буфера num
{
msg[num].cmd=MSG_NULL;
//опционально печатаем (об этом сбое)
printf("MSG from %i to %i deleted !\n",msg[num].srcaddr,msg[num].dstaddr);
}

void KeyboardBufferScan(void)
//просмотр буфера клавиатуры нижнего уровня и добавление kb-данных
  //в буфер сообщений, с преобразованием kb-кодов scan3.h->kbcod.h
{
int kbcod,kbwr;//для защиты от фонового изменения kbwrite в irq1 !
dword time;

kbwr=kbwrite;
while (kbread!=kbwr)//конец сканирования когда kb-буфер пуст!
  {
  //преобразование scod->kbcod
  kbcod=scod2kbcod[kbbuf[kbread].scod];
  //запись данных из буфера клавиатуры в буфер сообщений
  time=kbbuf[kbread].t1;
  msg[msgwrite].time=(time<<16)|kbbuf[kbread].t0;
  msg[msgwrite].count=0;//при рождении всегда 0
  msg[msgwrite].srcaddr=ADDR_NOBODY;//этот псевдоадрес только здесь!
  msg[msgwrite].dstaddr=ADDR_VRS;//к системе!
  msg[msgwrite].cmd=MSG_KBPC_DATA;
  msg[msgwrite].data[0]=kbbuf[kbread].hit;
  msg[msgwrite].data[1]=kbcod;

  //следующая позиция записи msg-буфера
  if (++msgwrite>=MSGBUF) msgwrite=0;
  //если следующая запись наедет на начало кольца msg-буфера
  if (msgwrite==msgread)
    {
    //возврат на шаг в msg-буфере
    if (--msgwrite<0) msgwrite=MSGBUF-1;
    //код из текущей позиции kb-буфера будет извлечен в следующий раз
    break;//конец сканирования, msg-буфер полон!
    }
  else//в msg-буфере есть еще место для записи, сдвигаем kb-позицию
    {
    //опциональная печать кодов
//    printf("hit=%i  kbcod=%i\n",kbbuf[kbread].hit,kbcod);
    //следующая позиция чтения kb-буфера
    disable();//чтобы в irq1 не попадало неправильное kbread=KBBUF !!!
    if (++kbread>=KBBUF) kbread=0;
    enable();
    }
  }
}

void MessageDeleter(void)
//системный поглотитель использованных/невостребованных сообщений
//сначала обнуляет все ненужные более сообщения (если это не
//сделано другими), а затем сжимает буфер мессагов, оставляя
//там только еще актуальные (неотжившие) сообщения;
//время жизни сообщений:
//от системы    к системе    2 витка
//от системы    к AP         1 виток, в т.ч. 'ко всем' !
//от AP         к системе    1 виток
//от младшей AP к старшей AP 1 виток,
//от старшей AP к младшей AP 2 витка, в т.ч. 'ко всем' !
//AP выполняются по возрастанию адресов !
{
int msgrd,msgwr;//для локального сканирования буфера сообщений

msgrd=msgread;
msgwr=msgwrite;
while (msgrd!=msgwr)//сканируем весь буфер сообщений
  {
  //увеличиваем возраст сообщения на один виток
  ++msg[msgrd].count;

  if (msg[msgrd].cmd!=MSG_NULL)//если сообщение не пустое
    //анализируем адреса и возраст сообщения
    {
    if (msg[msgrd].srcaddr==ADDR_VRS)//от системы
      {
      if (msg[msgrd].dstaddr==ADDR_ALL)//ко всем
        msg[msgrd].cmd=MSG_NULL;//стираем сообщение
      else
      if (msg[msgrd].dstaddr==ADDR_VRS)//система к системе!
        {
        if (msg[msgrd].count>=2)//если время жизни кончилось
          //стираем то, что _должна была_
          MSGDel(msgrd);//стереть VRS в SystemMessageFiltr() !
        //else оставляем сообщение на следующий виток
        }
      else//к индивидуальной AP
        MSGDel(msgrd);//стираем то, что _должна была_ стереть AP
      goto nextmsg;
      }

    if (msg[msgrd].dstaddr==ADDR_VRS)//от AP к системе
      {
      //анализ сообщения и соответствующие действия системы
      //либо делать это в SystemMessageFiltr() !!!
      msg[msgrd].cmd=MSG_NULL;//стираем сообщение
      goto nextmsg;
      }

    //теперь остался случай от AP к AP

    if (msg[msgrd].srcaddr<msg[msgrd].dstaddr)//от младшей к старшей
      MSGDel(msgrd);//стираем то, что _должна была_ стереть AP
    else//от старшей к младшей (в том числе 'ко всем' !)
      if (msg[msgrd].count>=2)//если время жизни кончилось
        MSGDel(msgrd);//стираем то, что _должна была_ стереть AP
      //else оставляем сообщение на следующий виток
    }
nextmsg:
  //переход к следующему сообщению
  if (++msgrd>=MSGBUF) msgrd=0;
  }

//сжимаем буфер сообщений

//пока просто оставляем кусок буфера от первого до последнего
  //нестертого сообщения, без 'ужатия пустых внутренностей'

msgrd=msgread;
msgwr=msgwrite;
//ищем первое непустое сообщение
while (msgrd!=msgwr)
  {
  if (msg[msgrd].cmd!=MSG_NULL) break;
  if (++msgrd>=MSGBUF) msgrd=0;
  }
msgread=msgrd;//корректируем начало буфера

//ищем последнее непустое сообщение
while (msgrd!=msgwr)
  {
  if (msg[msgrd].cmd!=MSG_NULL)
    {
    msgwrite=msgrd+1;//корректируем конец буфера
    if (msgwrite>=MSGBUF) msgwrite=0;
    }
  if (++msgrd>=MSGBUF) msgrd=0;
  }
}

void PaintFullBitmap(int bm,int col)
//закраска целого битмапа bm (это номер блока памяти) цветом col
{
int x1,z1;
x1=memory[bm].lenx-1;
z1=memory[bm].lenz-1;
PaintBitmap(bm,0,0,x1,z1,col);
}

void PaintBitmap(int bm,int x0,int z0,int x1,int z1,int col)
//быстрая закраска прямоугольного участка (x0,z0)...(x1,z1)
//битмапа номер bm постоянным цветом col;
{
int x,lenx,lenz,maxx,maxz;
byte far *tmemram;

if ((bm<=ISNOT)||(x0>x1)||(z0>z1)) return;
lenx=memory[bm].lenx; maxx=lenx-1;
lenz=memory[bm].lenz; maxz=lenz-1;
if ((x0>maxx)||(x1<0)) return;
if ((z0>maxz)||(z1<0)) return;
x0=max(x0,0); x1=min(x1,maxx);
z0=max(z0,0); z1=min(z1,maxz);
tmemram=memory[bm].rampntr;
if (memory[bm].rammode!=2) ConvertMEMmode(bm,2);
//мода битмапа 2 !
for (x=x0; x<=x1; ++x)//заливаем текстуру по столбцам
  SetRAM(tmemram+x*lenz+z0,z1-z0+1,col,1);
//если dx>>dz то можно ускорить простым циклом по x ! @
}

void RandColors(int *col,int len,int r,int g,int b,int dr,int dg,int db)
//заполнение массива col[0...(len-1)] случайными цветами
//со средними компонентами r,g,b и ~дисперсиями dr,dg,db
//получаются не слишком красивые наборы...
{
int r0,dr0,g0,dg0,b0,db0,ri,gi,bi,i;
r0=r-dr;
g0=g-dg;
b0=b-db;
dr0=max(2*dr,0)+1;
dg0=max(2*dg,0)+1;
db0=max(2*db,0)+1;
for (i=0; i<len; ++i)
  {
  ri=r0+Randu()%dr0;
  gi=g0+Randu()%dg0;
  bi=b0+Randu()%db0;
  col[i]=Color(ri,gi,bi);
  }
}

void bmpixel(int bm,int x,int z,int col)
//закраска пикселя (x,z) битмапа bm цветом col;
//можно значительно ускорить ! @
{
PaintBitmap(bm,x,z,x,z,col);
}

void bmline(int bm,int x0,int z0,int grad,int len,int col)
//закраска пикселей битмапа bm постоянным цветом col:
//пиксели на линии от (x0,z0) под углом grad (пока только 0 и 90
//градусов), длина линии len>0 пикселей, толщина 1 пиксель;
//можно значительно ускорить ! @
{
int x1,z1;
if (len<=0) return;
if (grad==0)
  {
  z1=z0;
  x1=x0+len-1;
  }
else//90 градусов
  {
  x1=x0;
  z1=z0+len-1;
  }
PaintBitmap(bm,x0,z0,x1,z1,col);
}

int pantext(int pan,int glen,int fgcol,int hor,int ver,char *format,...)
//аналог текстовой printf(), возвращает число литер в строке вывода;
//вывод текста в битмап панели pan цветом fgcol, не меняя фона;
//hor/ver=номер текстовой позиции первой литеры строки на битмапе,
//левый верхний _текстовый_ угол битмапа = 0/0 !
//всё это производятся внутри границ виртуальной рамки толщиной glen!
{
va_list argptr;
char st[256];//буфер вывода с запасом
int bm,x,z,x0,z2,lenst,lx,lz;

if (glen<0) return -1;
bm=panel[pan].memtex;
lx=memory[bm].lenx;
lz=memory[bm].lenz;
x0=glen;
z2=lz-glen;
x=x0+hor*8;
z=z2-ver*8-8;
if ((x<0)||(x>=lx)||(z<0)||(z>=lz)) return -1;

va_start(argptr,format);
lenst=vsprintf(st,format,argptr);//вывод строки в буфер
va_end(argptr);
PaintText(st,lenst,bm,x,z,PAINTFG,fgcol,0);
return lenst;
}

void panfill(int pan,int glen,int bgcol,int fglet,int letcol,int stepz)
//сначала закраска битмапа панели pan фоном цвета bgcol (при bgcol>=0)
//и потом заполнение его текстурой литеры fglet (при fglet>0, напр.
//шахматы 177) цветом letcol;
//между строками литер будет сдвиг stepz, а не просто 8 пикселей !!
//всё это производятся внутри границ виртуальной рамки толщиной glen!
{
int bm,x,z,x0,z0,x1,z1,i,slen;
char str[40];

if (glen<0) return;
bm=panel[pan].memtex;
x0=z0=glen;
x1=memory[bm].lenx-glen-1;
z1=memory[bm].lenz-glen-1;
//фон
if (bgcol>=0) PaintBitmap(bm,x0,z0,x1,z1,bgcol);

if ((fglet==0)||(stepz<=0)) return;
for (i=0; i<40; ++i) str[i]=(char)fglet;
//литерная текстура
for (z=z0; z<=z1-7; z+=stepz)
  for (x=x0; x<=x1-7; x+=320)
    {
    slen=(x1-x+1)/8;//литер до начала рамки справа
    if (slen>40) slen=40;
    PaintText(str,slen,bm,x,z,PAINTFG,letcol,0);
    }
}

void EndProgram(int cod)
//завершение программы: по требованию юзера cod=0, иначе по ошибке!
{
soundflag=OFF;
SoundOnOff();
//возврат старых обработчиков прерываний и их установок
OldInterruptHandlers();
if (cod!=0)
  {
  char etext[256];
  sprintf(etext,"Exit Program Error Code %i !\nPress any key to end program !",cod);
  WinErrorBox(etext);
  }
//старый режим дисплея
SetScreenMode(startscrnmode);
WinDone();
exit(0);
}

#define RAMARENA 8388608L//8МБ арены вместо всей far RAM DOS
void RAMAlloc(void)
//порт: выделение арены памяти для блоков памяти (была вся far RAM DOS)
{
ramfullfree=RAMARENA;
if (ramfullfree<MINRAMSIZE)
  {
  printf("RAM size %li too small for start Lila !\n",ramfullfree);
  EndProgram(1);
  }

//выделение RAM
ramfullstart=(byte huge *)malloc(ramfullfree);
if (ramfullstart==NULL)
  {
  printf("malloc() error !\n");
  EndProgram(2);
  }

//надежная установка начального адреса и длины кратными 4-м
ramfullfree=4*(ramfullfree/4-2);
ramfullstart=(byte huge *)(((uintptr_t)ramfullstart+3)&~(uintptr_t)3);

ramstart=ramfullstart;
ramfree=ramfullfree;
}

void UnLockMEM(int n)
//открепляет заголовок и блок памяти
{
byte huge *memram;
word len;

if (n<=ISNOT) return;//ошибка или аварийный блок ISNOT=0 !

if (memory[n].count<=0) return;//ошибка откреплений !

if (--memory[n].count>0) return;//уменьшение залочки

//когда счетчик становится равен нулю:
++freemems;
len=memory[n].fullen;
//корректируем общий размер свободной RAM
ramfullfree+=len;
//начало памяти блока
memram=(byte huge *)memory[n].rampntr;
//если блок примыкает к началу 'непрерывного куска RAM'
if ((memram+len)==ramstart)//сливаем их вместе
  {
  //корректируем размер и начало 'непрерывного куска RAM'
  ramfree+=len;
  ramstart=memram;
  //теперь _совсем_ уничтожаем заголовок
  ZeroMEMhead(n);
  if (maxmemnum==n)//если это заголовок с краевым номером
    //освобождаем все пустые заголовки в конце группы
    {
    //ищем с конца первый ненулевой длины блок
    for (n=MAXMEM-1; n>0; --n) if (memory[n].fullen!=0) break;
    maxmemnum=n;
    }
  //если номер внутри группы, пустой заголовок пока не освобождается
  }
//блок с нулевым счетчиком закреплений остается виртуально принадлежать
//своему овнеру, но может быть распределен в любой момент кому угодно !
}

void UnLockPAN(int n)
//открепляет панель и ее ресурсы
{
int cnt;

if (n<=ISNOT) return;//ошибка или аварийная панель ISNOT=0 !

cnt=--panel[n].count;
if (cnt<0) return;//ошибка откреплений !

//открепляем ресурсы
UnLockMEM(panel[n].memtex);
UnLockMEM(panel[n].memconv);

if (cnt==0)//последнее открепление, уничтожаем заголовок
  {
  ++freepans;
  ZeroPANhead(n);
  if (maxpannum==n)//если удаляется 'последняя' панель
    //уменьшаем макс номер панели, но не ниже нуля
    {
    //ищем первую с конца свободную панель
    for (n=MAXPAN-1; n>0; --n) if (panel[n].count>0) break;
    maxpannum=n;
    }
  }
}

void LockPAN(int n)
//ресурсы панели n закрепляются;
//счетчик ссылок на заголовок панели count: =0=заголовок свободен,
//=1 и только =1 если заголовок используется, более одной залочки
//панели лучше не делать - да это и не надо!
{
int cnt;
if (n<0) return;//фатальная ошибка
cnt=panel[n].count;
if (cnt==0)//при первом закреплении
  {
  --freepans;//корректируем число свободных заголовков панелей
  LockMEM(panel[n].memtex);//ресурсы панели закрепляются
  LockMEM(panel[n].memconv);
  ++panel[n].count;
  return;
  }
//else повторное закрепление

if (n==ISNOT) return;//аварийная панель ISNOT=0 более не закрепляется!
//else рабочая панель
LockMEM(panel[n].memtex);//ресурсы панели закрепляются
LockMEM(panel[n].memconv);
++panel[n].count;
}

void LockMEM(int n)
//закрепляется блок памяти, разрешена многократная залочка
//неаварийного блока!
{
int cnt;
if (n<0) return;//фатальная ошибка
cnt=memory[n].count;
if (cnt==0)//при первом закреплении
  {
  ramfullfree-=memory[n].fullen;//корректируем общую длину свободной памяти
  --freemems;//и кол-во свободных заголовков блоков памяти
  ++memory[n].count;
  return;
  }
//else повторное закрепление

if (n==ISNOT) return;//аварийный блок ISNOT=0 более не закрепляется!
//else рабочий блок
++memory[n].count;//только увеличиваем залочку
}

int ConvertMEMmode(int mem,int rammode)
//преобразование моды развертки блока памяти mem в моду rammode
{
byte far *memram;
byte far *tmpram;
byte far *mram;
byte far *tram;
word memlen,l,x,z;
int lenx,lenz,tmem,mode,result;

mode=memory[mem].rammode;
if (mode==rammode) return OK;

memory[mem].rammode=rammode;//т.о. после ошибки мода как после OK!
result=OK;
switch (mode)//определение исходной моды
  {
  default://другие моды пока не преобразуются
    return ERR;

  case 1://исходная мода 1 (pp-файл): слева вправо, потом вниз
    //начало блока памяти, его длина и размеры x,z
    memram=memory[mem].rampntr;
    memlen=memory[mem].len;
    lenx=memory[mem].lenx;
    lenz=memory[mem].lenz;
    //проверка совпадения длин
    if (memlen!=lenx*lenz) return ERR;
    //временно запрашиваем блок памяти размером с исходный
    tmem=GetLockMEM(ADDR_VRS,memlen);
    if (tmem<=ISNOT) { result=ERR; goto end; }
    CopyMEM(tmem,mem);//копируем mem->tmem
    tmpram=memory[tmem].rampntr;
    switch (rammode)//определение преобразованной моды
      {
      default://другие моды пока не используются...
        result=ERR; break;

      case 3://нужна мода 3 (нет сейчас): слева вниз, потом вправо
        for (l=0; l<memlen; ++l)//это медленный вариант!
          {
          x=l%lenx;
          z=l/lenx;
          memram[x*lenz+z]=tmpram[l];
          }
        break;

      //исходная мода 1 (pp-файл): слева вправо, потом вниз
      case 2://нужна мода 2 (для рендера): слева вверх, потом вправо
        for (x=0; x<lenx; ++x)//быстрый вариант
          {
          mram=memram+x*lenz;
          tram=tmpram+(lenz-1)*lenx+x;
          for (z=0; z<lenz; ++z)//можно еще чуть ускорить...
            {
//            memram[x*lenz+z]=tmpram[(lenz-z-1)*lenx+x];
            *(mram++)=*tram;
            tram-=lenx;
            }
          }
        break;
      }
    break;
  }

UnLockMEM(tmem);
end:
return result;
}

void CopyRAM(void far *dst,void far *src,word len)
//порт: копирует len байт памяти src в память dst (memmove из-за
//возможного перекрытия при CompressMEM)
{
memmove(dst,src,len);
}

void CopyMEM(int dstmem,int srcmem)
//копирует (по возможности) весь блок памяти srcmem в dstmem
//с защитой от выхода за границы блока dstmem!
//без контроля мод развертки блоков!
{
word slen,dlen,fullen;
byte far *sram;
byte far *dram;

sram=memory[srcmem].rampntr;
dram=memory[dstmem].rampntr;

slen=memory[srcmem].fullen;
dlen=memory[dstmem].fullen;
fullen= (slen>dlen)? dlen:slen;
//здесь для скорости используется fullen вместо логичных len!
CopyRAM(dram,sram,fullen);
}

void SetRAM(void far *dst,word numval,int val,int lenval)
//заполнение dst numval раз байтом val (lenval=1) с шагом байт
//           или numval раз    int val (lenval=2) с шагом 2 байта!
//опасно применять эту ф-ю при работе с содержимым блоков
//памяти без тщательного контроля невыхода за их границы! @
{
//порт: заполнение байтами/словами (оригинальный пролог с numval<<=1
//удален - мой цикл ниже сам заполняет словами numval штук!)
if (lenval<=1)
  memset(dst,val&0xFF,numval);
else
  {
  unsigned char *d=(unsigned char *)dst;
  unsigned char lo=(unsigned char)(val&0xFF),hi=(unsigned char)((val>>8)&0xFF);
  word i;
  for (i=0; i<numval; ++i) { d[0]=lo; d[1]=hi; d+=2; }
  }
}

void SetMEMtoConstval(int mem,int val)
//заполняет память блока mem байтовой ! константой val
{
if (mem<=ISNOT) return;
//здесь для скорости используется fullen вместо логичных len
SetRAM(memory[mem].rampntr,memory[mem].fullen,val,1);
}

void Bestpoz(int dir,int mode,int pan)
//юзер -> в удобную позицию наблюдения панели pan, или
//панель pan -> в удобную позицию наблюдения юзером,
//дистанция до панели определяется по mode;
{
fix yx,yz,yy,lenx,lenz;
d3poz delta;
int own;

if (pan<=ISNOT) return;

lenx=panel[pan].flenx;
lenz=panel[pan].flenz;
//позиция лицом к передней стороне панели, в середине по x/z
delta.angx=0;
delta.x0=lenx/2;
delta.z0=lenz/2;

if (mode==MODEPAN)//по y чтобы панель была макс крупной,
  //но целиком вписывалась в экран
  {
  yx=yz=0;
  if (PEN.lenx!=0) yx=fixmuldiv(PEN.poz.y0,lenx,PEN.lenx);
  if (PEN.lenz!=0) yz=fixmuldiv(PEN.poz.y0,lenz,PEN.lenz);
  yy=max(yx,yz);
  }
else//MODEBMP
  //битмап панели с увеличением по x 1:1
  yy=fixmul(PEN.poz.y0,panel[pan].fmulx);

if (yy<FIXYPMIN) yy=FIXYPMIN+100;
delta.y0=-yy;
//учет скоса взгляда
if (PEN.poz.y0!=0)
  delta.z0-=fixmuldiv(PEN.poz.z0+PEN.lenz/2,yy,PEN.poz.y0);

own=panel[pan].owner;
if (dir==USER2PAN)
  {
  //определяем ear-координаты панели по последним ap
  panel[pan].ear=ConvD3pozFromAP2Earth(panel[pan].ap,AP[own].ear);
  //ставим _юзера_ на delta перед панелью
  PEN.ear=ShiftD3poz(delta,panel[pan].ear);
  PEN.usershift=YES;
  }
else//PAN2USER
  {
  delta.x0=-delta.x0;
  delta.y0=-delta.y0;
  delta.z0=-delta.z0;
  //ставим _панель_ на delta перед юзером
  panel[pan].ear=ShiftD3poz(delta,PEN.ear);
  //преобразуем координаты панели из системы земли в систему AP
  panel[pan].ap=ConvD3pozFromEarth2AP(panel[pan].ear,AP[own].ear);
  panel[pan].earvalid=YES;
  panel[pan].penvalid=NO;//устанавливаем valid флаги панели!
  }
}

void PENSetup(void)
//первоначальная установка полей PEN
{
word pr4;

pr4=(FP_OFF((void far*)PEN.ram))%4;
//выравниваем penram по границе dword
penram=PEN.ram+(4-(int)pr4);

//масштаб-е PEN-битмапа в PEN-панель для графмоды 320x200
//приводящее к круглости круга ->320x240
penbmpfmulz=FIX6D5;
penbmpfmulz1=FIX5D6;
//мода развертки penram[]
PEN.rammode=0;
//признак отсутствия памяти полукадра
PEN.tmpmem=ISNOT;
}

void ZeroMEMhead(int n)
{
memory[n].owner=ADDR_NOBODY;
memory[n].count=0;
memory[n].len=0;
memory[n].fullen=0;
//memory[n].rampntr=NULL;
memory[n].rammode=0;
memory[n].lenx=0;
memory[n].lenz=0;
memory[n].transflag=NO;
memory[n].fmulz=FIX1;
}

void WarningWKP(char *str,int code)
//сообщение кода не фатальной ошибки, ожидание keypress
{
if (printwarn==OFF) return;
printf("%sError code is %i\n",str,code);
printf("Press any key for continue !\n");
WaitKeyPress();
}

int GetPAN(int owner)
//возвращает номер выделенной панели или ISNOT при ошибке;
//всегда устанавливает код ошибки в errpan;
{
int n;

errpan=0;//нет ошибки

//ищем номер первой _незакрепленной_ панели
for (n=0; n<MAXPAN; ++n) if (panel[n].count<=0) break;
if (n>=MAXPAN)
  {errpan=1;//нет свободных номеров!
  WarningWKP("PAN>=MAXPAN !\n",errpan);
  return ISNOT;
  }
//n=номер новой панели, в самый первый раз n=0=ISNOT !
if (maxpannum<n) maxpannum=n;
//устанавливаем наиболее часто используемые значения полей
ZeroPANhead(n);
panel[n].owner=owner;
//неаварийная панель будет иметься в мире:
if (n>ISNOT) panel[n].flag=YES;
else//поля аварийной панели
  {
  panel[n].flenx=FIX1;
  panel[n].flenz=FIX1;
  }
return n;
}

#define MAXNUMSTAR    10//макс число звезд на одной строке кадра;
void RenderStars(void)
//рендер звезд в буфер кадра по варианту starvar=0...(STARVAR-1)
//с увеличением числа звёзд от нуля до максимума;
//звёзды только выше горизонта или везде, согласно spaceflag;
{
//макс. уменьшение яркости звезды при мерцании, 0...15
static int ampstar[STARVAR]={0, 2, 2, 2, 0};
//количество звезд на периметре строки пикселей горизонта
static int numstar[STARVAR]={0, 1, 4,10,10};
//макс. яркость звезды [номер звезды на строке], 0...15
static int maxstar[MAXNUMSTAR]={15, 8, 8, 8, 6, 6, 6, 6, 6, 6};
//мин.  яркость --//--, 0...15, но не более maxstar[] !
static int minstar[MAXNUMSTAR]={ 8, 5, 5, 5, 3, 3, 3, 3, 3, 3};
//табличная яркость звезды [номер строки][номер звезды на ней]
static byte far powstar[MAXVLEN][MAXNUMSTAR];//2кб !
//псевдоугловой сдвиг звезды -//- от 0 градусов, в пикселях строки
static int  far angstar[MAXVLEN][MAXNUMSTAR];//4 кб !!
static int oldval_h,oldnumst,oldvlen,oldjear;
int n,i,j,jj,col,hlen,parthlen,vlen,len360;
int colst,angst,numst,ampst,powst;
word adr;

if (starvar<=0) return;

vlen=penbmplenz;
hlen=penbmplenx;
parthlen=partpenbmplenx;
numst=numstar[starvar];
if (numst>MAXNUMSTAR) numst=MAXNUMSTAR;

//дополнительная длина сдвига звёзд до оси 360 градусов
len360=horshift360-horshift;

//надо пересчитать координаты звезд если сменились пар-ры:
if ((oldval_h!=val_h)||(oldnumst!=numst)||(oldvlen!=vlen)||(oldjear!=jear))
  {
  oldval_h=val_h;
  oldnumst=numst;
  oldvlen=vlen;
  oldjear=jear;

  //вычисляем и запоминаем горизонт-е 'углы' и яркость звезд
  for (j=0; j<MAXVLEN; ++j)//цикл по вертикали макс размера
    {
    //фиксация случайности в 0 градусов каждого уровня j,
    //с учетом текущего вертикального масштаба!
    Srand2(777*(j-(int)((jear*(long)MAXVLEN)/vlen))+123);
    //это для меньшей корреляции координат при малом числе звезд
    Srand2(Rand2());//и не только это...
    //генерация звезд на всем 360-градусном диапазоне уровня
    for (n=0; n<numst; ++n)
      {
      //гориз-й сдвиг звезды относительно оси зрения 0 градусов
      angst=abs((Rand2()-Rand2()/7-Rand2()/33))%MAXHORSHIFT;
      //сдвиг с учетом текущего масштаба по горизонтали
      angst=(int)((angst*(long)horshift360)/MAXHORSHIFT);
      //яркость звезды в пределах min...max
      powst=minstar[n]+Rand2()%(maxstar[n]-minstar[n]+1);
      //запоминаем звезду в таблицы
      angstar[j][n]=angst;
      powstar[j][n]=powst;
      }
    }
  }

//поверх фона добавляем звезды
ampst=ampstar[starvar];
for (jj=0; jj<MAXVLEN; ++jj)//верт. развертка полной длины!
  {
  //уменьшенное j, с учетом вертикального масштаба
  j=(int)((jj*(word)vlen)/MAXVLEN);
  //строка горизонта и ниже заполняется только в космосе
  if (j<=jear) if (spaceflag==OFF) continue;
  //генерация звезд на _всем_ 360-градусном диапазоне уровня j
  for (n=0; n<numst; ++n)
    {
    angst=angstar[jj][n];//здесь jj !
    powst=powstar[jj][n];
    adr=(word)(-1);//псевдоадрес предварительно невидимой звезды!
    //определяем видимость звезды и ее положение в PEN-bmp
    if (numpart==1)//одношаговый рендер
      {
      if (len360<hlen)//len360 не хватает до конца экрана
        {
        //кусок фона от horshift до horshift360-1 частичной длиной len360
        i=angst-horshift;//сдвиг с учетом поворота оси зрения наблюдателя
        //номер adr байта памяти битмапа фона penram[adr] звезды,
          //попадающей на видимый участок уровня
        if ( (0<=i) && (i<len360) )        adr=hlen*j+i;
        //кусок фона начиная с horshift360 и далее до конца экрана
        i=angst;
        if ( (0<=i) && (i<(hlen-len360)) ) adr=hlen*j+len360+i;
        }
      else//len360=>hlen
        //кусок фона от horshift до <horshift360 полной длиной hlen
        {
        i=angst-horshift;
        if ( (0<=i) && (i<hlen) )          adr=hlen*j+i;
        }
      }
    else//==2, двухшаговый рендер
      {
      if (len360<hlen)//len360 не хватает до конца экрана
        {
        i=angst-horshift;
        if (actpart==0)//левый полукадр
          { if ((0<=i)&&(i<len360)&&(i<parthlen)) adr=parthlen*j+i; }
        else//правый
          { if ((parthlen<=i)&&(i<len360)) adr=parthlen*j+i-parthlen; }
        i=angst;
        if (actpart==0)//левый полукадр
          { if ((0<=i)&&(i<(parthlen-len360))) adr=parthlen*j+len360+i; }
        else//правый
          { if (((parthlen-len360)<=i)&&(i<(hlen-len360)))
              adr=parthlen*j+i-(parthlen-len360); }
        }
      else//len360=>hlen
        {
        i=angst-horshift;
        if (actpart==0)//левый полукадр
          { if ((0<=i)&&(i<parthlen)) adr=parthlen*j+i; }
        else//правый
          { if ((parthlen<=i)&&(i<hlen)) adr=parthlen*j+i-parthlen; }
        }
      }

    //проверка попадания звезды на видимый на экране участок
    if (adr!=(word)(-1))
      {
      //флуктуации яркости звезды (powst-ampst)...(powst)
      powst=powst-(Randi()%(ampst+1));
      //самые слабые звезды вообще пропускаем
      if (powst<=0) continue;
      //соответствующий интенсивности bw-цвет звезды
      colst=gray[powst];
      //цвет точки фона под звездой
      col=penram[adr];
      //если звезда ярче фона, она высвечивается
      if (powercol(colst)>powercol(col)) penram[adr]=colst;
      }
    }
  }
}

int _fastcall powercol(int color)
//возвращает яркость цвета color = его r+g+b
{
int c3;
c3=3*color;
return (syspal[c3]+syspal[c3+1]+syspal[c3+2]);
}

void SetPalette(byte far *rgb,int startcol,int endcol)
//порт: установка палитры (LUT бэкенда вместо DAC-портов VGA)
{
VideoSetPalette(rgb,startcol,endcol);
}

void GetPalette(byte far *rgb,int startcol,int endcol)
//порт: чтение палитры (возврат последней установленной)
{
VideoGetPalette(rgb,startcol,endcol);
}

void WriteScreen(void)
//запись экрана в pp-файл!
//в файле 64000+768+9+5=64782 байта:
//текстура и палитра,
//прозрачный цвет            '\r\n\r\nTCL\r\n'
//и (максимальная) цветность 'CLN\r\n'
{
static int fnum;//номер файла
byte pal[PAL768];
char file[32];
FILE *stream;
int hnd;
//имя текущего файла
sprintf(file,"%i.pp",fnum++);
//пишем экран на диск
_fmode=O_BINARY;
hnd=creat(file,S_IREAD|S_IWRITE);
Write(hnd,memory[VGAMEM].rampntr,PPTEXLEN);
//пишем палитру
GetPalette(pal,0,255);//читаем физпалитру из VGA !!!
Write(hnd,pal,PAL768);
//пишем прозрачный цвет и цветность
stream=fdopen(hnd,"w");
fprintf(stream,"\r\n\r\n -1\r\n  1\r\n");
//закрываем файл
fclose(stream);
}

void ColorArraySetup(void)
//рассчет массивов цветов и их указателей
{
int *arrcol[14];//массив указателей на массивы цветов
int i,j;

arrcol[0]=gray;    arrcol[1]=gray1;
arrcol[2]=red;     arrcol[3]=red1;
arrcol[4]=redgrn;  arrcol[5]=redgrn1;
arrcol[6]=grn;     arrcol[7]=grn1;
arrcol[8]=grnblu;  arrcol[9]=grnblu1;
arrcol[10]=blu;    arrcol[11]=blu1;
arrcol[12]=redblu; arrcol[13]=redblu1;

for (j=0; j<14; ++j)
  {
  for (i=0; i<16; ++i)
    arrcol[j][i]=hexcol(arrcol[j][i]);
  //теперь набираем радугу
  for (i=RADMIN; i<=RADMAX; ++i)
    {
    raduga[j][i-RADMIN]= ((j&1)==0)?
      arrcol[j][i]:arrcol[j][RADMAX+RADMIN-i];
    }
  }
}

void PaintGranBitmap(int bm,int glen,int mode,int col)
//одноцветная рамка толщиной granlen пикселей согласно битовой mode:
//единичные биты 0,1,2,3 означают наличие рамки у соответствующей
//линии края битмапа: прав,верх,лев,низ !
{
int lenx,lenz,b0,b1,b2,b3;

b0=mode&1;
b1=mode&2;
b2=mode&4;
b3=mode&8;
lenx=memory[bm].lenx;
lenz=memory[bm].lenz;
if (b0!=0) PaintBitmap(bm,lenx-glen,0,lenx-1,lenz-1,col);//справа
if (b1!=0) PaintBitmap(bm,0,lenz-glen,lenx-1,lenz-1,col);//сверху
if (b2!=0) PaintBitmap(bm,0,0,glen-1,lenz-1,col);//слева
if (b3!=0) PaintBitmap(bm,0,0,lenx-1,glen-1,col);//снизу
}

void Fractal(int far *arr1025,int dclevel,int *harmonics,int seed)
//заполнение массива arr1025[0...1024] случайными данными по
//~фрактальному алгоритму с уровнем постоянного тока dclevel
//и min/max уровнями 10 гармоник в массиве harmonics[0...19]
//seed - затравка для случайности!
{
int i,j,lenotr,numotr,a,b,ia,harmax,harmin;
long deltah;

Srandu(seed);
numotr=1;
arr1025[0]=arr1025[1024]=dclevel;
for (i=0; i<10; ++i)
  {
  lenotr=1024/numotr;
  harmin=harmonics[2*i];
  harmax=harmonics[2*i+1];//при max<min:
  deltah=harmax-harmin;//может быть отрицательным !!!
  for (j=0; j<numotr; ++j)
    {
    //a,b=значения на концах отрезка
    ia=lenotr*j;
    a=arr1025[ia];
    b=arr1025[ia+lenotr];
    //уровень среднего плюс случайная гармоника i от harmin до harmax
    //в срединную точку
    arr1025[ia+lenotr/2]=(a+b)/2+harmin+(int)((deltah*Randu())/MAXINT);
    }
  numotr*=2;
  }

//обнуляем отрицательные значения !!!
for (i=0; i<=1024; ++i) if (arr1025[i]<0) arr1025[i]=0;
}

#define MAXSORT  20// MAXSORT = log(2^20) >= log(max(MAXPAN,MAXMEM)) !!!
void QuickSort(int maxind,long *srtkey,int *srtind)
//сортируем ключевой массив srtkey[i] по возрастанию ключей и определяем
//номера исходных индексных позицый srtind[i], 0 <= i < maxind <=MAXSORT
//см книгу Н.Вирт, Алгоритмы и структуры данных, стр. 119 и стр. 121 !!!
{
int sR[MAXSORT],sL[MAXSORT];//это стек L-R границ просмотра;
int i,j,L,R,s;
long x,w;
int ii;

//сначала запрос на просмотр всего массива ключей
s=0; sL[s]=0; sR[s]=maxind-1;
do{
  //выбор из стека последнего запроса
  L=sL[s]; R=sR[s]; --s;
  do{
    //разделение srtkey[L]...srtkey[R]
    i=L; j=R; x=srtkey[(L+R)/2];
    do{
      while (srtkey[i]<x) ++i;
      while (x<srtkey[j]) --j;
      if (i<=j)//перестановка ключей и индексов
        {
        w =srtkey[i]; srtkey[i]=srtkey[j]; srtkey[j]=w;
        ii=srtind[i]; srtind[i]=srtind[j]; srtind[j]=ii;
        ++i; --j;
        }
      }
    while (i<=j);

/*старая версия
    if (i<R)//запись в стек запроса из правой части для будущей отработки
      {
      ++s; sL[s]=i; sR[s]=R;
      }
    R=j;//теперь L и R ограничивают левую часть, которая и отрабатывается
*/
//  новая версия
    if ((j-L)<(R-i))
      {
      if (i<R)//запись в стек запроса на сортировку правой части
        {
        if (++s>=MAXSORT) return;//ERR
        sL[s]=i; sR[s]=R;
        }
      R=j;//продолжение сортировки левой части
      }
    else
      {
      if (L<j)//запись в стек запроса на сортировку левой части
        {
        if (++s>=MAXSORT) return;//ERR
        sL[s]=L; sR[s]=j;
        }
      L=i;//продолжение сортировки правой части
      }

    }
  while (L<R);
  }
while (s>=0);
}

int _fastcall hexcol(int rgb)
//выдает цвет syspal по 3-х разрядному гекс-числу h=0xRGB
{
return nnn[0xf&(rgb>>8)][0xf&(rgb>>4)][0xf&rgb];
}

void SyspalSetup(void)
//расчет syspal и запись на диск
{
int hnd;
ComputSyspal();
_fmode=O_BINARY;
hnd=creat("syspal.lii",S_IREAD|S_IWRITE);
Write(hnd,syspal,sizeof(syspal));
Write(hnd,nnn,sizeof(nnn));
close(hnd);
}

void PaintZbitm(void)
//[пере]закраска битмапа земли
{
int i,j,col,r,g,b,c3,rr,gg,bb,dr,dg,db,step,power;

if (newbgpar==NO) return;
newbgpar=NO;

//новые цвета земли

//корректировка зашкала параметров
for (i=0; i<=2; ++i)
 {
 if (bgpar[bgvar][i]<0)  bgpar[bgvar][i]=0;
 if (bgpar[bgvar][i]>63) bgpar[bgvar][i]=63;
 }
bgpar[bgvar][3]&=3;
bgpar[bgvar][4]&=3;
bgpar[bgvar][5]&=3;
if (bgpar[bgvar][6]<0) bgpar[bgvar][6]=0;
if (bgpar[bgvar][6]>8) bgpar[bgvar][6]=8;
if (bgpar[bgvar][7]<0)  bgpar[bgvar][7]=0;
if (bgpar[bgvar][7]>64) bgpar[bgvar][7]=64;

//распаковка параметров
r=bgpar[bgvar][0];
g=bgpar[bgvar][1];
b=bgpar[bgvar][2];
dr=bgpar[bgvar][3];
dg=bgpar[bgvar][4];
db=bgpar[bgvar][5];
step=bgpar[bgvar][6];
power=bgpar[bgvar][7];

rr=(r*power)>>6;
gg=(g*power)>>6;
bb=(b*power)>>6;

for (i=1; i<=8; ++i)//палитра цветов земли в syspal
  {
  if (rr>63) rr=63;
  if (gg>63) gg=63;
  if (bb>63) bb=63;

  c3=3*i;
  syspal[c3]=rr;
  syspal[c3+1]=gg;
  syspal[c3+2]=bb;

  //копируем градации яркости из наиболее близкого цвета syspal
  col=Color(rr,gg,bb);
  for (j=0; j<MAXGRADCOL; ++j) gradcol[j][i]=gradcol[j][col];
  //уточняем цвет номинала 100%
  gradcol[GRADCOLNOM][i]=i;

  rr+=dr*step;
  gg+=dg*step;
  bb+=db*step;
  }

//переустанавливаем физическую палитру
SetPalette(syspal,1,8);//только цвета 1...8 !

for (i=0; i<16; ++i)//конверт земли
  {
  //треугольник цветов: 0123456776543210
  if (i<8) col=i;
  else     col=15-i;
  zbitm[i]=col+1;//col=1+(0...7)=несистемные цвета 1...8 !
  }
}

#define IMUL 4
void pangran(int pan,int glen,int mode,int *gcol,int i0,int i1,int istep)
//старый вариант с целым шагом...
{
int ii1,ii2;
istep=abs(istep)*IMUL;
if (i0<=i1) { ii1=i0; ii2=i1; }
else        { ii1=i1; ii2=i0; istep=-istep; }
pangran2(pan,glen,mode,gcol,i0,ii1,ii2,istep);
}
void pangran2(int pan,int len,int mode,int *col,int i0,int i1,int i2,int st4)
//рамка битмапа панели pan толщиной len пикселей и цветами снаружи
//внутрь, цвета берутся из набора col[индекс], индекс меняется с
//дробным шагом (st4)/IMUL сначала i0->i1 (или i2), затем циклически
//(i2->)i1->i2->i1 насколько позволяет длина len, причем i1<=i0<=i2 !
//параметр mode см. PaintGranBitmap() !
{
int lenx,lenz,bm,x0,x1,z0,z1,maxlen;
int i,ii1,ii2,n,c,b0,b1,b2,b3;

if (len<=0) return;

b0=mode&1;
b1=mode&2;
b2=mode&4;
b3=mode&8;
bm=panel[pan].memtex;
lenx=memory[bm].lenx;
lenz=memory[bm].lenz;
maxlen=max(lenx,lenz);
if (len>maxlen) len=maxlen;

if (st4==0)//постоянный цвет, рамка прямоугольная
  {
  PaintGranBitmap(bm,len,mode,col[i0]);
  return;
  }
//else границы индексов не совпадают и шаг индекса не нулевой

//цвет рамки плавно меняется

i=IMUL*i0;
ii1=IMUL*i1;
ii2=IMUL*i2;
for (n=0; n<len; ++n)//цикл по границе снаружи внутрь
  {
  //очередной цвет
  c=col[i/IMUL];
  //углы очередного периметра
  x0=z0=n;
  x1=lenx-x0-1;
  z1=lenz-z0-1;

  //красим границы по периметру от диагонали до диагонали:
  //справа
  if (b0!=0) PaintBitmap(bm,x1,z0,x1,z1,c);
  //сверху
  if (b1!=0) PaintBitmap(bm,x0,z1,x1,z1,c);
  //слева
  if (b2!=0) PaintBitmap(bm,x0,z0,x0,z1,c);
  //снизу
  if (b3!=0) PaintBitmap(bm,x0,z0,x1,z0,c);

  //меняем индекс цвета
  i+=st4;
  if (i>ii2) { i=ii2; st4=-st4; }
  if (i<ii1) { i=ii1; st4=-st4; }
  }
}

void LilaPaint(void)
//рисуем лейбл Lila в правом верхнем углу дисплея
//ASM дает ускорение не более 10 % ... - поэтому
//оставлен вариант на СИ ...
{
byte far  *lram;
byte near *pram;
int j,i,i0,col;

if (numpart==1) return;
//else полный экран (2-шаговый рендер)

//рисуем лейбл
i0=LILAX-1;
for (j=-6; j<LILAZ-6; ++j)
  {
  pram=penram+160*(j+200-LILAZ)+153;
  lram=lila+LILAX*(LILAZ-j-7);
  for (i=i0; i>=0; --i)
    {
    col=lram[i];
    if (col!=0) *pram=col;
    --pram;
    }
  }
}

void RenderZ(void)
//рендер земли
{
static fix EDYuser;//накопитель линейных смещений юзера
long DC,C2,C1,C0,YJ;
int j,yj,col,jz;
fix Zuser;
word edy;

//формулы см К-I, стр 78 !

//только 1 раз на весь рендер накапливаем смещения юзера
if (actpart==0) EDYuser+=DYuser;
edy=(word)fixtoi(EDYuser);

Zuser=PEN.ear.z0;//положение юзера по высоте
C0=fixtoi(fixmul(Zuser,penbmpfmulz1));
for (j=0; j<penbmplenz; ++j)
  {
  if (Zuser>=0)//если юзер не под землёй
    //красим землю, исключая уровень горизонта!
    {
    if (j>=jear) continue;//тут небо
    //else тут земля
    jz=jear-j;//абсол-й сдвиг строки вниз от горизонта

    C1=(val_h*C0)/jz;
    C2=(val_h*C0)/(jz+1);//это C1 строкой ниже
    DC=labs(C1-C2);

    YJ=(C1+edy)>>8;
    yj=(int)(YJ&15);
    //если разность удалений земли на соседних строках экрана
    //даёт изменение индекса цвета земли больше (четвёрки<<8):
    if (DC>1024)
      //плавно уменьшаем диапазон колебаний индекса цвета !
      yj=(int)((yj*1024)/DC);
    }
  else//под землёй - земля выше горизонта !
    {
    if (j>=jearuser) continue;//тут ещё небо
    YJ=edy>>8;
    yj=(int)(YJ&15);
    }

  col=zbitm[yj];//цвет из битмапа земли
  SetRAM(penram+partpenbmplenx*j,partpenbmplenx,col,1);
  }
}

void RenderNebo(void)
//рендер одноцветного фонового неба
{
int j0,j1,col;

//строки неба на горизонте и выше до конца экрана: j0...j1
j0=jear;
if (j0<0) j0=0;
//а если космос - то строки с самого низа
if (spaceflag==ON) j0=0;
j1=penbmplenz-1;
if (j0>j1) return;
col=nebocol[nebocoli];
SetRAM(penram+partpenbmplenx*j0,partpenbmplenx*(j1-j0+1),col,1);
}

void MirrorBMAP(int bm,int needx,int needz)
//отражение текстуры битмапа от срединной линии по x и/или по z
{
int lenx,lenz,x,z,lx2,lz2;
byte far *tram;
byte far *sram;
byte far *dram;

if (bm<=ISNOT) return;

lenx=memory[bm].lenx;
lenz=memory[bm].lenz;
lx2=lenx/2;
lz2=lenz/2;

tram=memory[bm].rampntr;
if (memory[bm].rammode!=2) ConvertMEMmode(bm,2);
//мода битмапа 2 !

if (needx!=YES) goto mirrz;

//отражение по x

for (x=0; x<lx2; ++x)
  {
  sram=tram+x*lenz;
  dram=tram+(lenx-x-1)*lenz;
  for (z=0; z<lenz; ++z) *dram++=*sram++;
  }

mirrz: if (needz!=YES) return;

//отражение по z

for (z=0; z<lz2; ++z)
  {
  sram=tram+z;
  dram=tram+lenz-z-1;
  for (x=0; x<lenx; ++x)
    {
    *dram=*sram;
    dram+=lenz;
    sram+=lenz;
    }
  }
}

void NeboColor(int dr,int dg,int db,int dpow,int dnum)
//всевозможные регулировки цвета неба:
//изменения r/g/b/яркости или абсолютного номера цвета!
{
static int ri,gi,bi=15,pow=16;
int r,g,b,col;

col=nebocol[nebocoli];
if (dnum!=0)//это приоритетный параметр!
  {
  col+=dnum;
  col=max(col,0);
  col=min(col,255);
  goto end;
  }

pow+=dpow;
pow=max(pow,0);
pow=min(pow,16);

ri+=dr;
ri=max(ri,0);
ri=min(ri,15);

gi+=dg;
gi=max(gi,0);
gi=min(gi,15);

bi+=db;
bi=max(bi,0);
bi=min(bi,15);

r=(ri*pow)/16;
g=(gi*pow)/16;
b=(bi*pow)/16;
col=color(r,g,b);
end:
nebocol[nebocoli]=col;
}

void RPBpixel(int bm,int x0,int z0,int x1,int z1,int *col,int i0,int i1)
//закраска прямоугольного куска (x0,z0)...(x1,z1) битмапа bm
//одиночными пикселями со случайными цветами col[i0...i1]
// RPB=RandPaintBitmap !
{
int x,z,lenx,lenz,maxx,maxz,dind1;
static long seed;
byte far *ram;

if ((bm<=ISNOT)||(x0>x1)||(z0>z1)) return;
lenx=memory[bm].lenx; maxx=lenx-1;
lenz=memory[bm].lenz; maxz=lenz-1;
if ((x0>maxx)||(x1<0)) return;
if ((z0>maxz)||(z1<0)) return;
x0=max(x0,0); x1=min(x1,maxx);
z0=max(z0,0); z1=min(z1,maxz);

dind1=i1-i0+1;
if (dind1<1) dind1=1;

if (memory[bm].rammode!=2) ConvertMEMmode(bm,2);
//мода битмапа 2 !
ram=memory[bm].rampntr+x0*lenz;
for (x=x0; x<=x1; ++x)
  {
  for (z=z0; z<=z1; ++z)
    {
    //встроенный случайный датчик для скорости!
    seed=RANDUMUL*seed+RANDUADD;
    ram[z]=col[i0+((int)(0x7FFF&(seed>>16)))%dind1];
    }
  ram+=lenz;
  }
}

void RPBline(int bm,int x0,int z0,int x1,int z1,
             int grad,int len0,int len1,int *col,int i0,int i1)
//красит кусок битмапа линиями с углом grad (пока только 0
//и 90 градусов) и случайными:
//длиной в диапазоне len0...len1 и цветом col[i0...i1]
{
int maxx,maxz,lenx,lenz,x,xs,xe,z,zs,ze,rlen,rcol,dind1,dlen1;
byte far *tram;
byte far *ram;

if ((bm<=ISNOT)||(len0<=0)||(len1<=0)) return;

if ((x0>x1)||(z0>z1)) return;
lenx=memory[bm].lenx; maxx=lenx-1;
lenz=memory[bm].lenz; maxz=lenz-1;
if ((x0>maxx)||(x1<0)) return;
if ((z0>maxz)||(z1<0)) return;
x0=max(x0,0); x1=min(x1,maxx);
z0=max(z0,0); z1=min(z1,maxz);

dind1=i1-i0+1;
if (dind1<1) dind1=1;
dlen1=len1-len0+1;
if (dlen1<1) dlen1=1;

tram=memory[bm].rampntr;
if (memory[bm].rammode!=2) ConvertMEMmode(bm,2);
//мода битмапа 2 !

if (grad==0)//горизонтальные линии
  {
  for (z=z0; z<=z1; ++z)
    {
    xs=x0;
    rlen=Randu()%len1;//полная случайность стартовой rlen !
    while (xs<=x1)
      {
      xe=xs+rlen;
      if (xe>x1) xe=x1;
      rcol=col[i0+Randu()%dind1];
      ram=tram+xs*lenz+z;
      for (x=xs; x<=xe; ++x)
        {
        *ram=rcol;
        ram+=lenz;
        }
      rlen=len0+Randu()%dlen1;//rlen в диапазоне len0...len1
      xs=xe+1;
      }
    }
  }
else//90 градусов - вертикальные линии
  {
  for (x=x0; x<=x1; ++x)
    {
    zs=z0;
    rlen=Randu()%len1;//полная случайность стартовой rlen !
    while (zs<=z1)
      {
      ze=zs+rlen;
      if (ze>z1) ze=z1;
      rcol=col[i0+Randu()%dind1];
      ram=tram+x*lenz+zs;
      for (z=zs; z<=ze; ++z) *ram++=rcol;
      rlen=len0+Randu()%dlen1;//rlen в диапазоне len0...len1
      zs=ze+1;
      }
    }
  }
}

void ZeroPANhead(int n)
{
panel[n].flag=NO;
panel[n].sides=SIDES;
panel[n].render=NO;
panel[n].owner=ADDR_NOBODY;
panel[n].count=0;
panel[n].ap=d3null;
panel[n].earvalid=NO;//т.о. ear,
panel[n].penvalid=NO;//pen и
panel[n].xbevalid=NO;//xbe можно не обнулять!
panel[n].flenx=1;
panel[n].flenz=1;
panel[n].memtex =ISNOT;
panel[n].memconv=ISNOT;
panel[n].fmulx=FIX1;
panel[n].fmulz=FIX1;
panel[n].xb0=0;
panel[n].xb1=0;
panel[n].zb0=0;
panel[n].zb1=0;
panel[n].gradcon=0;
panel[n].glass=NO;
panel[n].rfluct=0;//стандартно нет флуктуаций пикселей !
panel[n].rendlenx=1;//не ноль !!!
panel[n].scrollx=0;
panel[n].gradlenx=1;
panel[n].gradampx=0;
panel[n].heomode=FULPAN;//панель есть во всех 4-х квадрантах!
panel[n].graddin=0;
}

void ComputSyspal(void)
//рассчет syspal и массива преобразования цветов nnn
{
int r,g,b,r1,g1,b1;
int i,j,k,col,c3;
fix fcw;

//сжимаем цветность rgb-пространства 64x64x64 умножением на FIXC
//и ставим в nnn сначала метку наличия цвета в rgb-кубике 4*4*4
for (r=0; r<64; ++r)
  for (g=0; g<64; ++g)
    for (b=0; b<64; ++b)
      {
      //меняем цвет r,g,b -> r1,g1,b1
      fcw=FIXC1*(r+g+b);
      //уменьшение цветности:       r1,g1,b1 всегда больше 0 !
      r1=pfixtoi(FIXC*r+fcw);//поэтому pfixtoi вместо fixtoi !
      g1=pfixtoi(FIXC*g+fcw);
      b1=pfixtoi(FIXC*b+fcw);
      //шаг дискретизации компонент цвета=4
      nnn[r1>>2][g1>>2][b1>>2]=1;//ставим метку
      }

//вытаскиваем из массива меток цвета в массив syspal и делаем
//из первого (nnn) массив преобразования любых цветов в syspal
col=8;//'предыдущий' номер цвета
for (i=0; i<16; ++i)
  for (j=0; j<16; ++j)
    for (k=0; k<16; ++k)
      {
      //проверяем метку наличия цвета в массиве nnn
      if (nnn[i][j][k]>0)//метка есть!
        {
        if (++col>=256) continue;//лишние цвета:
        //это возможно при ошибочных модификациях в будущем!
        c3=col*3;
        //вытаскиваем цвет в палитру
        //шаг дискретизации компонент цвета=4
        syspal[c3]  =(i<<2)+3;//+3 для яркости и цветности!
        syspal[c3+1]=(j<<2)+3;
        syspal[c3+2]=(k<<2)+3;
        //из метки делаем номер цвета!
        nnn[i][j][k]=col;
        }
      }
//контроль: ка было проверено, col=255 !
//printf("MAX color=%i !\n",col); WKP();
}

void ConvertPalette(byte far *spal,byte far *dpal,fix colness,int startcol)
//преобразование палитры palsrc->paldst (только для цветов
//с номерами от startcol до 255), с цветностью colness;
{
int scol3,r,g,b,r1,g1,b1,i;
fix fc,fc1,fcw;
long w,w1;

//вычисляем коэф-ты преобразования, см Идеи IV, 5 февр. 96 г.
fc=colness;
fc1=(FIX1-fc)/3;

//цикл корректировки 256 цветов src палитры
scol3=3*startcol;
for (i=0; i<768; i+=3)
  {
  //интенсивности компонент цвета col=i/3 src палитры
  r=spal[i];
  g=spal[i+1];
  b=spal[i+2];

  if (i<scol3) { r1=r; g1=g; b1=b; }
  else//меняем цветность r,g,b -> r1,g1,b1
    {
    w=r+g+b;
    fcw=fc1*w;

    r1=fixtoi(fc*r+fcw);
    g1=fixtoi(fc*g+fcw);
    b1=fixtoi(fc*b+fcw);
    if (r1<0)  r1=0; if (r1>63) r1=63;
    if (g1<0)  g1=0; if (g1>63) g1=63;
    if (b1<0)  b1=0; if (b1>63) b1=63;

    w1=r1+g1+b1;
    //корректируем яркость результата под исходную яркость,
    if (w1!=w)//если они _не совпадают_ !
      {
      if (w1!=0)
        {
        r1=(int)((r1*w)/w1);
        g1=(int)((g1*w)/w1);
        b1=(int)((b1*w)/w1);
        if (r1>63) r1=63;
        if (g1>63) g1=63;
        if (b1>63) b1=63;
        }
      else//w1==0
      if (w!=0) r1=g1=b1=(int)(w/3);
      }
    }
  //результат в dst палитру
  dpal[i]=r1;
  dpal[i+1]=g1;
  dpal[i+2]=b1;
  }
}

int _fastcall color(int r16,int g16,int b16)
//цвета с 4-бит. компонентами rgb, в шкале интенсивностей 0...15 !
{
return Color((r16<<2)+3,(g16<<2)+3,(b16<<2)+3);
//                   +3 для яркости и цветности!
}

int _fastcall Color(int r,int g,int b)
//возвращает цвет в syspal, примерно соответствующий 18-бит
//цвету с компонентами r,g,b в шкале интенсивностей 0...63
{
int r1,g1,b1;
fix fcw;

fcw=FIXC1*(r+g+b);
//уменьшение цветности:       r1,g1,b1 всегда больше 0 !
r1=pfixtoi(FIXC*r+fcw);//поэтому pfixtoi вместо fixtoi !
g1=pfixtoi(FIXC*g+fcw);
b1=pfixtoi(FIXC*b+fcw);
//номер цвета syspal, близкий по rgb копонентам
return nnn[r1>>2][g1>>2][b1>>2];
}

void ComputGradcol(void)
//рассчет таблицы градация яркости цветов gradcol[][col]
{
int pow,i,col,c3,r,g,b,r1,g1,b1;
byte pal[PAL768];
//цветность syspal была уменьшена в 0.133=8716/65536 раз, а теперь
//нужна обратная коррекция цветности до исходного 100% значения с
//colorness = (65536/8716)<<16 = 492752 :
ConvertPalette(syspal,pal,492752L,0);

//цикл по всем фиксированным цветам syspal
for (col=9; col<256; ++col)
  {
  c3=col*3;
  //интенсивности компонент цвета
  r=pal[c3];
  g=pal[c3+1];
  b=pal[c3+2];

  //цикл изменения яркости компонент
  for (i=0; i<=MAXGRADCOL1; ++i)
    {
    pow=MAXPOWER-((MAXPOWER-MINPOWER)*i)/MAXGRADCOL1;

    r1=(r*pow)/100; if (r1>63) r1=63;
    g1=(g*pow)/100; if (g1>63) g1=63;
    b1=(b*pow)/100; if (b1>63) b1=63;

    //соответствующий номер цвета syspal
    gradcol[i][col]=Color(r1,g1,b1);
    }
  }

//все цвета слоя gradcol[GRADCOLNOM] принудительно делаем
//совпадающими, иначе есть небольшие отличия
for (i=0; i<256; ++i) gradcol[GRADCOLNOM][i]=i;
}

void Set4Pan(int pan,d3poz poz,int centrdy,int centrdz)
//на расстоянии centrdy,centrdz см от poz с 4-х сторон
//устанавливается центр панели pan и её трёх копий !
{
int i,pani,owner;
d3poz centr,shift;
angle pangx;
if (pan<=ISNOT) return;
owner=panel[pan].owner;
//делаем и ставим панели
pangx=poz.angx;
for (i=0; i<4; ++i)
  {
  if (i==0) pani=pan;
  else
    {
    pani=GetPAN(owner);
    if (pani<=ISNOT) return;
    CopyPAN(pani,pan);
    LockPAN(pani);
    }
  poz.angx=pangx+ANG90G*i;
  shift=XYZgrad2D3poz(0,-centrdy,centrdz,0);
  centr=ShiftD3poz(shift,poz);
  SetPanelCentr(pani,centr);
  }
}

void ReadNextPPfile(int napr)
//загрузка pp-файла в битмап стены:
//при napr>0 загрузка следующего файла, при <0 предыдущего файла;
{
static struct ffblk fbuf;
static int start=YES,oldnapr=0;
static char oldfile[16];
char file[16];
int findok;

if (oldnapr<0) napr=0;

if (start==YES) findok= (findfirst("pp\\*.pp",&fbuf,0)==0)? YES:NO;
else
if (napr>0)
  {
  strcpy(oldfile,"pp\\");
  strcat(oldfile,fbuf.ff_name);
  if (findnext(&fbuf)==0) findok=YES;
  else findok= (findfirst("pp\\*.pp",&fbuf,0)==0)? YES:NO;
  }

strcpy(file,"pp\\");
strcat(file,fbuf.ff_name);
if ((napr<0)&&(start==NO)) strcpy(file,oldfile);

if (findok==NO) { start=YES; return; }
oldnapr=napr;
start=NO;
LoadPPfile(wallbitm,file,0,FIX2);
}

int ReadLLfile(fix flenx,fix flenz,int indplus)
//читает текущий ll-файл и делает лабиринт с параметрами
// flenx, flenz, indplus - см описание лабиринта !
//при ошибке возвращает ERR
{
d3poz shift;//сдвиг оригина лабиринта
int tmem,lsize,res;
byte far *lbuf;

//выделяем временный буфер размером с файл
lsize= (llfilesize<MAXLABLEN)? (int)llfilesize:MAXLABLEN;
tmem=GetLockMEM(ADDR_VRS,lsize);
if (tmem<=ISNOT) return ERR;

lbuf=memory[tmem].rampntr;
//сдвиг влево на 160 см, вперед на 5 м, вниз на рост юзера:
//это удобно для большинства ll-файлов !
shift=XYZgrad2D3poz(-160,500,-USERZ0,0);

//грузим лабиринт
res=OpenRead(llfile,0,lbuf,lsize);
if (res==OK) Labirint(ADDR_Labir,shift,flenx,flenz,lbuf,lsize,indplus);
UnLockMEM(tmem);
return res;
}

int FindLLfile(int deltafile)
//определение имени и размера ll-файла:
//при deltafile=0  текущего файла,
//при deltafile=1  следующего файла,
//(при deltafile=2  следующего файла+1, и т.д.)
//при deltafile=-1 предыдущего файла (и т.д...)
//возвращает ERR если файл не найден!
{
static struct ffblk fbuf;
static int actf=0;//номер текущего файла в списке ll-файлов директории
static int numf=0;//количество ll-файлов в директории
static int start=YES;
int i,findok;

if (start==YES)//определяем количество ll-файлов
  {
  start=NO;
  findok= (findfirst("ll\\*.ll",&fbuf,0)==0)? YES:NO;
  while(findok==YES)
    {
    ++numf;
    findok= (findnext(&fbuf)==0)? YES:NO;
    }
  //else numf=0
  }

if ((start==NO)&&(numf<=0)) return ERR;

//номер нужного файла в списке
actf+=deltafile;
if (actf<0) actf=numf-1;
actf%=numf;

for (i=0; i<=actf; ++i)//перебираем файлы до нужного
  {
  if (i==0) findok= (findfirst("ll\\*.ll",&fbuf,0)==0)? YES:NO;
  else      findok= (findnext(&fbuf)==0)?               YES:NO;
  //на всякий случай проверка:
  if (findok==NO) { start=YES; return ERR; }
  }

strcpy(llfile,"ll\\");
strcat(llfile,fbuf.ff_name);
llfilesize=fbuf.ff_fsize;
return OK;
}

int DelLab(void)
//уничтожение временного лабиринта, даёт ERR если нет его!
{
int n,res;
res=ERR;
for (n=0; n<=maxpannum; ++n)
  if (panel[n].owner==ADDR_Labir) { UnLockPAN(n); res=OK; }
return res;
}

int Lab2world(void)
//оставление временного лабиринта AP_First в мире напостоянку,
//даёт ERR если нет его!
{
int n,res;
res=ERR;
for (n=0; n<=maxpannum; ++n)
  if (panel[n].owner==ADDR_Labir)
    {
    panel[n].owner=ADDR_APFirst;
    //сдвиг панели от оригина AP_First
    panel[n].ap=DeltaD3poz(panel[n].ear,AP[ADDR_APFirst].ear);
    panel[n].earvalid=NO;
    res=OK;
    }
return res;
}

void NewPanelsPars(void)
//меняем нужные параметры панелей 1 раз за рендер
{
int rendlenx,sx,pan;
if (scrollflag!=ON) return;
for (pan=0; pan<=maxpannum; ++pan)
  {
  sx=panel[pan].scrollx;
  if (sx!=0)
    {
    sx+=allscrollx;//общая добавка всех скроллируемых панелей!
    if (sx<0)//отрицательный скролл превращаем в большой положительный
      {
      rendlenx=panel[pan].rendlenx;
      sx=rendlenx-((-sx)%rendlenx);
      }
    panel[pan].xb0+=sx;
    panel[pan].xb1+=sx;
    }
  }
}

void SetTimerFrequency(long firq)
//порт: частота таймера не перепрограммируется, время дает бэкенд
{
(void)firq;
}

void SoundSetup(void)
//порт: сетап звука - таблица делителей октав + waveOut-бэкенд
{
int okt,n;
//делители частоты таймера остальных октав
for (okt=1; okt<NUMOKT; ++okt)
  for (n=0; n<12; ++n) notediv[okt*12+n]=notediv[(okt-1)*12+n]/2;
//последняя нота До:
notediv[MAXNOTE]=notediv[MAXNOTE-12]/2;
//PORT: тик секвенсора 250 Гц - в аудио-потоке, независимо от FPS
WinSoundSetTick(SoundTickPump);
//PORT: звук включен по умолчанию (в оригинале включался через SOUND.BAT);
//клавиша S переключает звук вкл/выкл
soundflag=ON;
SoundOnOff();
Sound(SNDLILASTART);
}

void SoundTickPump(void)
//ПОРТ: бывший модуль звука таймерного прерывания; вызывается из
//аудио-потока каждые 4 мс (250 Гц) - независимо от частоты кадров
{
static int seed,timenote,actnote,oldfrqdiv;
int df,frqdiv;

if (soundstate<2)//0=запрос на стоп звука
  {
  soundstate=1;//1=стоп звука
  frqdiv=2;//частота ~600 кГц
  goto setdiv;
  }

if (soundstate==2)//старт звука
  {
  timenote=actnote=0;//идём в начало первой ноты кэша
  soundstate=3;
  }

//параметры ноты кэша
frqdiv=notediv[notecash[actnote]];
timenote+=VRS_TIME_STEP;
if (timenote>=durnote)
  {
  timenote=0;
  if (++actnote>=numnotecash) soundstate=0;
  }

//ширина полосы шума
df= (noisediv!=0)? frqdiv/noisediv:frqdiv;
if (df>0)
  {
  seed=25741*seed+13851;//Randi()
  //добавляем шум симметрично к частоте
  frqdiv+=-df/2+((0x7fff&seed)>>4)%(df+1);
  }

setdiv:
if (oldfrqdiv!=frqdiv)
  {
  SoundToneHW(frqdiv);//порт: меандр PIT -> waveOut
  oldfrqdiv=frqdiv;
  }
}

void USERandPENstartpoz(void)
//начальная позиция наблюдателя и PEN
{
//юзера в оригин земли и выше на рост человека
Sound(SNDSTARTPOZ);
USERearfixXYZgrad(0,0,FUSERZ0,0);
//скос взгляда нулевой
PEN.zgrad=0;
PEN.pozshift=YES;
}

void LilaLabLoad(void)
//читаем с диска стартовые лабиринт[ы]
{
int tmem,pan,lsize,i,n,ld[10];
byte far *lbuf;
byte far *buf;
char b[4];

lsize=MAXLABLEN;
//выделяем временный буфер памяти
tmem=GetLockMEM(ADDR_VRS,lsize);
if (tmem<=ISNOT) return;
lbuf=memory[tmem].rampntr;

//инсталлируем часть labdata[LLBMS] на случай отсутствия файла ll.lii
for (n=0; n<SYSBMS; ++n)
  {
  labdata[n].sysbms=n;
  labdata[n].glass=NO;
  labdata[n].sides=SIDES;
  }
labdata[GLASS].sysbms=GLASS;
labdata[GLASS].glass=YES;
labdata[GLASS].gradcon=10;

//грузим [часть] labdata с диска, формат см в файле!
if (OpenRead("ll.lii",0,lbuf,lsize)==OK)
  {
  buf=lbuf;
  while (1)
    {
    //пустая строка (lbuf[0]=CR, lbuf[1]=LF): конец файла
    if (buf[1]==ASCIILF) break;
    //аварийный конец, если будет ~превышена байтовая длина буфера
    if ((buf-lbuf)>=(lsize-100)) break;
    for (i=0; i<9; ++i)//вытаскиваем 9 целых чисел
      {
      b[0]=buf[0];
      b[1]=buf[1];
      b[2]=buf[2];
      b[3]=0; ld[i]=atoi(b);
      buf+=4;//сдвиг к следующему числу строки
      }
    buf+=2;//сдвиг к началу следующей строки
    n=ld[0];//индекс labdata
    if ((n<0)||(n>=LLBMS)) break;//конец по неверному индексу
    labdata[n].sysbms=  ld[1];
    labdata[n].glass=   ld[2];
    labdata[n].gradcon= ld[3];
    labdata[n].gradampx=ld[4];
    labdata[n].gradnum2=ld[5];
    labdata[n].scrollx= ld[6];
    labdata[n].rfluct=  ld[7];
    labdata[n].sides=   ld[8];
    }
  }

//грузим лабиринт VRS
if (OpenRead("vrs.ll",0,lbuf,lsize)==OK)
  Labirint(ADDR_VRS,d3null,FIX320,FIX240,lbuf,lsize,0);
//столб с текстом из оригна VRS
Stolb(ADDR_VRS,d3null,50,1440,DOSKAV+TEXTR,20,SIDE0);
//стекло вокруг и над столбом
pan=GetPANtxt(ADDR_VRS,0,8,5,1280);
panfill(pan,0,0,0,0,0);
pantext(pan,0,grnblu[12],2,2,  ". .");
pantext(pan,0,grnblu[12],0,3,".ЛИЛА.");
SetPanelPar(pan,13,YES,10,0,0,1,0,SIDES);
Set4Pan(pan,d3null,640,3360);

//грузим лабиринт AP_First: это неизменная часть панелей AP!
if (OpenRead("ap1.ll",0,lbuf,lsize)==OK)
  Labirint(ADDR_APFirst,d3null,FIX320,FIX240,lbuf,lsize,0);
//столб с текстом из оригна AP
Stolb(ADDR_APFirst,d3null,40,300,DOSKAV+TEXTRG,10,SIDES);
pan=GetPANtxt(ADDR_APFirst,5,9,1,320);
panfill(pan,0,0,0,0,0);
pantext(pan,5,blu[12],0,0,"Первая ПП");
SetPanelPar(pan,FULPAN,YES,10,0,0,0,0,SIDES);
Set4Pan(pan,d3null,160,240);

UnLockMEM(tmem);
}

void PaintText88(char far *tstr,int tlen,int bmap,int x,int z,
                 int fgbg,byte far *fgcol,int bgcol)
//текст с трафаретом из 8x8 fg-цветов, плюс возможно const bg-цвет;
//fgcol[i][j]: i растет слева направо, j растет снизу вверх !
//эта функция для моды битмапа 2, которая однако не проверяется!
{
byte far *memram;
byte far *ramlit;
byte far *ramstr;
byte near *ramfnt;
int i,j,bitstr,let,lenx,lenz,lenz8,maxlen;
byte far *fgc;

if (bmap<=ISNOT) return;
memram=memory[bmap].rampntr;
lenx=memory[bmap].lenx;
lenz=memory[bmap].lenz;
if ((z<0)||((z+7)>=lenz)) return;
if ((x<0)||((x+7)>=lenx)) return;
maxlen=(lenx-x)/8;
if (tlen>maxlen) tlen=maxlen;
ramstr=memram+x*lenz+z;
lenz8=lenz*8;

for (i=0; i<tlen; ++i)
  {
  let=(byte)tstr[i];
  ramlit=ramstr+(i+1)*lenz8;
  ramfnt=&fnt.pattern[let][0];
  fgc=fgcol+64;
  switch(fgbg)
    {
    default:
    case PAINTFG:
      for (j=7; j>=0; --j)
        {
        fgc-=8;
        ramlit-=lenz;
        bitstr=ramfnt[j];
        if (bitstr==0) continue;
        if (bitstr&128) ramlit[0]=*fgc;
        if (bitstr&64)  ramlit[1]=*(fgc+1);
        if (bitstr&32)  ramlit[2]=*(fgc+2);
        if (bitstr&16)  ramlit[3]=*(fgc+3);
        if (bitstr&8)   ramlit[4]=*(fgc+4);
        if (bitstr&4)   ramlit[5]=*(fgc+5);
        if (bitstr&2)   ramlit[6]=*(fgc+6);
        if (bitstr&1)   ramlit[7]=*(fgc+7);
        }
      break;
    case PAINTFGBG:
      for (j=7; j>=0; --j)
        {
        fgc-=8;
        ramlit-=lenz;
        bitstr=ramfnt[j];
        ramlit[0]= (bitstr&128)? *fgc    :bgcol;
        ramlit[1]= (bitstr&64)?  *(fgc+1):bgcol;
        ramlit[2]= (bitstr&32)?  *(fgc+2):bgcol;
        ramlit[3]= (bitstr&16)?  *(fgc+3):bgcol;
        ramlit[4]= (bitstr&8)?   *(fgc+4):bgcol;
        ramlit[5]= (bitstr&4)?   *(fgc+5):bgcol;
        ramlit[6]= (bitstr&2)?   *(fgc+6):bgcol;
        ramlit[7]= (bitstr&1)?   *(fgc+7):bgcol;
        }
      break;
    }
  }
}

void PaintText(char far *tstr,int tlen,int bmap,int x,int z,
               int fgbg,int fgcol,int bgcol)
//не более tlen литер из текстстроки tstr рисуется в битмап
//bmap (это номер блока памяти текстуры);
//fg-цвет пикселей букв col, bg-цвет под буквами не меняется;
//оригин (левый нижний пиксель) первой литеры в x,z битмапа,
//для самих x/z отсчет всегда от левого нижнего угла битмапа!
//при налезании литеры на конец битмапа конец вывода текста;
//эта функция для моды битмапа 2, которая однако не проверяется!
{
byte far *memram;
byte far *ramlit;
byte far *ramstr;
byte near *ramfnt;
int i,j,bitstr,let,lenx,lenz,lenz8,maxlen;

if (bmap<=ISNOT) return;
memram=memory[bmap].rampntr;
lenx=memory[bmap].lenx;
lenz=memory[bmap].lenz;
if ((z<0)||((z+7)>=lenz)) return;
if ((x<0)||((x+7)>=lenx)) return;
//макс количество литер до правого конца битмапа
maxlen=(lenx-x)/8;
if (tlen>maxlen) tlen=maxlen;
//указатель на начальный пиксель всей строки литер
ramstr=memram+x*lenz+z;
lenz8=lenz*8;

for (i=0; i<tlen; ++i)//цикл по литерам текста
  {
  let=(byte)tstr[i];//очередная литера
  //указатель на левый-нижний пиксель (литеры+1)
  ramlit=ramstr+(i+1)*lenz8;
  ramfnt=&fnt.pattern[let][0];//первый байт фонта литеры

  switch(fgbg)
    {
    default:
    case PAINTFG:
      for (j=7; j>=0; --j)//цикл по столбцам фонта литеры
        {
        ramlit-=lenz;//указатель на нижний-очередной пиксель литеры
        bitstr=ramfnt[j];//j-й байт фонта литеры
        if (bitstr==0) continue;//пустой байт пропускаем
        //битовая одновременная вертикальная развертка байта фонта
        //пиксели фона внутри литер не меняем !
        if (bitstr&128) ramlit[0]=fgcol;
        if (bitstr&64)  ramlit[1]=fgcol;
        if (bitstr&32)  ramlit[2]=fgcol;
        if (bitstr&16)  ramlit[3]=fgcol;
        if (bitstr&8)   ramlit[4]=fgcol;
        if (bitstr&4)   ramlit[5]=fgcol;
        if (bitstr&2)   ramlit[6]=fgcol;
        if (bitstr&1)   ramlit[7]=fgcol;
        }
      break;
    case PAINTBG:
      for (j=7; j>=0; --j)
        {
        ramlit-=lenz;
        bitstr=ramfnt[j];
        //пиксели литер внутри фона не меняем !
        if (!(bitstr&128)) ramlit[0]=bgcol;
        if (!(bitstr&64))  ramlit[1]=bgcol;
        if (!(bitstr&32))  ramlit[2]=bgcol;
        if (!(bitstr&16))  ramlit[3]=bgcol;
        if (!(bitstr&8))   ramlit[4]=bgcol;
        if (!(bitstr&4))   ramlit[5]=bgcol;
        if (!(bitstr&2))   ramlit[6]=bgcol;
        if (!(bitstr&1))   ramlit[7]=bgcol;
        }
      break;
    case PAINTFGBG:
      for (j=7; j>=0; --j)
        {
        ramlit-=lenz;
        bitstr=ramfnt[j];
        ramlit[0]= (bitstr&128)? fgcol:bgcol;
        ramlit[1]= (bitstr&64)?  fgcol:bgcol;
        ramlit[2]= (bitstr&32)?  fgcol:bgcol;
        ramlit[3]= (bitstr&16)?  fgcol:bgcol;
        ramlit[4]= (bitstr&8)?   fgcol:bgcol;
        ramlit[5]= (bitstr&4)?   fgcol:bgcol;
        ramlit[6]= (bitstr&2)?   fgcol:bgcol;
        ramlit[7]= (bitstr&1)?   fgcol:bgcol;
        }
      break;
    }
  }
}

int GetPANtxt(int owner,int glen,int tlenx,int tlenz,int panlenx)
//выделяет панель размером panlenx (panlenz будет больше panlenx в
//tlenz/tlenx) см для последующей записи ~текстовых данных, размер
//битмапа будет tlenx/tlenz литер 8x8 плюс 2*glen пикселей рамки !
//возвращает номер панели или ISNOT при неудаче!
{
int bm,pan,blenx,blenz;
fix fpanlenx,fpanlenz;

if ((tlenx<=0)||(tlenz<=0)||(glen<0)) return ISNOT;

pan=GetPAN(owner);
if (pan<=ISNOT) return ISNOT;

blenx=8*tlenx+2*glen;
blenz=8*tlenz+2*glen;
//параметр FIX6D5: 'круглость круга' для текста, иначе на
//расстоянии с горизонт-м увеличением =1.0 получается сжатое Z,
//на весь экран 30 строк вместо 25 и буквы видны немного хуже...
bm=GetLockMEM2(owner,blenx,blenz,FIX6D5,YES);
//прозрачный цвет стандартно есть ... ---^
if (bm<=ISNOT) return ISNOT;
//если битмап есть:
LockPAN(pan);//лочим панель без ресурсов !

//сначала меняем параметры панели
fpanlenx=itofix(panlenx);
//длина z панели пропорциональна формату битмапа
fpanlenz=fixmuldiv(fpanlenx,itofix(blenz),itofix(blenx));
panel[pan].fmulx=fpanlenx/blenx;
panel[pan].fmulz=fpanlenz/blenz;//размеры битмапа-на всю панель

//а потом присоединяем ресурсы:
SetPanelRes(pan,bm,ISNOT);//конверта пока нет !
//координаты панели и содержимое битмапа определяет вызывающий код!
return pan;
}

int GetLockMEM2(int owner,int lenx,int lenz,fix fmulz,int transfl)
//выделяет блок двумерной памяти для битмапа с lenx/lenz и fmulz
//(для круглости круга и нормальной высоты литер fmulz=FIX6D5),
//а также флагом наличия прозрачного цвета transfl;
//возвращает номер заголовка блока, либо ISNOT с установкой errmem
{
dword dlen;
word len;
int bm;

errmem=0;

if ((lenx<=0)||(lenz<=0))
  {errmem=5;//неверные размеры битмапа
  WarningWKP("MEM2: lenx<=0 || lenz<=0 !\n",errmem);
  return ISNOT;
  }

dlen=((dword)lenx)*((dword)lenz);
if (dlen>MAXMEMLEN)
  {errmem=4;//длина блока памяти для битмапа более разрешенного
  WarningWKP("MEM2len>MAXMEMLEN !\n",errmem);
  return ISNOT;
  }
len=(word)dlen;

//выделение памяти текстуры битмапа
bm=GetLockMEM(owner,len);
if (bm==ISNOT) return ISNOT;//ошибка, выделение аварийного блока
//сбой errmem печатается в GetLockMEM() !
//else OK, устанавливаем поля заголовка битмапа
memory[bm].lenx=lenx;
memory[bm].lenz=lenz;
memory[bm].transflag=transfl;
memory[bm].fmulz=fmulz;
memory[bm].rammode=2;
return bm;
}

void SetConvert1to1(int conv)
//делает конверт с преобразованием цветов 1:1
{
int i;
if ((conv<=ISNOT)||(memory[conv].len<256)) return;//ошибка
for (i=255; i>=0; --i) memory[conv].rampntr[i]=i;//цвет i -> i !
}

void SetConvertSort(int conv,int i0,int i1,int i2)
//делает конверт с преобразованием цветов;
//для каждого цвета с компонентами rgb ищется цвет с
//порядком компонент, определяемых индексами i0,i1,i2:
//0,1,2=r,g,b; 1,0,2=g,r,b; 1,2,0=g,b,r и т.д.
{
int col,col1,c3,c31,r,g,b,r1,g1,b1;

if ((conv<=ISNOT)||(memory[conv].len<256)) return;
SetConvert1to1(conv);//это для цветов 0...8
for (col=9; col<256; ++col)//основные цвета меняем
  {
  c3=3*col;
  r=syspal[c3+i0];
  g=syspal[c3+i1];
  b=syspal[c3+i2];
  memory[conv].rampntr[col]=0;
  for (col1=9; col1<256; ++col1)
    {
    c31=3*col1;
    r1=syspal[c31];
    g1=syspal[c31+1];
    b1=syspal[c31+2];
    if ((r==r1)&&(g==g1)&&(b==b1))
      {
      memory[conv].rampntr[col]=col1;
      break;
      }
    }
  }
}

void SetConvert2nega(int conv)
//делает конверт с преобразованием цветов в серый негатив
{
int i,c3,bw,col;
if ((conv<=ISNOT)||(memory[conv].len<256)) return;//ошибка
//цвета 1...8 меняем согласно предполагаемой яркости
for (i=0; i<=8; ++i) memory[conv].rampntr[i]= (i>0)? gray[12-i]:0;
for (i=9; i<256; ++i)//а основные цвета согласно истинной яркости
  {
  c3=i*3;
  bw=(syspal[c3]+syspal[c3+1]+syspal[c3+2])/3;
  col=gray[15-bw/4];
  memory[conv].rampntr[i]=col;
  }
}

void SetConvertColness(int conv,fix colness)
//конверт с нужной цветностью
{
byte pal[PAL768];
int i,c3;
if ((conv<=ISNOT)||(memory[conv].len<256)) return;//ошибка
SetConvert1to1(conv);//для цветов 0...8
//рассчёт палитры pal с цветностью colness из палитры syspal,
//имеющей стандартную цветность FIXC (fix 0.133):
ConvertPalette(syspal,pal,fixdiv(colness,FIXC),9);
//добавляем в pal предполагаемую палитру цветов 1...8
for (i=0; i<=8; ++i)
  {
  c3=i*3; pal[c3]=pal[c3+1]=pal[c3+2]= (i>0)? 4*(4+i):0;
  }
//рассчет таблицы преобр-я цветов pal->syspal
ComputConvsys(pal,memory[conv].rampntr,0);
}

void ComputConvsys(byte far *srcpal,byte far *dstram,int transcol)
//рассчет таблицы преобразования цветов из палитры srcpal в syspal
//с учетом прозрачного цвета transcol и запись её в dstram[0..256]
{
int col,c3,r,g,b;
byte pal[PAL768];

//корректируем палитру srcpal во временную
ConvertPalette(srcpal,pal,FIXC,0);

//преобразовываем палитру из pal в syspal путем огрубления
//интенсивностей цветов с шагом=4
for (col=0; col<256; ++col)
  {
  c3=col*3;
  //интенсивности компонент цвета col палитры pal
  r=pal[c3];
  g=pal[c3+1];
  b=pal[c3+2];
  //таблица преобразования
  dstram[col]=nnn[r>>2][g>>2][b>>2];
  }
//если есть прозрачный цвет - заносим его в таблицу
if ((0<=transcol)&&(transcol<=255)) dstram[transcol]=0;
}

int GetLockMEMconv(int owner,int transfl)
//выделяет/закрепляет один конверт с флагом преобразования
//цвета или цветов в прозрачный цвет transfl;
//возвращает номер блока памяти конверта;
{
int mem;
mem=GetLockMEM(owner,256);
if (mem>ISNOT) memory[mem].transflag=transfl;
return mem;
}

int LoadPPfile(int bm,char *fname,long offset,fix colness)
//из fname со сдвигом offset от начала грузится текстура в блок
//памяти bm и преобразуется в syspal с fix цветностью colness
//или int цветностью из файла, если только она там есть !
//возврат OK или ERR если файла нет !
//pp-файл содержит 64000+768+9 [+5] = 64777 [64782] байт:
//текстура
//палитра
//\r\n\r\nTCL\r\n
//и опционально int colorness:
//CLN\r\n
{
byte tmpram[PPPALLEN];
byte far *texram;
long len,lenx,lenz;
int transcol,x,z,lx,lz;
char num[10];
int hnd;

if (bm<=ISNOT) return ERR;

len=memory[bm].len;
lenx=memory[bm].lenx;
lenz=memory[bm].lenz;

if ((len!=(lenx*lenz))||(lenx<=0)||(lenz<=0)) return ERR;

//грузим текстуру с диска из pp файла
if ((hnd=open(fname,O_RDONLY|O_BINARY,S_IREAD))==-1) return ERR;
lseek(hnd,offset,SEEK_SET);
Read(hnd,memory[bm].rampntr,(word)len);
//определяем длину для решения о загрузке цветности из файла
len=filelength(hnd);
memory[bm].rammode=1;//входная  мода =1 текстуры pp-файла
ConvertMEMmode(bm,2);//выходная мода =2 для рендера

//грузим палитру, номер прозрачного цвета и цветность
lseek(hnd,PPTEXLEN,SEEK_SET);
Read(hnd,tmpram,PPPALLEN);
close(hnd);

strncpy(num,(char*)tmpram+PAL768,9);
num[9]=0;
transcol=atoi(num);
if (len>64777u)//если есть - цветность берётся из файла !
  {
  strncpy(num,(char*)tmpram+PAL768+9,5);
  num[5]=0;
  colness=itofix(atoi(num));
  }

memory[bm].transflag=NO;
if (palfromppfile==0)//преобразуем битмап в цвета syspal
  {
  //прозрачный цвет отмечаем в флаге текстуры
  if ((0<=transcol)&&(transcol<=255)) memory[bm].transflag=YES;

  //вычисляем табл. преобр-я цветов битмапа в syspal:
  //исходная палитра битмапа из tmpram -> в tmpram с коррекцией цветности
  ConvertPalette(tmpram,tmpram,colness,0);
  //рассчет таблицы преобр-я цветов в syspal:
  //из палитры в tmpram[0...768] -> в tmpram[0...256]
  ComputConvsys(tmpram,tmpram,transcol);

  //преобразуем текстуру в syspal:
  lx=(int)lenx;
  lz=(int)lenz;
  for (x=0; x<lx; ++x)
    {
    texram=memory[bm].rampntr+x*lz;
    for (z=lz-1; z>=0; --z) texram[z]=tmpram[texram[z]];
    }
  }
else
if (palfromppfile==1)//меняем syspal на палитру файла
  {
  memcpy(syspal,tmpram,PAL768);
  SetPalette(syspal,0,255);
  }
//else =2: палитру не меняем, но и битмап не преобразуем !

return OK;
}

int let2i(int let)
//преобразует одноразрядное 64/65-ричное число в двоичное:
//литера "0123456789  ABC...Z  abc...z  {  }    |" преобразуется
//     в  0...     9 10... 35 36... 61 62 63   64
//в остальных случаях возращает 0 !
{
int val;
val=0;
if (('0'<=let)&&(let<='9')) val=let-'0';
if (('A'<=let)&&(let<='Z')) val=let-'A'+10;
if (('a'<=let)&&(let<='z')) val=let-'a'+36;//10 цифр плюс 26 букв
if (let=='{') val=62;
if (let=='}') val=63;
if (let=='|') val=64;
return val;
}

void LilaPaintSetup(void)
//раскодировка цветов лейбла
{
int i,col;
//преобразуем литеры "0123456789ABC..." в плавные цвета syspal
//а пробелы в прозрачный цвет 0
for (i=0; i<LILALEN; ++i)
  {
  col=lila[i];
  if (col==32) lila[i]=0;
  else         lila[i]=red[let2i(col)];
  }
}

int LoadSyspalFile(char *fname)
{
int hnd,i,c3;
hnd=open(fname,O_RDONLY|O_BINARY,S_IREAD);
if (hnd==-1) return ERR;//нет такого файла
Read(hnd,syspal,sizeof(syspal));
Read(hnd,nnn,sizeof(nnn));
close(hnd);
//устанавливаем некую палитру цветов 0...8
for (i=0; i<=8; ++i)
  {
  c3=i*3;
  syspal[c3]=syspal[c3+1]=syspal[c3+2]= (i>0)? 4*(4+i):0;
  }
return OK;
}

int GetLockMEM(int owner,word len)
//выделяет и однократно закрепляет блок памяти;
//возвращает номер выделенного блока памяти или ISNOT при ошибке;
//в текущей версии VRS блок памяти может быть только в RAM!
//всегда устанавливает код ошибки в errmem;
{
word fullen;//длина кратная 4, не менее 4, и не менее len !
int n;

errmem=0;

if (len>MAXMEMLEN)
  {errmem=3;//запрошен слишком длинный блок!
  WarningWKP("MEMlen>MAXMEMLEN !\n",errmem);
  return ISNOT;
  }

if (len==0) ++len;//блок с len=0 получает длину len=1, а fullen=4 !!!
fullen=4*(len/4);
if (fullen<len) fullen+=4;

//если места нет даже с учетом всех дыр - ошибка
if (ramfullfree<fullen)
  {errmem=2;//не хватает памяти для блока!
  WarningWKP("ramfullfree<MEMlen !\n",errmem);
  return ISNOT;
  }

//ищем незакрепленный блок памяти нужного размера в группе блоков
for (n=0; n<=maxmemnum; ++n)
  if ((memory[n].fullen==fullen)&&(memory[n].count==0))//блок найден
    {
    memory[n].owner=owner;//устанавливаем обязательные поля
    memory[n].len=len;
    memory[n].lenx=len;//а это на случай двумерного доступа
    memory[n].lenz=1;
    LockMEM(n);
    return n;
    }

//если блока нет, выделяем память из непрерывного куска (н.к.) RAM

//если нет места в н.к. RAM - делаем сжатие памяти
if (ramfree<fullen) CompressMEM();
//тут уже гарантировано место под len байт в н.к. RAM !

//ищем первый свободный заголовок блока (нулевой длины)
for (n=0; n<MAXMEM; ++n) if (memory[n].fullen==0) break;

if (n>=MAXMEM)//нет заголовков блоков нулевой длины
  {
  CompressMEM();//делаем сжатие памяти
  //теперь снова ищем заголовок блока
  for (n=0; n<MAXMEM; ++n) if (memory[n].fullen==0) break;
  if (n>=MAXMEM)
    {errmem=1;//нет свободных заголовков
    WarningWKP("MEM>=MAXMEM !\n",errmem);
    return ISNOT;
    }
  }

//n=номер свободного заголовка блока памяти,
//в самый первый раз n=0=ISNOT !!!
if (maxmemnum<n) maxmemnum=n;//корректируем размер группы блоков
ZeroMEMhead(n);
memory[n].owner=owner;//устанавливаем обязательные поля
memory[n].fullen=fullen;
memory[n].len=len;
//начальный адрес любого выделенного блока будет кратен 4,
  //т.к. ramfullstart и fullen всех блоков всегда кратны 4 !
memory[n].rampntr=(byte far *)ramstart;
memory[n].lenx=len;//а это на случай двумерного доступа
memory[n].lenz=1;
LockMEM(n);//там корректируется также общий свободный размер памяти!
//а здесь корректируем только размер н.к. памяти
ramfree-=fullen;
ramstart+=fullen;
return n;
}

void CompressMEM(void)
//сжатие RAM
{
long sortkey[MAXMEM];//параметры сортировки MEM
int sortind[MAXMEM];//номера MEM в порядке возрастания sortkey
byte far *ram;
byte huge *ramnew;//новое начало RAM для очередного блока памяти
byte huge *ramold;//старое начало RAM
int i,n,smemnum;
word len;

//запись полных RAM-адресов блоков для сортировки
smemnum=0;
for (n=0; n<=maxmemnum; ++n)
  {
  //не будем сортировать блок VGAMEM !!!
  if (n==VGAMEM) continue;
  ram=memory[n].rampntr;
  sortind[smemnum]=n;
  //PORT: ключ сортировки - сам указатель (в DOS был seg:off 20-битный
  //линейный адрес, на плоской памяти формула seg<<4+off дает коллизии!)
  sortkey[smemnum]=(long)(uintptr_t)ram;
  ++smemnum;
  }
//сортировка блоков по порядку возрастания адресов
QuickSort(smemnum,sortkey,sortind);

//перемещение всех занятых блоков в RAM вниз, а всех дыр вверх:

ramnew=ramfullstart;//начало RAM для всего набора блоков памяти
//спускаем занятые блоки вниз
for (i=0; i<smemnum; ++i)//перебираем массив сортированных номеров
  {
  n=sortind[i];//действительный номер блока
  if (memory[n].count==0)//для дыр только обнуляем длины их блоков
    {
    memory[n].len=0;
    memory[n].fullen=0;
    continue;
    }
  len=memory[n].fullen;
  ramold=(byte huge *)memory[n].rampntr;
  if (ramold!=ramnew)//блок не на новом месте
    {
    //перемещаем содержимое памяти на новое место
    CopyRAM(ramnew,ramold,len);
    memory[n].rampntr=(byte far *)ramnew;
    }
  ramnew+=len;
  }

//слияние всех дыр в н.к. RAM
ramstart=ramnew;
ramfree=ramfullfree;

//возможно корректировка maxmemnum
//ищем с конца первый ненулевой длины блок
for (n=MAXMEM-1; n>0; --n) if (memory[n].fullen!=0) break;
maxmemnum=n;
}

void RenderSteklPanels(void)
//рендер всех стеклянных панелей
{
int pan,n,spannum;//количество стеклянных панелей;
long sortkey[STEKLMAXPAN];//параметры сортировки PAN,
int sortind[STEKLMAXPAN];//номера PAN в порядке возрастания sortkey;

//сортировка стеклянных панелей
spannum=0;
for (n=0; n<=maxpannum; ++n)
  {
  if (panel[n].flag!=YES) continue;
  if (panel[n].render!=YES) continue;
  //пропускаем _нестеклянные_ панели
  if (panel[n].glass==NO) continue;
  sortind[spannum]=n;
  sortkey[spannum]=panel[n].pen.y0;
  if (++spannum>=STEKLMAXPAN) break;
  }
//сортировка панелей по порядку возрастания расстояний pen.y0
QuickSort(spannum,sortkey,sortind);

//рендер стеклянных панелей от дальних к ближним
for (n=spannum-1; n>=0; --n)
  {
  pan=sortind[n];//действительный номер панели
  RenderPanel(pan);
  }
}

void AllAPdataSetup(void)
{
int n;//адресный номер AP

for (n=0; n<=numap; ++n)//0=адрес VRS (надо для преобр-й координат) !
  {
  AP[n].state=AP_START;
  AP[n].taskflag=ON;
  AP[n].taskadress=AP_Null;//пустая заглушка на случай пропуска адреса AP
  AP[n].ear=d3null;//ставим все AP в оригин VRS
  AP[n].earshift=YES;
  }

AP[ADDR_APFirst].taskadress=AP_First;
AP[ADDR_APRend].taskadress=AP_Render;
AP[ADDR_APOVGAg].taskadress=AP_OutVGAgra;
//ставим систему AP_First в стартовую позицию
AP[ADDR_APFirst].ear=ap1startpoz;
}

void WKP(void)
//WK с печатью!
{
printf("Wait Key Press !\n");
WaitKeyPress();
}

void VRSWorldSetup(void)
//система загружает битмапы и наполняет мир несколькими панелями...
{
int bs[12],cs[12];
int n,x,z,i,var;

//выделяем и стираем битмап стены
wallbitm=GetLockMEM2(ADDR_ALL,320,200,FIX6D5,NO);
SetMEMtoConstval(wallbitm,0);

//выделяем 64к-битмап дали
dalibitm=GetLockMEM2(ADDR_VRS,320,200,FIX1,NO);
//грузим даль из файла если возможно
var=Dalivar(0);//по варианту 0 или 1
if (var>1)//нет никаких файлов дали!
  {
  //уменьшаем размер битмапа дали !!
  UnLockMEM(dalibitm);
  dalibitm=GetLockMEM(ADDR_VRS,512);
  }

//делаем набор битмапов

//стекло (прозрачное серое)
glassbitm=GetLockMEM2(ADDR_ALL,30,16,FIX1,YES);
SetMEMtoConstval(glassbitm,0);

//однородные [текстуры]
for (n=0; n<=1; ++n) bs[n]=GetLockMEM2(ADDR_ALL,30,16,FIX1,NO);
SetMEMtoConstval(bs[0],red[11]);
SetMEMtoConstval(bs[1],redgrn[11]);

//бетон и доски
for (n=2; n<=9; ++n) bs[n]=GetLockMEM2(ADDR_ALL,30,30,FIX1,NO);

for (x=0; x<30; ++x)//вертикальные полосы
  {
  //бетон
  switch(x%10)
    {
    case 0: case 1: case 2: i= 8; break;
    case 3: case 4:         i= 6; break;
    case 5: case 6: case 7: i=10; break;
    case 8: case 9:         i=12; break;
    }
  PaintBitmap(bs[2],x,0,x,64,red[i]);
  PaintBitmap(bs[3],x,0,x,64,redgrn[i-1]);//жёлтый цвет делаем темнее
  //доски
  i=x%30;  i= (i<15)? i:(29-i);  i=5+i/4;
  RPBline(bs[6],x,0,x,64,90,4,8,red+i+1,0,3);
  RPBline(bs[7],x,0,x,64,90,4,8,redgrn+i,0,3);
  }

for (z=0; z<30; ++z)//горизонтальные полосы
  {
  //бетон
  switch(z%10)
    {
    case 0: case 1: case 2: i= 8; break;
    case 3: case 4:         i= 6; break;
    case 5: case 6: case 7: i=10; break;
    case 8: case 9:         i=12; break;
    }
  PaintBitmap(bs[4],0,z,64,z,red[i]);
  PaintBitmap(bs[5],0,z,64,z,redgrn[i-1]);
  //доски
  i=z%30;  i= (i<15)? i:(29-i);  i=5+i/4;
  RPBline(bs[8],0,z,64,z,0,4,8,red+i+1,0,3);
  RPBline(bs[9],0,z,64,z,0,4,8,redgrn+i,0,3);
  }

//набор конвертов без преобр-я в прозрачный цвет

for (n=0; n<=7; ++n) cs[n]=GetLockMEMconv(ADDR_ALL,NO);

SetConvert2nega(cs[0]);//серый негатив
SetConvertColness(cs[1],0);//цветность 0
SetConvertColness(cs[2],-FIX1);//цветность -1
//перестановки rgb:
SetConvertSort(cs[3],0,2,1);//rbg
SetConvertSort(cs[4],1,0,2);//grb
SetConvertSort(cs[5],1,2,0);//gbr
SetConvertSort(cs[6],2,0,1);//brg
SetConvertSort(cs[7],2,1,0);//bgr

//соответствие битмапов и конвертов

for (i=0; i<10; i+=2)//7*10/2=35 вариантов однородных, бетона и досок
  {
  n=7*i/2;
  sysbms[n]=bs[i];     //r
  sysbms[n+1]=bs[i+1]; //rg
  sysbms[n+2]=bs[i];   syscvs[n+2]=cs[4];//g
  sysbms[n+3]=bs[i+1]; syscvs[n+3]=cs[6];//gb
  sysbms[n+4]=bs[i];   syscvs[n+4]=cs[5];//b
  sysbms[n+5]=bs[i+1]; syscvs[n+5]=cs[3];//rb
  sysbms[n+6]=bs[i];   syscvs[n+6]=cs[1];//gray
  }

//9 вариантов стены
for (n=0; n<=8; ++n) sysbms[n+35]=wallbitm;
//syscvs[35]=ISNOT;rgb
syscvs[36]=cs[3];//rbg
syscvs[37]=cs[4];//grb
syscvs[38]=cs[5];//gbr
syscvs[39]=cs[6];//brg
syscvs[40]=cs[7];//bgr
syscvs[41]=cs[1];//gray
syscvs[42]=cs[2];//-rgb
syscvs[43]=cs[0];//-gray

//прозрачное стекло
sysbms[44]=glassbitm;

//читаем с диска стартовые лабиринт[ы]
LilaLabLoad();
}

int Dalivar(int var)
//попытка установки варианта дали var (с загрузкой файлов дали) !
//возвращает действительно установленный вариант!
{
var=min(var,DALIVAR-1);
var=max(var,0);
dalivar=var;
newdali=YES;

if (dalivar==0)//вариант дали 0
  {
  //двумерная готовая даль
  dali2d=YES; dalipaint=NO;
  //грузим облака far0.pp или файл из строки вызова
  if ( LoadPPfile(dalibitm,(VRS_argc<=1)? ppfile[0]:VRS_argv[1],0,FIX2) == ERR )
    ++dalivar;//нет такого файла
  }

if (dalivar==1)//вариант дали 1
  {
  //двумерная готовая даль
  dali2d=YES; dalipaint=NO;
  //грузим горы
  if ( LoadPPfile(dalibitm,ppfile[1],0,FIX2) == ERR )
    ++dalivar;//нет такого файла
  }

if (dalivar>1)//одномерная даль с перекраской
  {
  dali2d=NO; dalipaint=YES;
  }

return dalivar;
}

void SetPanelPar(int pan,int heomode, int glass, int gradcon,int gradampx,
                         int gradnum2,int scrollx,int rfluct,int sides)
//установка расширенных параметров панели
{
int num;
if (pan<=ISNOT) return;
panel[pan].heomode =heomode;
panel[pan].glass   =glass;
panel[pan].gradcon =gradcon;
panel[pan].gradampx=gradampx;
num                =gradnum2;
if (num<=0) num=1;//число полуволн на длине панели>=1 !
panel[pan].gradlenx=(panel[pan].flenx*2)/num;
panel[pan].scrollx =scrollx;
panel[pan].rfluct  =rfluct;
panel[pan].sides   =sides;
}

void CopyMEM2(int dbm,int dx0,int dz0,
              int sbm,int sx0,int sz0,
              int lenx,int lenz)
//совмещая оригины окон битмапов dbm,sbm (dx0,dz0 и sx0,sz0)
//копируем текстуру из sbm в dbm вверх и вправо по окнам с
//размерами dst/src окон не более lenx,lenz;
{
int slenx,slenz,dlenx,dlenz;
int slenx0,slenz0,dlenx0,dlenz0;
int lx,lz,dx,sx,sx1;
byte far *sram;
byte far *dram;

if ((dx0<0)||(dz0<0)||(sx0<0)||(sz0<0)) return;
slenx=memory[sbm].lenx;
slenz=memory[sbm].lenz;
dlenx=memory[dbm].lenx;
dlenz=memory[dbm].lenz;
//макс длины src/dst окон
slenx0=slenx-sx0;
slenz0=slenz-sz0;
dlenx0=dlenx-dx0;
dlenz0=dlenz-dz0;
//выбираем минимумы из src/dst длин
lx=min(slenx0,dlenx0);
lz=min(slenz0,dlenz0);
//корректируем размеры окна
lenx=min(lenx,lx);
lenz=min(lenz,lz);
if ((lenx<=0)||(lenz<=0)) return;
//есть что копировать
ConvertMEMmode(sbm,2);
ConvertMEMmode(dbm,2);
sx1=sx0+lenx;
sram=memory[sbm].rampntr;
dram=memory[dbm].rampntr;
//копируем текстуру по столбцам слева направо
for (sx=sx0,dx=dx0; sx<sx1; ++sx,++dx)
  CopyRAM(dram+dx*dlenz+dz0,sram+sx*slenz+sz0,lenz);
//если lenx>>lenz то можно ускорить простым циклом по x ! @
}

void InterCopyBitmap(int dstbm,int srcbm,int step)
//битмап srcbm содержит байтовый шаблон интенсивности, кот.
//сначала копируются в dstbm с шагом step>=2 по x/z, затем
//'пропущенные' точки dstbm также заполняются путем линейной
//интерполяции интенсивности, сначала по x-строкам, затем
//по z-столбцам; dstbm выделяется в вызывающем коде!
//если dstbmlenx/z != srcbmlenx/z*step-(step-1), то на
//правом и на верхнем краях битмапа будет нестертая грязь!
{
int i,i0,i1,x,x0,x1,z,z0,z1,sx,sz,dx,dz;
int slenx,slenz,dlenx,dlenz;
byte far *sram;
byte far *dram;

if (step<2) return;

slenx=memory[srcbm].lenx;
slenz=memory[srcbm].lenz;
sram=memory[srcbm].rampntr;
ConvertMEMmode(srcbm,2);

dlenx=memory[dstbm].lenx;
dlenz=memory[dstbm].lenz;
dram=memory[dstbm].rampntr;
ConvertMEMmode(dstbm,2);//моды битмапов 2 !

//копирование с шагом step
for (sx=0; sx<slenx; ++sx)
  {
  dx=sx*step;
  if (dx>=dlenx) break;
  for (sz=0; sz<slenz; ++sz)
    {
    dz=sz*step;
    if (dz>=dlenz) break;
    dram[dx*dlenz+dz]=sram[sx*slenz+sz];
    }
  }

//интерполяция по x-строкам с шагом по z=step
for (dz=0; dz<dlenz; dz+=step)
  {
  for (dx=0; dx<dlenx; dx+=step)
    {
    x0=dx;
    x1=dx+step;
    if (x1>=dlenx) break;
    i0=dram[x0*dlenz+dz];
    i1=dram[x1*dlenz+dz];
    //интерполяция _между_ x0...x1
    for (x=x0+1; x<x1; ++x)
      {
      i=(i1*(x-x0)+i0*(x1-x))/step;
      dram[x*dlenz+dz]=(byte)i;
      }
    }
  }

//интерполяция по z-столбцам с шагом по x=1
for (dx=0; dx<dlenx; ++dx)
  {
  for (dz=0; dz<dlenz; dz+=step)
    {
    z0=dz;
    z1=dz+step;
    if (z1>=dlenz) break;
    i0=dram[dx*dlenz+z0];
    i1=dram[dx*dlenz+z1];
    //интерполяция _между_ z0...z1
    for (z=z0+1; z<z1; ++z)
      {
      i=(i1*(z-z0)+i0*(z1-z))/step;
      dram[dx*dlenz+z]=(byte)i;
      }
    }
  }
}

void RepaintBitmap(int bm,byte far *convcol)
//каждый байт битмапа bm перекрашивается в новый цвет
//из массива convcol с индексом=этому байту;
{
int x,z,lenx,lenz;
byte far *ram;
lenx=memory[bm].lenx;
lenz=memory[bm].lenz;
ConvertMEMmode(bm,2);//мода битмапа 2 !
ram=memory[bm].rampntr;
for (x=0; x<lenx; ++x)
  {
  for (z=0; z<lenz; ++z) ram[z]=convcol[ram[z]];
  ram+=lenz;
  }
}

void CopyBitmap(int dbm,int dx0,int dz0,
                int sbm,int sx0,int sz0,
                int lenx,int lenz,int porog)
//это CopyMEM2(), но копирование только если цвет >=porog:
//совмещая оригины окон битмапов dbm,sbm (dx0,dz0 и sx0,sz0)
//копируем текстуру из sbm в dbm вверх и вправо по окнам с
//размерами dst/src окон не более lenx,lenz;
{
int slenx,slenz,dlenx,dlenz,col;
int slenx0,slenz0,dlenx0,dlenz0;
int lx,lz,sx,sx1,sz,sz1,dx,dz;
byte far *sram;
byte far *dram;

if ((dx0<0)||(dz0<0)||(sx0<0)||(sz0<0)||(porog>255)) return;

if (porog<=0)//копирование без порога
  { CopyMEM2(dbm,dx0,dz0,sbm,sx0,sz0,lenx,lenz); return; }

slenx=memory[sbm].lenx;
slenz=memory[sbm].lenz;
dlenx=memory[dbm].lenx;
dlenz=memory[dbm].lenz;
//макс длины src/dst окон
slenx0=slenx-sx0;
slenz0=slenz-sz0;
dlenx0=dlenx-dx0;
dlenz0=dlenz-dz0;
//выбираем минимумы из src/dst длин
lx=min(slenx0,dlenx0);
lz=min(slenz0,dlenz0);
//корректируем размеры окна
lenx=min(lenx,lx);
lenz=min(lenz,lz);
if ((lenx<=0)||(lenz<=0)) return;
//есть что копировать
ConvertMEMmode(sbm,2);
ConvertMEMmode(dbm,2);
sx1=sx0+lenx;
sz1=sz0+lenz;
//копируем текстуру попиксельно
for (sx=sx0,dx=dx0; sx<sx1; ++sx,++dx)
  {
  dram=memory[dbm].rampntr+dx*dlenz;
  sram=memory[sbm].rampntr+sx*slenz;
  for (sz=sz0,dz=dz0; sz<sz1; ++sz,++dz)
    {
    col=sram[sz];
    if (col>=porog) dram[dz]=col;
    }
  }
//если lenx>>lenz то можно ускорить ставя цикл по x внутрь ! @
}

void PaintDbitm(void)
//[пере]закраска битмапа и строки периметра дали
{
//список упакованных параметров дали по порядку:
//номер прозрачного цвета для дали (0...255, нет его при <0):
//int dtranscol;//dalipar[0]=поизв: это глобальная переменная !

//параметры раскраски битмапа дали
int dcolmode;//dalipar[1]=0...3: 0=цвета абсолютные, 1=цвета из набора радуги,
  //2=цвета начиная с gray[0], 3=яркий цвет 255;
int dcolor0;//dalipar[2]=поизв: 16*(цвет dbitm[0] или индекс цвета);
int dcolstep;//dalipar[3]=произв: 256*(шаг по цвету dbitm[i->i+1]) !

//отклонение нулевого уровня дали от уровня горизонта;
//int dalidjear;//dalipar[4]=произв: это глобальная переменная !

//коэфф-ты преобр-я геометрии дали в линии верха и низа дали
int heotop,heobtm;//[5,6]=произв: в младшем байте дробная часть !

//баланс сцепления уровней const цвета с низом дали=0, с горизонтом=64
int dconbal;//[7]=0...64: (если нет фрактальных смещений цветов);
int dcon0;//[8]>=0: дополнительный сдвиг цвета при сцеплении с _горизонтом_ !
//баланс между сцеплением уровней цвета по dconbal (=0 для 100%) и чисто
int dfracbal;//[9]=0...64: фрактальными смещениями цветов (=64 для 100%);

int dheoseed,dcolseed;//[10,11]=произв: зёрна случайности фракталей

//фрактальные параметры геометрии и цветов дали:
int daliheo0,dalicol0;//[12,13]=произв: уровень нулевой гармоники
//диапазон [min,max] уровней 10 первых фрактальных гармоник
int *daliheo;//daliheo[0...19]->dalipar[14...33]=произв!
int *dalicol;//dalicol[0...19]->dalipar[34...53]=произв!
long x,shift,lcol,len,top,btm,l,dlen;
int len2,k,i,col,dh,ds,con,lenx,lenz;
byte far *daliram;

if ((newdali==NO)||(dalibitm<=ISNOT)) return;
newdali=NO;
//новый вариант дали

daliram=memory[dalibitm].rampntr;
dlen=memory[dalibitm].len;
lenx=memory[dalibitm].lenx;
lenz=memory[dalibitm].lenz;

//корректировка зашкала параметров
dalipar[dalivar][1]&=3;
for (i=7; i<=9; ++i)  if (dalipar[dalivar][i]<0)  dalipar[dalivar][i]=0;
for (i=7; i<=9; i+=2) if (dalipar[dalivar][i]>64) dalipar[dalivar][i]=64;

//параметры из текущего варианта dalipar[dalivar][]
dtranscol=dalipar[dalivar][0];
dcolmode= dalipar[dalivar][1];
dcolor0=  dalipar[dalivar][2];
dcolstep= dalipar[dalivar][3];
dalidjear=dalipar[dalivar][4];
heotop=   dalipar[dalivar][5];
heobtm=   dalipar[dalivar][6];
dconbal=  dalipar[dalivar][7];
dcon0=    dalipar[dalivar][8];
dfracbal= dalipar[dalivar][9];
dheoseed=dalipar[dalivar][10];
dcolseed=dalipar[dalivar][11];
daliheo0=dalipar[dalivar][12];
dalicol0=dalipar[dalivar][13];
daliheo=&dalipar[dalivar][14];
dalicol=&dalipar[dalivar][34];


if (dalipaint==YES)//красим _битмап_ дали
  {
  lcol=16*(long)dcolor0;
  for (l=0; l<dlen; ++l)
    {
    col=(int)(lcol>>8);
    if (col>256) break;
    if (dcolmode==1) col=raduga[0][col];
    else
    if (dcolmode==2) col=gray[col];
    else
    if (dcolmode==3) col=255;
    daliram[(word)l]=col;
    lcol+=dcolstep;
    }
  }

//готовим периметр дали

//вычисляем геометрию дали в данные dalistrs[]
Fractal(dalistrs,daliheo0,daliheo,dheoseed);

//преобразуем геометрию в данные dalistrl[], dalistrh[]
for (i=0; i<STRLEN; ++i)
  {
  len=dalistrs[i];
  btm=(len*heobtm)/256;
  top=(len*heotop)/256;
  if (btm<-128) btm=-128;
  if (btm> 127) btm= 127;
  len=top-btm;
  if (len<0)   len=0;
  if (len>255) len=255;

  dalistrl[i]=(int)len;
  dalistrh[i]=(int)btm;
  }

//теперь вычисляем сами данные dalistrs[]
Fractal(dalistrs,dalicol0,dalicol,dcolseed);

//корректируем данные dalistrs[] по двум балансам
len2=STRLEN/2;
for (i=0; i<STRLEN; ++i)
  {
  dh=dalistrh[i];
  ds=dalistrs[i];
  //баланс сцепления цветов с низом дали / с горизонтом
  con=(dconbal*(dcon0+dh))/64;
  //баланс между сцеплением / фрактальным смещением
  shift=con+(dfracbal*(ds-con))/64;
  if (dali2d==YES)//смещение в двумерной дали
    {
    //даль зеркально отражаем по горизонтали !
    //мода битмапа дали 2 !
    k= (i<len2)? i:(STRLEN-1-i);
    x=((lenx-1)*(long)k)/(len2-1);
    shift+=x*lenz;
    }
  dalistrs[i]=(word)shift;
  }
}

//формат буфера лабиринта: см файл vrs.ll !
int Labirint(int owner,d3poz shift,fix flenx,fix flenz,
             byte far *lbuf,int lbuflen,int indplus)
//из буфера литер lbuf[] длиной не более lbuflen байт декодируются
//параметры панелей и производится установка последних со сдвигом
//shift от оригина AP[owner], размеры _полной_ панели flenx,flenz;
//позиция и размеры панели определяются из lbuf, другие параметры
//каждой группы панелей берутся из массива структур labdata[LLBMS]!
//к номеру группы каждой панели добавляется величина indplus !
{
int cv,n,pan,b,c,d,e,f,g,x,bm,blx,blz,ind,m;
int sx,sy,sz,iang,imulx,imulz,heomode,sumx;
fix fx,fy,fz,flenx1,flenx2;
char nlab[20],a[4];
byte far *lab;
d3poz strpoz;
angle angx;

//высота целой панели (при imulz=4): flenz
//длина целой панели (при imulx=4) для углов:
// 90*(4*n+0)/4 градусов:            flenx
// 90*(4*n+1 или 3)/4 градусов:
flenx1=fixmul(flenx,iifixdiv(11180,10000));
// 90*(4*n+2)/4 градусов:
flenx2=fixmul(flenx,iifixdiv(14142,10000));

//все панели лабиринта с _начала_ lbuf[] и до конца
lab=lbuf;
while (1)
  {
  //пустая строка (lab[0]=CR, lab[1]=LF): конец лабиринта !
  if (lab[1]==ASCIILF) break;
  //копируем 16-байтный заголовок, содержащий 3 числа:
  strncpy(nlab,(char far *)lab,16);
  //первая литера-маркер заголовка строки
  m=nlab[0];
  //маркер комментария произвольной длины
  if (m=='/')
    {
    //пропускаем все литеры до конца строки
    for (; ; ++lab)
      {
      //превышена байтовая длина лабиринта
      if ((lab-lbuf)>=lbuflen) return ERR;
      if (lab[1]==ASCIILF) break;
      }
    lab+=2;
    continue;
    }
  //вытаскиваем дробные части из 65-ричных литер
  sx=let2i(nlab[4]);
  sy=let2i(nlab[9]);
  sz=let2i(nlab[14]);
  //ставим 0 на местах гекс- и разделительных литер (xyz)
  nlab[4]=nlab[9]=nlab[14]=nlab[5]=nlab[10]=nlab[15]=0;
  //прибавляем целые десятичные части
  sx+=16*atoi(nlab+1);
  sy+=16*atoi(nlab+6);
  sz+=16*atoi(nlab+11);
  //координаты оригина строки
  fx=fixmuldiv(itofix(sx),flenx,FIX16);
  fy=fixmuldiv(itofix(sy),flenx,FIX16);
  fz=fixmuldiv(itofix(sz),flenz,FIX16);
  strpoz=fixXYZgrad2D3poz(fx,fy,fz,0);
  //маркер для абсолютной позиции хелпа в системе AP_First
  if (m=='H') helppoz=strpoz;
  //маркер для абсолютной позиции оригина AP_First в системе земли
  if (m=='O')
    {
    AP[ADDR_APFirst].ear=ap1startpoz=strpoz;
    AP[ADDR_APFirst].earshift=YES;
    }
  //суммарное смещение по x оригина панели от оригина строки
  sumx=0;
  lab+=16;
  for (x=0; ; ++x)
    {
    //превышена байтовая длина лабиринта
    if ((lab-lbuf)>=lbuflen) return ERR;
    //конец текущей строки лабиринта
    if (lab[1]==ASCIILF) { lab+=2; break; }
    //после заголовка шестёрки литер 'abcde;' дают параметры панели:
    a[0]=lab[0];
    a[1]=lab[1];
    a[2]=0;
    b=lab[2];
    c=lab[3];
    d=lab[4];
    e=lab[5];
    //это всегда декодируем!
    f=lab[6];
    g=lab[7];
    //смещение панели от предыдущей
    if (f==ASCII32) f=16;//значение по умолчанию
    else            f=let2i(f);
    //смещение накапливаем с учётом знака
    sumx+= (g=='-')? -f:f;

    //нет панели, если в начале два пробела:
    if ((a[0]==ASCII32)&&(a[1]==ASCII32)) goto endx;
    //else извлекаем номер группы панели
    ind=atoi(a)+indplus;
    if (ind<0)      ind=0;
    if (ind>=LLBMS) ind=LLBMS-1;
    //номер угла панели
    if (b==ASCII32) iang=0;//значение по умолчанию
    else            iang=15&let2i(b);
    //умножители высоты и длины панели
    if (c==ASCII32) imulz=4;//значение по умолчанию
    else            imulz=let2i(c);
    if (d==ASCII32) imulx=4;//значение по умолчанию
    else            imulx=let2i(d);
    //мода геометрии панели
    if (e==ASCII32) heomode=FULPAN;//значение по умолчанию
    else            heomode=15&let2i(e);

    //нет панели с нулевой высотой, длиной или модой геометрии
    if ((imulz==0)||(imulx==0)||(heomode==0)) goto endx;

    pan=GetPAN(owner);
    if (pan<=ISNOT) return ERR;//исчерпано количество панелей
    n=labdata[ind].sysbms;//номер системного битмапа/конверта
    n=max(n,0); n=min(n,SYSBMS-1);
    bm=sysbms[n];
    cv=syscvs[n];
    SetPanelRes(pan,bm,cv);
    LockPAN(pan);

    //размеры панели
    panel[pan].flenz=(imulz*flenz)/4;
    switch(iang%4)
      {
      case 0: angx=grad2ang(90*(iang/4));
        panel[pan].flenx=(imulx*flenx)/4;
        break;
      case 1: angx=grad2ang(90*(iang/4))+4836;//arctg(1/2)=26.5650 гр.
        panel[pan].flenx=(imulx*flenx1)/4;
        break;
      case 3: angx=grad2ang(90*(iang/4))+(16384-4836);//90гр.-arctg(1/2)
        panel[pan].flenx=(imulx*flenx1)/4;
        break;
      case 2: angx=grad2ang(45*(iang/2));
        panel[pan].flenx=(imulx*flenx2)/4;
        break;
      }
    //округляем угол !
    angx=ANGSTEP*(angx/ANGSTEP);
    //текстура битмапа повторяется по x imulx/4 и по z imulz/4 раз;
    //если imulz>4, то при рендере будет зашкал развертки за конец
    //его текстуры, поэтому передвигаем правый конец битмапа влево
    //по x на (imulz-4)/4 пикселей
    blx=memory[bm].lenx;
    blx-=(imulz-4)/4;
    //а вот это решает остаточные проблемы...
    if ((imulz>4)&&((imulz%4)!=0)) --blx;
    if (blx<=0) blx=1;
    panel[pan].xb1=blx-1;
    //для качественного симметричного скроллинга поле эффективной
    //длины битмапа панели также уменьшаем на количество повторов
    //битмапа по вертикали
    panel[pan].rendlenx=blx;
    panel[pan].fmulx=fixdiv(flenx,itofix(blx));
    //по z ничего не меняем
    blz=memory[bm].lenz;
    if (blz<=0) blz=1;
    panel[pan].fmulz=fixdiv(flenz,itofix(blz));

    //остальные параметры панели:
    //часть их меняем с учётом ручных регуляторов лабиринта !
    SetPanelPar(pan,heomode,
      labdata[ind].glass^labglass,
      labdata[ind].gradcon+labgradcon,
      labdata[ind].gradampx+labgradampx,
      labdata[ind].gradnum2,//это пока не регулир-ся...
      labdata[ind].scrollx,//+labscrollx делается в ScrollLab() !
      labdata[ind].rfluct+labrfluct,
      labdata[ind].sides);

    //положение панели внутри лабиринта
    panel[pan].ap=strpoz;
    panel[pan].ap.x0+=fixmuldiv(itofix(sumx),flenx,FIX16);
    panel[pan].ap.angx+=angx;
    //сдвиг панели с учетом сдвига лабиринта
    panel[pan].ap=ConvD3pozFromAP2Earth(panel[pan].ap,shift);
endx://на одну панель уходит 8 литер лабиринта
    lab+=8;
    }
  }
return OK;
}

void SRPBpixel(long *seed,
               int bm,int x0,int z0,int x1,int z1,int *col,int i0,int i1)
//вариант RPBpixel() с внешним seed
{
int x,z,lenx,lenz,maxx,maxz,dind1;
byte far *ram;

if ((bm<=ISNOT)||(x0>x1)||(z0>z1)) return;
lenx=memory[bm].lenx; maxx=lenx-1;
lenz=memory[bm].lenz; maxz=lenz-1;
if ((x0>maxx)||(x1<0)) return;
if ((z0>maxz)||(z1<0)) return;
x0=max(x0,0); x1=min(x1,maxx);
z0=max(z0,0); z1=min(z1,maxz);

dind1=i1-i0+1;
if (dind1<1) dind1=1;

if (memory[bm].rammode!=2) ConvertMEMmode(bm,2);
//мода битмапа 2 !
ram=memory[bm].rampntr+x0*lenz;
for (x=x0; x<=x1; ++x)
  {
  for (z=z0; z<=z1; ++z)
    {
    //встроенный случайный датчик для скорости!
    *seed=RANDUMUL*(*seed)+RANDUADD;
    ram[z]=col[i0+((int)(0x7FFF&((*seed)>>16)))%dind1];
    }
  ram+=lenz;
  }
}

void VGAechoSwitch(void)
//ЛилаМайя (эхо-зеркало на стене) !
{
static int sw;
int n;
sw^=1;
//меняем все системные битмапы стены wallbitm<->VGAMEM
for (n=0; n<=8; ++n) sysbms[n+35]= (sw==0)? wallbitm:VGAMEM;
}

void AP_Render(void)
{
int msgrd,msgwr;
int kbcod,hit;
static int addr_AP=ADDR_ALL;
static int backgroundvar;//вариант вызова программ фона
int pan,i,var;

//просматриваем весь буфер сообщений
msgrd=msgread;
msgwr=msgwrite;
while (msgrd!=msgwr)//сканируем весь буфер сообщений
  {
  //если сообщение не 'для всех' и не 'для этой AP', то пропуск его
  if ( !((msg[msgrd].dstaddr==ADDR_ALL)||(msg[msgrd].dstaddr==addr_AP)) )
    goto nextmsg;

  //расшифровываем сообщение
  switch (msg[msgrd].cmd)
    {
    default: break;
    case MSG_AP_SET_ADDR://принять адрес AP
      addr_AP=GetIntMSG(msgrd);
      break;
    case MSG_AP_CSTART://начать работу AP 'с нуля'
      //нужная картинка фона
      backgroundvar=0;
      spaceflag=OFF;
      AP[addr_AP].state=AP_WORK;
      break;
    case MSG_AP_END://закончить работу AP
      //если состояние AP итак нерабочее, то пропуск сообщения
      if (AP[addr_AP].state==AP_START) break;
      AP[addr_AP].state=AP_START;
      printf("Full Stop Render !\n");
      break;
    case MSG_AP_STOP://приостановить работу AP
      //если состояние AP не было рабочее, то пропуск сообщения
      if (AP[addr_AP].state!=AP_WORK) break;
      AP[addr_AP].state=AP_STOP;
      printf("Stop Render !\n");
      break;

    case MSG_AP_INPUT_OFF: break;
    case MSG_AP_INPUT_ON://выбор [панели] AP
      break;

    case MSG_AP_WSTART://продолжить работу AP
      //если AP не была приостановлена, то пропуск сообщения
      if (AP[addr_AP].state!=AP_STOP) break;
      AP[addr_AP].state=AP_WORK;
      break;

    case MSG_KBPC_DATA:
      if (AP[addr_AP].state!=AP_WORK) break;
      hit=msg[msgrd].data[0];
      kbcod=msg[msgrd].data[1];

      switch(kbcod)
        {
        case KeyCodTab:
        case KeyCodUp:
        case KeyCodDown:
        case KeyCodLeft:
        case KeyCodRight:
          if (Control==OFF)//переадресуем хоткеи в AP_OutVGAgra
            msg[msgrd].dstaddr=ADDR_APOVGAg;
          else//ON, не переадресуем
            {
            if (hit==KEYON)
              switch(kbcod)
                {
                //меняем абсолютный цвет неба
                case KeyCodUp:   NeboColor(0,0,0,0,1); break;
                case KeyCodDown: NeboColor(0,0,0,0,-1); break;
                //меняем скорость вращения дали
                case KeyCodLeft: --dalidadt; break;
                case KeyCodRight: ++dalidadt; break;
                }
            else//KEYOFF
            if (kbcod==KeyCodTab) simsimflag^=1;//симметричный рендер по x
            }
          goto nextmsg;
        }

      if (hit==KEYON)
        switch(kbcod)
          {
          //размах флуктуаций пикселей
          case KeyCodV: ++randfluct; break;
          case KeyCodC: --randfluct; break;

          //скорость скроллинга
          case KeyCodF: ++allscrollx; break;
          case KeyCodD: --allscrollx; break;

          // +- _номер_ текущего параметра дали
          case KeyCodAdd6:
            if (++dnumpar>=DALIPAR) dnumpar=DALIPAR-1;
            printf("dnumpar=%i\n",dnumpar);
            break;
          case KeyCodAdd4:
            if (--dnumpar<0) dnumpar=0;
            printf("dnumpar=%i\n",dnumpar);
            break;
          // +- текущий _параметр_ дали
          case KeyCodAdd5:
            if (Shift==OFF) ++dalipar[dalivar][dnumpar];
            else
              {
              dalipar[dalivar][dnumpar]+=16;
              printf("%5i",dalipar[dalivar][dnumpar]);
              }
            newdali=YES; break;
          case KeyCodAdd2:
            if (Shift==OFF) --dalipar[dalivar][dnumpar];
            else
              {
              dalipar[dalivar][dnumpar]-=16;
              printf("%5i",dalipar[dalivar][dnumpar]);
              }
            newdali=YES; break;

          case KeyCodPoint://это > для увеличения экрана
            if (++penbmpsize>5) penbmpsize=5; PEN.usershift=YES; break;
          case KeyCodComma://это < для уменьшения экрана
            if (--penbmpsize<0) penbmpsize=0; PEN.usershift=YES; break;

          //rgb и яркость цвета 1 земли или цвета неба
          case KeyCodDel:
          case KeyCodIns:
          case KeyCodEnd:
          case KeyCodHome:
          case KeyCodPgDn:
          case KeyCodPgUp:
          case KeyCodAdd7:
          case KeyCodAdd9:
            if (Control==OFF)
              {
              switch(kbcod)
                {
                //rgb цвета 1
                case KeyCodDel: --bgpar[bgvar][0]; newbgpar=YES; break;
                case KeyCodIns: ++bgpar[bgvar][0]; newbgpar=YES; break;
                case KeyCodEnd:  --bgpar[bgvar][1]; newbgpar=YES; break;
                case KeyCodHome: ++bgpar[bgvar][1]; newbgpar=YES; break;
                case KeyCodPgDn:  --bgpar[bgvar][2]; newbgpar=YES; break;
                case KeyCodPgUp:  ++bgpar[bgvar][2]; newbgpar=YES; break;
                //яркость цвета 1
                case KeyCodAdd7: --bgpar[bgvar][7]; newbgpar=YES; break;
                case KeyCodAdd9: ++bgpar[bgvar][7]; newbgpar=YES; break;
                }
              }
            else//Control=ON
              {
              switch(kbcod)
                {
                //rgb цвета неба
                case KeyCodDel: NeboColor(-1,0,0,0,0); break;
                case KeyCodIns: NeboColor(1,0,0,0,0); break;
                case KeyCodEnd:  NeboColor(0,-1,0,0,0); break;
                case KeyCodHome: NeboColor(0,1,0,0,0); break;
                case KeyCodPgDn: NeboColor(0,0,-1,0,0); break;
                case KeyCodPgUp: NeboColor(0,0,1,0,0); break;
                //яркость цвета неба
                case KeyCodAdd7: NeboColor(0,0,0,-1,0); break;
                case KeyCodAdd9: NeboColor(0,0,0,1,0); break;
                }
              }
            break;
          }
      if (hit==KEYOFF)
        switch(kbcod)
          {
          //разрешение флуктуаций пикселей
          case KeyCodX: fluctflag^=1; break;//по x
          case KeyCodZ: fluctflag^=2; break;//по z
          case KeyCodB: randfluct=0; break;//размах флуктуаций=0

          case KeyCodS: scrollflag^=1; break;//разрешение скроллинга
          case KeyCodG: allscrollx=0; break;//скорость скроллинга=0

          case KeyCodF10: DeChoiceAP(); break;//отмена выбора AP

          case KeyCodAdd1: dalidadt=0; break;//стоп движения дали
          case KeyCodAdd3: dalinali^=1; break;//даль есть или нет

          case KeyCodEnter://печать текущего параметра варианта дали
            printf("dali[var=%1i][par=%2i]=%i\n",
              dalivar,dnumpar,dalipar[dalivar][dnumpar]); WKP(); break;

          case KeyCodP://печать параметров варианта цветов земли и неба
            for (i=0; i<8; ++i) printf("%3i ",bgpar[bgvar][i]);
            printf("\nbgvar=%i   skyvar=%i  skycolor=%i\n",
                      bgvar,   nebocoli,  nebocol[nebocoli]); WKP(); break;

          case KeyCod0://быстрое переключение космоса/земли
            spaceflag^=1;
            if (spaceflag==0)//земля
              {
              starvar=2;
              dalinali=YES;
              nebocoli=3;
              }
            else//космос
              {
              starvar=3;
              dalinali=NO;
              nebocoli=0;
              }
            break;

          //номер варианта земли, неба и дали
          case KeyCod1: case KeyCod4: case KeyCod7:
          case KeyCod2: case KeyCod5: case KeyCod8:
          case KeyCod3: case KeyCod6: case KeyCod9:
            var=kbcod-KeyCod1;
            if (Alter==ON)   Dalivar(var);//вариант дали
            else
            if (Control==ON) nebocoli=var;//вариант цвета неба
            else
            if (Shift==OFF) { bgvar=var; newbgpar=YES; }//вариант цветов земли bgpar
            break;

          //rgb-множители шага по цвету
          case KeyCodNL:     ++bgpar[bgvar][3]; newbgpar=YES; break;
          case KeyCodAddDiv: ++bgpar[bgvar][4]; newbgpar=YES; break;
          case KeyCodAddMul: ++bgpar[bgvar][5]; newbgpar=YES; break;
          //шаг по цвету от i к i+1, для цветов i=1...7
          case KeyCodAddMinus: --bgpar[bgvar][6]; newbgpar=YES; break;
          case KeyCodAddPlus:  ++bgpar[bgvar][6]; newbgpar=YES; break;
          //средняя (50%) яркость цвета 1
          case KeyCodAdd8: bgpar[bgvar][7]=32; newbgpar=YES; break;

          //космос или нет
          case KeyCodAddEnter: spaceflag^=1; break;

          //вариант звезд
          case KeyCodAdd0: if (++starvar>=STARVAR) starvar=STARVAR-1; break;
          case KeyCodAddDel: if (--starvar<0) starvar=0; break;

          //вариант программ рендера фона
          case KeyCodDiv: ++backgroundvar; break;

          }
      break;
    }

nextmsg:
  //текущее сообщение ОТ ДАННОЙ AP к кому угодно
  if (msg[msgrd].srcaddr==addr_AP)
    //стираем по завершению 1-го оборота
    { if (msg[msgrd].count>0) msg[msgrd].cmd=0; }
  else//НЕ ОТ ДАННОЙ AP К ДАННОЙ AP стираем сейчас
    if (msg[msgrd].dstaddr==addr_AP) msg[msgrd].cmd=0;

  //переход к следующему сообщению
  if (++msgrd>=MSGBUF) msgrd=0;
  }

if (AP[addr_AP].state==AP_WORK)//если состояние AP 'рабочее'
  {
  angle za;

  //текущие размеры PEN-bmp
  numpart=1;
  switch(penbmpsize)
    {
    default:
    case 0: penbmplenx=80;  penbmplenz=50; break;
    case 1: penbmplenx=120; penbmplenz=70; break;
    case 2: penbmplenx=160; penbmplenz=100; break;
    //размер экрана 3: вертикальная псевдо-стереопара!!!
    //горизонтальный обзор более 90 градусов!
    case 3: penbmplenx=320; penbmplenz=100; break;
    //размер экрана 4: горизонтальная псевдо-стереопара!
    case 4: penbmplenx=160; penbmplenz=200; break;
    //двухшаговый режим подготовки полного экрана!
    case 5: penbmplenx=320; penbmplenz=200; numpart=2; break;
    }
  //одинаковый угол обзора экрана!
  val_h=penbmplenz;
  /* если val_h регулируется вручную:
  //защита от выхода val_h за границы, иначе фатально !
  if (val_h<=0) val_h=1;
  if (val_h>max_val_h) val_h=max_val_h;
  */
  //'текущая lenx' с учетом многошагового рендера
  partpenbmplenx=penbmplenx/numpart;//должна быть всегда кратна 4 !
  //текущая длина буфера цвета
  PEN.ramlen=partpenbmplenx*penbmplenz;
  //размеры PEN
  PEN.lenx=itofix(penbmplenx);
  PEN.lenz=penbmplenz*penbmpfmulz;

  //положение PEN-панели согласно размерам и скосу взгляда
  if (PEN.zgrad> ZGRADMAX) PEN.zgrad= ZGRADMAX;
  if (PEN.zgrad<-ZGRADMAX) PEN.zgrad=-ZGRADMAX;
  za=grad2ang(PEN.zgrad);//текущий угол скоса взгляда
  PEN.poz.y0=itofix(val_h);
  PEN.poz.x0=-PEN.lenx/2;
  PEN.poz.z0=-PEN.lenz/2+fixdiv(val_h*fsinus(za),fcosin(za));
  penpozx0=fixtoi(PEN.poz.x0);
  penpozz0=fixtoi(PEN.poz.z0);
  penpanlenz=fixtoi(PEN.lenz);
  jear=fixtoi(-penpozz0*penbmpfmulz1);//строка горизонта на экране
  //строка близкой к юзеру земли на экране
  jearuser=fixtoi((-fixtoi(PEN.ear.z0)-penpozz0)*penbmpfmulz1);

  //длина гориз. сдвига по PEN-bmp, эквивалентная углу 360 град.
  horshift360=(int)((31416L*(long)val_h)/5000L);
  //угол оси зрения юзера в системе земли = PEN.ear.angx,
  //гориз-й сдвиг по PEN-bmp из-за вращения юзера:
  //horshift=val_h*3.1416*PEN.ear.angx/ANG180G
  horshift=fixtoi(fixmul(itofix(val_h),
                         fixmul(iifixdiv(31416,10000),
                                iifixdiv(PEN.ear.angx/2,ANG90G))));
  //PEN.ear.angx=0...65535, что вне диапазона fix=+-32767
  //(и ANG180G), поэтому используется пол-угла !
  //сдвиг пикселей на экране: угол растет против часов по модулю 360
  horshift=(horshift360-horshift)%horshift360;

  if ((penbmpsize<3)||(penbmpsize>4))//монокадр
    {
    //открепляем блок PEN.tmpmem после стереокадра
    UnLockMEM(PEN.tmpmem); PEN.tmpmem=ISNOT;
    }
  else//стереокадр
    {
    //выделяем промежуточную память половинки стереокадра
    if (PEN.tmpmem<=ISNOT)//если только ее еще нет!
      {
      printwarn=OFF; PEN.tmpmem=GetLockMEM(addr_AP,PEN.ramlen);
      printwarn=ON;
      if (PEN.tmpmem<=ISNOT) printf("PEN.tmpmem: errmem=%i\n",errmem);
      }
    }

  //выделяем и закрепляем y-память, по 2 байта на 1 пиксель !
  printwarn=OFF;
  PEN.ymem=GetLockMEM(addr_AP,2*PEN.ramlen);
  printwarn=ON;
  if (PEN.ymem<=ISNOT) { printf("PEN.ymem: errmem=%i\n",errmem); return; }

  //находим координаты всех видимых панелей в USER-системе
  ComputPanelsPosition();

  //цикл по номеру текущего шага рендера
  for (actpart=0; actpart<numpart; ++actpart)
    {
    //закраска PEN-панели фоновой картинкой выбранного типа
    switch(backgroundvar)
      {
      case 0://штатный фон
        //новые параметры битмапов земли, дали
        PaintZbitm();
        PaintDbitm();
        if (spaceflag==ON)//если космос
          {
          RenderNebo();//рендер неба
          //вместо земли - небо
          RenderD();//рендер дали
          }
        else//не космос
          {
          RenderNebo();//рендер неба
          RenderStars();//рендер звезд
          RenderZ();//рендер земли
          RenderD();//рендер дали
          }
        break;

      case 1: break;//нет стирания фона

      case 2: SetRAM(penram,PEN.ramlen,COLTRANS,1); break;//черный фон

      //вариант только для уст-ки белой рамки
      case 3: SetBorderColor(gray[15]); ++backgroundvar; break;
      case 4: SetRAM(penram,PEN.ramlen,gray[8],1); break;//серый фон

      //вариант для уст-ки черной рамки
      case 5: SetBorderColor(COLTRANS); break;
      }
    backgroundvar%=5;//перебор вариантов 0...4!
    //заполнение y-буфера const y-профилем фона
    SetYbuf(YBG);

    //рендер нестеклянных панелей
    for (pan=0; pan<=maxpannum; ++pan)
      {
      //пропускаем отсутствующие и полностью невидимые панели
      if (panel[pan].flag!=YES) continue;
      if (panel[pan].render!=YES) continue;
      //пропускаем стеклянные панели для верного затенения!
      if (panel[pan].glass==YES) continue;
      RenderPanel(pan);
      }
    //рендер всех стеклянных панелей
    RenderSteklPanels();
    //определяем параметры клиппинга
    if (clip==YES) ClipComput();

    //конец первого шага двухшагового рендера
    if ((numpart==2)&&(actpart==0))//запоминаем отрендеренный кусок экрана
      {
      byte far *tmpram;

      //выделяем промежуточную память полукадра
      if (PEN.tmpmem<=ISNOT)//если только ее еще нет!
        {
        printwarn=OFF;
        PEN.tmpmem=GetLockMEM(addr_AP,PEN.ramlen);
        printwarn=ON;
        if (PEN.tmpmem<=ISNOT)
          { printf("PEN.tmpmem: errmem=%i\n",errmem); break; }
        }

      tmpram=memory[PEN.tmpmem].rampntr;
      CopyRAM(tmpram,penram,PEN.ramlen);
      }
    }

  //открепляем y-память отовсюду
  UnLockMEM(PEN.ymem);
  }
}

void AP_OutVGAgra(void)
//вывод PEN-панели на физический монитор VGA в графмоде 320x200
  //надо сделать возможным вывод ЛЮБОЙ панели !!!
{
static int addr_AP=ADDR_ALL;
int msgrd,msgwr;
int kbcod,hit;
int hlen,parthlen,vlen,j,j1,ie0,je0;
//флаг корректировки и физическая цветность системной палитры
static int physpalcorrflag=NO,zerkalo=OFF;
static fix colness;
byte far *tmpram;

//просматриваем весь буфер сообщений
msgrd=msgread;
msgwr=msgwrite;
while (msgrd!=msgwr)//сканируем весь буфер сообщений
  {
  //если сообщение не 'для всех' и не 'для этой AP', то пропуск его
  if ( !((msg[msgrd].dstaddr==ADDR_ALL)||(msg[msgrd].dstaddr==addr_AP)) )
    goto nextmsg;

  //расшифровываем сообщение
  switch (msg[msgrd].cmd)
    {
    default: break;
    case MSG_AP_SET_ADDR://принять адрес AP
      addr_AP=GetIntMSG(msgrd);
      break;
    case MSG_AP_CSTART://начать работу AP 'с нуля'
      if (actscrnmode!=0x13)//экран не в графическом режиме
        {
        //графический VGA режим 320x200 с 256-ю цветами
        SetScreenMode(0x13);
        //затираем графэкран
        SetRAM(MK_FP(0xA000,0),64000u,COLTRANS,1);
        SetBorderColor(COLTRANS);
        //установка физической палитры экрана
        SetPalette(syspal,0,255);//все цвета !!!
        //для стартовой корр-ки физпалитры с повышением цветности
        physpalcorrflag=YES;
        colness=FIX1+FIX05;//цветность = 1.5 улучшает все и не искажает
          //даже наиболее чувствительные картинки...
        }
      else//уже графрежим
        SetRAM(MK_FP(0xA000,0),64000u,COLTRANS,1);
      AP[addr_AP].state=AP_WORK;
      break;
    case MSG_AP_WSTART://продолжить работу AP
      //если AP не была приостановлена, то пропуск сообщения
      if (AP[addr_AP].state!=AP_STOP) break;
      AP[addr_AP].state=AP_WORK;
      break;
    case MSG_AP_END://закончить работу AP
      //если состояние AP итак нерабочее, то пропуск сообщения
      if (AP[addr_AP].state==AP_START) break;
      //удаляем все рабочие ресурсы AP:
      //...
      AP[addr_AP].state=AP_START;
      printf("Full Stop VGA !\n");
      break;
    case MSG_AP_STOP://приостановить работу AP
      //если состояние AP не было рабочее, то пропуск сообщения
      if (AP[addr_AP].state!=AP_WORK) break;
      AP[addr_AP].state=AP_STOP;
      printf("Stop VGA !\n");
      break;
    case MSG_KBPC_DATA:
      if (AP[addr_AP].state!=AP_WORK) break;
      hit=msg[msgrd].data[0];
      kbcod=msg[msgrd].data[1];
      if (hit==KEYON)
        switch(kbcod)
          {
          //регулировка физической цветности палитры системы
          case KeyCodRight://увеличение на 0.5
            colness+=FIX05; physpalcorrflag=YES; break;
          case KeyCodLeft://уменьшение на 0.5
            colness-=FIX05; physpalcorrflag=YES; break;
          }
      else//KEYOFF
        switch(kbcod)
          {
          case KeyCodTab: zerkalo^=1; break;
          case KeyCodDown://цветность 0
            colness=0; physpalcorrflag=YES; break;
          case KeyCodUp://стандартная цветность 1.5
            colness=FIX1+FIX05; physpalcorrflag=YES; break;
          }
      break;
    }

nextmsg:
  //текущее сообщение ОТ ДАННОЙ AP к кому угодно
  if (msg[msgrd].srcaddr==addr_AP)
    //стираем по завершению 1-го оборота
    { if (msg[msgrd].count>0) msg[msgrd].cmd=0; }
  else//НЕ ОТ ДАННОЙ AP К ДАННОЙ AP стираем сейчас
    if (msg[msgrd].dstaddr==addr_AP) msg[msgrd].cmd=0;

  //переход к следующему сообщению
  if (++msgrd>=MSGBUF) msgrd=0;
  }

if (AP[addr_AP].state==AP_WORK)//если состояние AP 'рабочее'
  {
  //размеры картинки на экране
  vlen=penbmplenz;
  hlen=penbmplenx;
  parthlen=partpenbmplenx;

  //вывод картинки на физический экран дисплея

  //левый нижний край картинки на экране, считая je сверху вниз
  ie0=160-hlen/2;
  ie0=4*(ie0/4);//по границам dword!
  if (ie0<0) ie0=0;
  je0=100+vlen/2;
  if (je0>=200) je0=199;
  switch(PEN.rammode)
    {
    default:
    case 0://вывод слева вправо, потом вверх
    if (logovisible==YES) LilaPaint();//рисуем лейбл Lila
    switch(numpart)
      {
      default:
      case 1://одношаговый рендер
        if ((penbmpsize<3)||(penbmpsize>4))//монокадр
          {
          if (zerkalo==OFF)
            {
            for (j=0; j<vlen; ++j)
              CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),penram+hlen*j,hlen);
            }
          else//зеркальное отражение нижней половины экрана в верхнюю
            {
            for (j=0; j<vlen/2; ++j)
              CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),penram+hlen*j,hlen);
            for (j=vlen-1; j>=vlen/2; --j)
              {
              j1=vlen-1-j;
              CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),penram+hlen*j1,hlen);
              }
            }
          }
        else//стереокадр
          {
          if (PEN.tmpmem>ISNOT) tmpram=memory[PEN.tmpmem].rampntr;

          if (penbmpsize==3)//вертикальная псевдо-стереопара
            {
            //наверх предыдущий кадр из блока PEN.tmpmem
            ie0=0;
            je0=99;
            if (PEN.tmpmem>ISNOT)
              for (j=0; j<vlen; ++j)
                CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),tmpram+hlen*j,hlen);
            //вниз текущий кадр из penram
            ie0=0;
            je0=199;
            for (j=0; j<vlen; ++j)
              CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),penram+hlen*j,hlen);
            }

          if (penbmpsize==4)//горизонтальная псевдо-стереопара
            {
            //слева предыдущий кадр из блока PEN.tmpmem
            ie0=0;
            je0=199;
            if (PEN.tmpmem>ISNOT)
              for (j=0; j<vlen; ++j)
                CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),tmpram+hlen*j,hlen);
            //справа текущий кадр из penram
            ie0=160;
            je0=199;
            for (j=0; j<vlen; ++j)
              CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),penram+hlen*j,hlen);
            }

          //копируем penram в блок PEN.tmpmem для следующего раза
          if (PEN.tmpmem>ISNOT) CopyRAM(tmpram,penram,PEN.ramlen);
          }
        break;

      case 2://двухшаговый рендер
        if (PEN.tmpmem>ISNOT) tmpram=memory[PEN.tmpmem].rampntr;
        else//сбойная ситуация- нет левого полукадра
          tmpram=penram;//просто повторяем правый...

        if (zerkalo==OFF)
          {
          for (j=0; j<vlen; ++j)//строка из 2-х половинок!
            {
            //сначала левая половина строки, из первого шага рендера
            CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),tmpram+parthlen*j,parthlen);
            //потом правая половина
            CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0+parthlen),penram+parthlen*j,parthlen);
            }
          }
        else//зеркальное отражение нижней половины экрана в верхнюю
          {
          for (j=0; j<vlen/2; ++j)
            {
            CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),tmpram+parthlen*j,parthlen);
            CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0+parthlen),penram+parthlen*j,parthlen);
            }
          for (j=vlen-1; j>=vlen/2; --j)
            {
            j1=vlen-1-j;
            CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),tmpram+parthlen*j1,parthlen);
            CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0+parthlen),penram+parthlen*j1,parthlen);
            }
          }

        //только здесь открепляем блок
        UnLockMEM(PEN.tmpmem); PEN.tmpmem=ISNOT;
        break;
      }
      break;
    }

  //если нужна корректировка физпалитры
  if (physpalcorrflag==YES)
    {
    static int first=YES;
    byte pal[PAL768];
    ConvertPalette(syspal,pal,colness,0);
    SetPalette(pal,0,255);
    physpalcorrflag=NO;
    //в самый первый раз восстанавливаем цветность цветов 1...8
    if (first==YES) { SetPalette(syspal,1,8); first=NO; }
    }
  }
}

void SetPanelRes(int pan,int bmap,int conv)
//устанавливает ресурсы панели: текстуру bmap и конверт conv,
//которые могут быть и с аварийным номером ISNOT !
//также устанавливает рекомендуемые множители размеров панели
//из блока текстуры для круглости круга...
{
if (pan<=ISNOT) return;//не трогаем аварийную панель!

panel[pan].memtex=bmap;
panel[pan].memconv=conv;

//максимальный диапазон развертки битмапа панели
panel[pan].xb0=0;
panel[pan].zb0=0;
panel[pan].xb1=memory[bmap].lenx-1;
panel[pan].zb1=memory[bmap].lenz-1;
panel[pan].rendlenx=memory[bmap].lenx;
//потом можно скорректировать эти пар-ры...

//круглость круга согласно рекомендуемым множителям из битмапа:
//метод растяжения панели по вертикали Z 1/1 -> 6/5
panel[pan].fmulz=fixmul(panel[pan].fmulz,memory[bmap].fmulz);

//mulx/mulz панели для корректировки ее размеров относительно
//битмапа должны устанавливаться перед вызовом данной функции !
SetPanelSize2Bmap(pan);//размеры панели с охватом всего битмапа
//а после этого можно изменить размеры панели...
}

void SetPanelSize2Bmap(int pan)
//уст-ка размеров панели с охватом всего её битмапа 1:1
//фактически это внутренняя функция SetPanelRes()...
{
int bmap;
bmap=panel[pan].memtex;
panel[pan].flenx=panel[pan].fmulx*memory[bmap].lenx;
panel[pan].flenz=panel[pan].fmulz*memory[bmap].lenz;
}

void FullPanelBmap(int pan)
//натягивание текстуры впритык к краям панели
{
int bmap,lenx,lenz;
bmap=panel[pan].memtex;
lenx=memory[bmap].lenx;
lenz=memory[bmap].lenz;
if (lenx<=0) lenx=1;
if (lenz<=0) lenz=1;
panel[pan].fmulx=panel[pan].flenx/lenx;
panel[pan].fmulz=panel[pan].flenz/lenz;
}

void Stolb(int owner,d3poz poz,int lenx,int lenz,int bmind,int grad,int sides)
//квадратный столб из 4-х панелей с индексом системной текстуры
//bmind и градациями grad от номинала, размерами lenx,lenz см,
//низ/центр которого расположен в poz, с флагом obor видимости
//панелей столба изнутри;
{
int pan,i,glass,bm,cv;
d3poz shift;
fix fx2,fy2;
angle pangx;

bm=sysbms[bmind];
cv=syscvs[bmind];
glass= (bm==glassbitm)? YES:NO;

pangx=poz.angx;
for (i=0; i<4; ++i)
  {
  pan=GetPAN(owner);
  if (pan<=ISNOT) return;
  SetPanelRes(pan,bm,cv);
  LockPAN(pan);
  SetPanLenxz(pan,lenx,lenz);
  FullPanelBmap(pan);
  SetPanelPar(pan,FULPAN,glass,grad,0,0,0,0,sides);
  fy2=fx2=panel[pan].flenx/2;
  poz.angx=pangx+ANG90G*i;
  shift=fixXYZgrad2D3poz(-fx2,-fy2,0,0);
  panel[pan].ap=ShiftD3poz(shift,poz);
  }
}

void SetPanLenxz(int pan,int lenx,int lenz)
//устанавливает размеры панели в см
{
panel[pan].flenx=itofix(lenx);
panel[pan].flenz=itofix(lenz);
}

void SetPanFlenxz(int pan,fix flenx,fix flenz)
//устанавливает fix-размеры панели
{
panel[pan].flenx=flenx;
panel[pan].flenz=flenz;
}

void USERearXYZgrad(int x,int y,int z,int grad)
//помещение наблюдателя (оригина USER-системы) в нужную точку
//земли xyz и поворот на угол a в градусах;
{
PEN.ear=XYZgrad2D3poz(x,y,z,grad);
PEN.usershift=YES;
}

void USERearfixXYZgrad(fix x,fix y,fix z,int grad)
{
PEN.ear=fixXYZgrad2D3poz(x,y,z,grad);
PEN.usershift=YES;
}

void CopyPAN(int dst,int src)
//копирует параметры src панели в dst _без_ закрепления !!!
{
if (dst<=ISNOT) return;
panel[dst].flag=  panel[src].flag;
panel[dst].sides= panel[src].sides;
panel[dst].render=panel[src].render;
//овнер такой же, иначе координаты будут неверны!!!
panel[dst].owner= panel[src].owner;
//count не трогаем: после копирования надо однократно закреплять копию!!!
panel[dst].ap=    panel[src].ap;
panel[dst].ear=   panel[src].ear;
panel[dst].pen=   panel[src].pen;
panel[dst].xbelft=panel[src].xbelft;
panel[dst].xbergt=panel[src].xbergt;
panel[dst].earvalid=panel[src].earvalid;
panel[dst].penvalid=panel[src].penvalid;
panel[dst].xbevalid=panel[src].xbevalid;
panel[dst].flenx= panel[src].flenx;
panel[dst].flenz= panel[src].flenz;
panel[dst].memtex=panel[src].memtex;
panel[dst].memconv=panel[src].memconv;
panel[dst].fmulx= panel[src].fmulx;
panel[dst].fmulz= panel[src].fmulz;
panel[dst].xb0=   panel[src].xb0;
panel[dst].xb1=   panel[src].xb1;
panel[dst].zb0=   panel[src].zb0;
panel[dst].zb1=   panel[src].zb1;
panel[dst].gradcon =panel[src].gradcon;
panel[dst].glass   =panel[src].glass;
panel[dst].rfluct  =panel[src].rfluct;
panel[dst].rendlenx=panel[src].rendlenx;
panel[dst].scrollx =panel[src].scrollx;
panel[dst].gradampx=panel[src].gradampx;
panel[dst].gradlenx=panel[src].gradlenx;
panel[dst].heomode =panel[src].heomode;
panel[dst].graddin =panel[src].graddin;
}

int PanelIsVisible(int n)
//возвращает бинарное значение флага видимости панели n для рендера
  //и вычисляет ее PEN-bmp (xbe) координаты
{
long XbeA,Zpe0A,Zpe1A,XbeB,Zpe0B,Zpe1B,Xbelft,Xbergt;
fix Yp0,Yp1;//удаление Yp края xp0, xp1 панели
fix pnflenx,pnflenz;//размеры панели n
//координаты левого края и правого края панели n в системе панели n,
//которые не будут уходить назад за границу видимости
fix xp0,xp1;
//координата верха панели n в системе панели n (низ=0)
fix zp1;
angle an,an1;
int gr,front;

//определяем, будет ли наблюдатель видеть хотябы часть панели
//и каковы границы этой части на экране наблюдения
X0=panel[n].pen.x0;
Y0=panel[n].pen.y0;
Z0=panel[n].pen.z0;
Alpha=panel[n].pen.angx;
fsin=fsinus(Alpha);
fcos=fcosin(Alpha);
pnflenz=panel[n].flenz;
pnflenx=panel[n].flenx;
//сначала xp имеет максимальный диапазон
xp0=0;
xp1=pnflenx;//тут концы без запаса, чтобы диапазон панели был идеальным
zp1=pnflenz;//для закрытия щелей, возникающих при применении ZAPASX/Z !

Yp0=Y0;
Yp1=Y0+fixmul(xp1,fsin);
//если оба края панели сзади границы видимости - конец
if ((Yp0<FIXYPMIN)&&(Yp1<FIXYPMIN)) return NO;
//если только один из краев сзади - выкидываем его задний кусок
if (Yp0<FIXYPMIN)
 {
 Yp0=FIXYPMIN;
 //находим соответствующую точку панели на границе видимости
 xp0=fixdiv(Yp0-Y0,fsin);//fsin=0 только для парралельной X панели !
 }
else
if (Yp1<FIXYPMIN)
 {
 Yp1=FIXYPMIN;
 xp1=fixdiv(Yp1-Y0,fsin);
 }
//теперь кусок панели xp0...1 всегда имеет Yp0...1>=FIXYPMIN

//определяем PEN-bmp/pan координаты вершин видимого куска панели

//проекция точек панели на ребре xp: (xp,0) и (xp,zp1)
//в точки PEN-bmp/pan (Xbe,Zpe0) и (Xbe,Zpe1) соотв-но,

//две вершины на ребре xp0
XbeA=lfixmuldiv(val_h,fixmul(xp0,fcos)+X0,Yp0)-penpozx0;
//я вычисляю Zpe вместо Zbe для экономии и защиты от переполнения!
Zpe0A=lfixmuldiv(val_h,Z0,    Yp0)-penpozz0;
Zpe1A=lfixmuldiv(val_h,Z0+zp1,Yp0)-penpozz0;

//другие две вершины, на ребре xp1
XbeB=lfixmuldiv(val_h,fixmul(xp1,fcos)+X0,Yp1)-penpozx0;
Zpe0B=lfixmuldiv(val_h,Z0,    Yp1)-penpozz0;
Zpe1B=lfixmuldiv(val_h,Z0+zp1,Yp1)-penpozz0;

//на экране трапеция 'на боку', границы описывающего ее прямоугольника
if (XbeA<=XbeB)//панель видна спереди
  {
  //проверяем флаг видимости фронта панели
  if ((panel[n].sides&SIDE0)==0) return NO;
  Xbelft=XbeA;
  Xbergt=XbeB;
  front=YES;
  }
else//панель видна сзади
  {
  //проверяем флаг видимости тыла панели
  if ((panel[n].sides&SIDE1)==0) return NO;
  Xbelft=XbeB;
  Xbergt=XbeA;
  front=NO;
  }
//Zpe1 всегда больше Zpe0, т.к. zp1>0
Zpetop=max(Zpe1A,Zpe1B);
Zpebtm=min(Zpe0A,Zpe0B);

//несколько проверок вылезания ВСЕГО этого прямоугольника за экран
if (Xbergt<0) return NO;//панель слева экрана
if (Xbelft>=penbmplenx) return NO;//справа
if (Zpetop<0) return NO;//снизу
if (Zpebtm>=penpanlenz) return NO;//сверху

//отсекаем вылазиющие за экран по xbe ЧАСТИ его
if (Xbelft<0) Xbelft=0;
if (Xbergt>=penbmplenx) Xbergt=penbmplenx-1;
//диапазон по xbe оставшейся части заносим в структуру панели n
panel[n].xbelft=(int)Xbelft;
panel[n].xbergt=(int)Xbergt;

//не-яркость будет меняться только при перемещении панели или юзера!!!
if (graddinflag!=0)//динамическая не-яркость
  {
  an=panel[n].ear.angx;
  an1= (an<ANG180G)? an:(0-an);
  switch(graddinflag)
    {
    case 1://не-яркость 0...16
      gr=(int)(an1/2048);//если виден фронт панели
      if (front==NO) gr=16-gr;//если виден тыл панели
      break;

    default://конец цикла изменения флага
      graddinflag=gr=0; break;
    }
  panel[n].graddin=gr;
  }
else panel[n].graddin=0;

//возвращаем признак видимости панели на экране
return YES;
}

void ComputPanelsPosition(void)
//определяет координаты панелей в системах земли и наблюдателя и
  //их видимость для рендера
{
int n,own;

//систему осколков все время привязываем к AP1
//для движения осколков синхронно с панелью-целью
AP[ADDR_OS].ear=     AP[ADDR_APFirst].ear;
AP[ADDR_OS].earshift=AP[ADDR_APFirst].earshift;

for (n=0; n<=maxpannum; ++n)
  {
  if (panel[n].flag==NO) continue;//панель отсутствует в пространстве

  own=panel[n].owner;//AP-собственник панели
  //own[er] корректен при адресе от ADDR_VRS=0 до (MAXAP-1)
  if ((own<0)||(own>=MAXAP)) continue;

  if (AP[own].earshift==YES)//система AP owner-а панели сдвинулась,
    //старые ear/pen/xbe координаты панели недействительны
    panel[n].earvalid=panel[n].penvalid=panel[n].xbevalid=NO;

  if (PEN.usershift==YES)//USER-система сдвинулась,
    //pen и xbe координаты панели недействительны
    panel[n].penvalid=panel[n].xbevalid=NO;

  if (panel[n].earvalid==NO)//если ear координаты недействительны
    //из-за сдвига панели n (или системы ее owner-a)
    {
    //преобразуем координаты панели из системы AP в систему земли
    panel[n].ear=ConvD3pozFromAP2Earth(panel[n].ap,AP[own].ear);
    panel[n].earvalid=YES;
    //старые pen/xbe координаты панели недействительны
    panel[n].penvalid=panel[n].xbevalid=NO;
    }

  if (panel[n].penvalid==NO)//если pen координаты недействительны
    //из-за сдвига панели (или ее системы, или наблюдателя)
    {
    //преобразуем координаты панели из системы земли в USER-систему
    panel[n].pen=ConvD3pozFromEarth2AP(panel[n].ear,PEN.ear);
    panel[n].penvalid=YES;
    //старые xbe координаты панели недействительны
    panel[n].xbevalid=NO;
    }

  if (PEN.pozshift==YES)//изменилась PEN.poz (скос взгляда), теперь
    panel[n].xbevalid=NO;//недействительны xbe коорд-ты всех панелей!

  if (panel[n].xbevalid==NO)//если xbe коорд-ты недействительны
    {
    //определяем видимость панели для рендера и xbe коорд-ты
    panel[n].render=PanelIsVisible(n);
    panel[n].xbevalid=YES;
    }
  }

//только теперь сбрасываем флаги сдвигов оригинов всех AP
for (own=0; own<MAXAP; ++own) AP[own].earshift=NO;
//а также флаги сдвигов PEN
PEN.usershift=PEN.pozshift=NO;

//если какие-то панели были невидимы, то перед новым появлением
//надо сбросить все ...valid флаги этих панелей (или старший флаг)!
}

