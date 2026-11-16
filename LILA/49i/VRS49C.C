/*
    Постоянная часть СИ-кода Lila
*/


#include "vrs49.h"
#include "kbcod.h"
#include <sys\stat.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <alloc.h>
#include <string.h>
#include <conio.h>
#include <stdio.h>
#include <values.h>
#include <stdarg.h>
#include <dos.h>


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

void SetTimerFrequency(long firq)
//установка 0-го канала таймера на прерывания с частотой firq Герц
{
long divizer;//делитель частоты таймера;
int lb,mb;

if (firq<18) firq=18;
divizer=1191830L/firq;
if (divizer>65535L) divizer=65535L;
if (divizer<20) divizer=20;//не более 60 кГц...
lb=(int)(divizer&255);
mb=(int)(divizer/256);
//0x36=00110110: загрузка 2-х двоичных байт делителя в 0-й канал,
  //режим 3 (генератор меандра)
disable();
outportb(0x43,0x36);
outportb(0x40,lb);
outportb(0x40,mb);
enable();
}

word Write(int handle,void far *buf,word len)
{
static word n;
return _dos_write(handle,buf,len,&n);
}

void SetScreenMode(int mode)
//установка видео-моды mode с очисткой экрана
{
_AX=mode;
geninterrupt(0x10);
actscrnmode=mode;
}

int GetScreenMode(void)
//возвращает текущую видео-моду экрана
{
_AH=0x0F;
geninterrupt(0x10);
_AH=0;
return _AX;
}

void SetBorderColor(int bclr)
//цвет рамки экрана дисплея
{
_BH=(byte)bclr;
_AX=0x1001;
geninterrupt(0x10);
}

void NewInterruptHandlers(void)
//включение новых обработчиков прерываний и их начальные установки
{
OldTimerInterrupt=getvect(0x08);
OldKeyboardInterrupt=getvect(0x09);
Old0Interrupt=getvect(0);
setvect(0,New0Interrupt);
setvect(0x08,NewTimerInterrupt);
setvect(0x09,KeyboardInterrupt);
SetTimerFrequency(250);
}

void OldInterruptHandlers(void)
//возврат старых обработчиков прерываний и их установок
{
SetTimerFrequency(18);
setvect(0x09,OldKeyboardInterrupt);
setvect(0x08,OldTimerInterrupt);
setvect(0,Old0Interrupt);
}

int LoadYcolTabFile(char *fname)
//загрузка с диска таблицы gradcol[][]
{
int hnd;
hnd=open(fname,O_RDONLY|O_BINARY,S_IREAD);
if (hnd==-1) return ERR;//нет такого файла
Read(hnd,gradcol,sizeof(gradcol));
close(hnd);
return OK;
}

int LoadPalTabFile(char *fname)
//загрузка с диска таблиц syspal и nnn
{
int hnd,col3;
hnd=open(fname,O_RDONLY|O_BINARY,S_IREAD);
if (hnd==-1) return ERR;//нет такого файла
Read(hnd,syspal,sizeof(syspal));
Read(hnd,nnn,sizeof(nnn));
close(hnd);
//устанавливаем палитру несистемных цветов
//COLTRANS, COLBORDR и COLEMPTY итак черные...
col3=COLPRINT*3;
syspal[col3]=syspal[col3+1]=syspal[col3+2]=63;
return OK;
}

int LoadScod2kbTabFile(char *fname)
//загрузка с диска таблицы scod2kbcod
{
int hnd;
hnd=open(fname,O_RDONLY|O_BINARY,S_IREAD);
if (hnd==-1) return ERR;//нет такого файла
Read(hnd,scod2kbcod,sizeof(scod2kbcod));
close(hnd);
return OK;
}

int LoadSinTabFile(char *fname)
//загрузка с диска тригонометр-х таблиц
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
word n;
_dos_read(handle,buf,len,&n);
return n;
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
lgrad=(360+grad%360)%360;
return (angle)((lgrad<<16)/360);
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
{
int a;
disable();//для безопасности irq мыши во время Mouse() !!
_AX=0;//аппаратно-программный сброс мыши
Mouse();//вызов драйвера мыши вместо int33 - так быстрее!
a=_AX;
enable();//это надо иметь ввиду если код сам сидит в irq!
if (a!=-1) mousepresent=NO;
else       mousepresent=YES;
}

void MouseSense(void)
//установка стандартной чувствительности мыши x=y=50 микки-пиксель
//и огромного 7777>>50 порога удвоения скорости, т.е. нет удвоения
{
disable();
_DX=7777;
_CX=50;//y
_BX=50;//x
_AX=0x001A;
Mouse();
enable();
}

void MouseStatus(void)
//определяет все параметры мыши
{
int mkeys;

disable();
//определение счетчиков перемещения мыши
_AX=0x000B; Mouse();
mousedx=_CX;//перемещение по X после последнего вызова функции
mousedy=_DX;// --//--     по Y --//--

//определение текущего положения клавиш мыши
_AX=0x0003; Mouse();
mkeys=_BX;//_BX бит 0 (1) == 1: нажата левая (правая) клавиша

//число нажатий    b0 за время с последнего вызова этой функции
_BX=0; _AX=0x0005; Mouse();
mouseb0on=_BX;//может быть любое >=0 !
//число отпусканий b0 --//--
_BX=0; _AX=0x0006; Mouse();
mouseb0off=_BX;
//то же для b1
_BX=1; _AX=0x0005; Mouse();
mouseb1on=_BX;
_BX=1; _AX=0x0006; Mouse();
mouseb1off=_BX;

enable();
mouseb0=mkeys&1;
mouseb1=(mkeys&2)>>1;
mousedy=-mousedy;//так более логично!
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
  Xbelft=XbeA;
  Xbergt=XbeB;
  }
else//панель видна сзади
  {
  //проверяем разрешения видимости оборота панели
  if (panel[n].oborot==NO) return NO;
  Xbelft=XbeB;
  Xbergt=XbeA;
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
//признак видимости панели на экране
return YES;
}

//если этот обработчик IRQ клавиатуры компилить 386-м кодом, то
//для 32-бит операций _может_ использоваться EAX, хотя в стеке
//сохраняется только AX, что приведёт к многочисленным сбоям !!!
//поэтому здесь должны использоваться только 16-битные операции!
//irq таймера со сменой vrs_time не происходят до iret отсюда !!
#define ENCOD 224//сканкод-маркер расширенных клавиш (224 on 224 off);
void interrupt far KeyboardInterrupt(void)
{
static int enflag=0;//флаг последействия сканкода ENCOD;
static int flag225=0;//флаг последействия сканкода 225 клавиши Pause:
  //после него флаг=2 чтобы пропустить 2 следующих за 225 сканкода;
static int pausecnt=0;//счетчик для клавиши Pause:
  //на первый сканкод 225 он увеличивается до 1, генерится спецкод
  //нажатия Pause, после следующего 225 он только сбрасывается в 0;
static int scan;//физический сканкод;
static int scod;//спецкод из scod.h;

anycodflag=1;//принят какой-то код...

scan=inportb(0x60);

if (scan==ENCOD) { enflag=1; goto end; }
if (scan==225)
  {
  flag225=2;
  if (++pausecnt>=2) pausecnt=0;
  if (pausecnt==1)
    {
    if (kbbufvalid==YES)//если буфер готов для записи
      {
      //запись кода в буфер нижнего уровня
      kbbuf[kbwrite].t0=vrstime0;
      kbbuf[kbwrite].t1=vrstime1;
      kbbuf[kbwrite].scod=0;//спецкод клавиши Pause!
      kbbuf[kbwrite].hit=KEYOFF;//делаем отпускание!
      //установка позиции для записи в буфер следующего кода клавиши
      if (++kbwrite>=KBBUF) kbwrite=0;
      //если следующая запись наедет на начало кольца кодов
      if (kbwrite==kbread)
        //возврат на шаг для затирания конца вместо начала
        if (--kbwrite<0) kbwrite=KBBUF-1;
        //@ можно дать звуковой сигнал ошибки переполнения буфера !
      }
    }
  goto end;
  }
if (flag225>0) { --flag225; goto end; }

scod=scan&0x7F;//старший бит обнуляем;
if (enflag==1)
  {
  enflag=0;
  //клавиша CodPrntScrn генерит два кода, остается код 55/183,
    //а совпадающий с другой клавишей код 42/170 игнорируется
  if (scod==42) goto end;
  //прошлый код был ENCOD, нынешний - расширенный сканкод
  scod+=128;
  }

if (kbbufvalid==YES)
  {
  kbbuf[kbwrite].t0=vrstime0;
  kbbuf[kbwrite].t1=vrstime1;
  kbbuf[kbwrite].scod=scod;
  kbbuf[kbwrite].hit= (scan<128)? KEYON:KEYOFF;
  if (++kbwrite>=KBBUF) kbwrite=0;
  if (kbwrite==kbread) if (--kbwrite<0) kbwrite=KBBUF-1;//@
  }

end:
scan=inportb(0x61);
outportb(0x61,scan|0x80);
outportb(0x61,scan&0x7f);
outportb(0x20,0x20);
}

void MSGDel(int num)
//стираем сообщение в позиции буфера num
{
msg[num].cmd=MSG_NULL;
//опционально печатаем (об этом сбое)
printf("MSG from %i to %i deleted !\n",msg[num].srcaddr,msg[num].dstaddr);
}

void ComputPanelsPosition(void)
//определяет координаты панелей в системах земли и наблюдателя и
  //их видимость для рендера
{
int n,own;

for (n=0; n<=maxpannum; ++n)
  {
  if (panel[n].flag==NO) continue;//панель отсутствует в пространстве

  if ((n==pnclpan)||(n==curpan))//обе панели указки считаем особо
    {
    //определяем видимость панели для рендера и xbe коорд-ты
    panel[n].render=PanelIsVisible(n);
    continue;
    }

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
for (own=0; own<=ADDR_LAST_AP; ++own) AP[own].earshift=NO;
//а также флаг сдвига PEN.poz
PEN.pozshift=NO;

//если какие-то панели были невидимы, то перед новым появлением
//надо сбросить все ...valid флаги этих панелей (или старший флаг)!
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

void PaintText(char far *tstr,int tlen,int bmap,int x,int z,
               int fgbg,int fgcol,int bgcol)
//не более tlen литер из текстстроки tstr рисуется в битмап bmap,
//сейчас bmap - это номер блока памяти текстуры!
//fg-цвет пикселей букв col, bg-цвет под буквами не меняется;
//оригин (левый нижний пиксель) первой литеры в x,z битмапа,
//для самих x/z отсчет всегда от левого нижнего угла битмапа!
//при налезании литеры на конец битмапа конец вывода текста;
//эта функция для моды битмапа 2 !
{
byte far *memram;
byte far *ramlit;
byte far *ramstr;
byte near *ramfnt;
int i,j,bitstr,let,lenx,lenz,lenz8,maxlen,mem;

mem=bmap;//!
memram=memory[mem].rampntr;
lenx=memory[mem].lenx;
lenz=memory[mem].lenz;
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

void PaintText88(char far *tstr,int tlen,int bmap,int x,int z,
                 int fgbg,byte far *fgcol,int bgcol)
//текст с трафаретом из 8x8 fg-цветов, плюс возможно const bg-цвет;
//fgcol[i][j]: i растет слева направо, j растет снизу вверх !
//эта функция для моды битмапа 2 !
{
byte far *memram;
byte far *ramlit;
byte far *ramstr;
byte near *ramfnt;
int i,j,bitstr,let,lenx,lenz,lenz8,maxlen,mem;
byte far *fgc;

mem=bmap;//!
memram=memory[mem].rampntr;
lenx=memory[mem].lenx;
lenz=memory[mem].lenz;
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

void CopyPAN(int dst,int src)
//копирует параметры src панели в dst без закрепления!
{
panel[dst].oborot=panel[src].oborot;
panel[dst].ap=    panel[src].ap;
panel[dst].earvalid=NO;
panel[dst].penvalid=NO;
panel[dst].xbevalid=NO;
panel[dst].flenx= panel[src].flenx;
panel[dst].flenz= panel[src].flenz;
panel[dst].memtex= panel[src].memtex;
panel[dst].memconv=panel[src].memconv;
panel[dst].fmulx= panel[src].fmulx;
panel[dst].fmulz= panel[src].fmulz;
panel[dst].xb0=   panel[src].xb0;
panel[dst].xb1=   panel[src].xb1;
panel[dst].zb0=   panel[src].zb0;
panel[dst].zb1=   panel[src].zb1;
panel[dst].consti=panel[src].consti;
panel[dst].neproz=panel[src].neproz;
panel[dst].rand  =panel[src].rand;
}

void PencilPoz(int numpoz)
//перебор позиций (острия) карандаша на экране
{
pnclmmm=1;//карандаш едет по x/z;

//посредине экрана по x/z
pnclx=penbmplenx/2;
pnclz=penbmplenz/2;
switch(numpoz)//дальность по y
  {
  //мин удаление, сдвиг вправо чтобы не было клиппинга из-за карандаша!
  case 0: fpncly=FIXYPMIN; pnclx+=penbmplenx/4; break;
  case 1:
  case 3: fpncly=FMINYSTEPSW; break;//номинальное
  case 2: fpncly=FPNCLMAXY; break;//макс удаление
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
int x,lenx,lenz,maxx,maxz,tmem;
byte far *tmemram;

if ((x0>x1)||(z0>z1)) return;
lenx=memory[bm].lenx; maxx=lenx-1;
lenz=memory[bm].lenz; maxz=lenz-1;
if ((x0>maxx)||(x1<0)) return;
if ((z0>maxz)||(z1<0)) return;
x0=max(x0,0); x1=min(x1,maxx);
z0=max(z0,0); z1=min(z1,maxz);
tmem=bm;//!
tmemram=memory[tmem].rampntr;
if (memory[tmem].rammode!=2) ConvertMEMmode(tmem,2);
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
char s[128];//буфер вывода с запасом
int bm,x,z,x0,z2,len;

if (glen<0) return -1;
va_start(argptr,format);
len=vsprintf(s,format,argptr);//вывод строки в буфер
va_end(argptr);
bm=panel[pan].memtex;
x0=glen;
z2=memory[bm].lenz-glen;
x=x0+hor*8;
z=z2-ver*8-8;
PaintText(s,len,bm,x,z,PAINTFG,fgcol,0);
return len;
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
//возврат старых обработчиков прерываний и их установок
OldInterruptHandlers();
if (cod!=0)
  {
  printf("Exit Program Error Code %i !\n",cod);
  printf("Press any key to end program !\n");
  getch();
  }
//старый режим дисплея
SetScreenMode(startscrnmode);
exit(0);
}

void RAMAlloc(void)
//выделение far RAM для блоков памяти
{
ramfullfree=farcoreleft();//размер доступной far RAM !
if (ramfullfree<MINRAMSIZE)
  {
  printf("RAM size %li too small for start Lila !\n",ramfullfree);
  EndProgram(1);
  }
if (ramfullfree<NOMRAMSIZE)
  printf("RAM size %li small for all Lila options !\n",ramfullfree);

//выделение far RAM
ramfullstart=(byte huge *)farmalloc(ramfullfree);
if (ramfullstart==NULL)
  {
  printf("DOS farmalloc() error !\n");
  EndProgram(2);
  }

//надежная установка начального адреса и длины кратными 4-м
ramfullfree=4*(ramfullfree/4-2);
ramfullstart=(byte huge *)
  MK_FP(FP_SEG(ramfullstart),4*(FP_OFF(ramfullstart)/4+1));

ramstart=ramfullstart;
ramfree=ramfullfree;
}

void ZeroPANhead(int n)
{
panel[n].flag=NO;
panel[n].oborot=YES;
panel[n].render=NO;
panel[n].owner=ADDR_NOBODY;
panel[n].count=0;
panel[n].ap=d3null;
panel[n].earvalid=NO;//т.о. ear,
panel[n].penvalid=NO;//pen и
panel[n].xbevalid=NO;//xbe можно не обнулять!
panel[n].flenx=0;
panel[n].flenz=0;
panel[n].memtex =ISNOT;
panel[n].memconv=ISNOT;
panel[n].fmulx=FIX1;
panel[n].fmulz=FIX1;
panel[n].xb0=0;
panel[n].xb1=0;
panel[n].zb0=0;
panel[n].zb1=0;
panel[n].consti=0;
panel[n].neproz=0;
panel[n].rand=0;//флуктуации пикселей будут, но пока нулевые!
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
//ресурсы панели закрепляются;
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
//быстро копирует len байт памяти src в память dst;
//замедление 15% от rep movsd при len=1280, 30% при len=640 !
//опасно применять эту ф-ю при работе с содержимым блоков
//памяти без тщательного контроля невыхода за их границы! @
{
word soff,doff;
soff=FP_OFF(src);
doff=FP_OFF(dst);
//если количество байт и начальный адрес src/dst кратны 4
if (((len&3)==0)&&((soff&3)==0)&&((doff&3)==0))//то dword версия
  MoveDword(FP_SEG(src),soff,FP_SEG(dst),doff,len>>2);
else
  movedata(FP_SEG(src),soff,FP_SEG(dst),doff,len);
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
if (lenval<=1)//повторяем младший байт в старшем
  { val&=0xFF; val|=(val<<8); }
else//lenval=2
  numval<<=1;//тепрь это число байт !

//если количество байт и начальный адрес dst кратны 4
if (((numval&3)==0)&&((FP_OFF(dst)&3)==0))//то dword версия
  SetDword(dst,numval>>2,val);
else
  Setmem(dst,numval,val);
}

void SetMEMtoConstval(int mem,int val)
//заполняет память блока mem байтовой ! константой val
//эта ф-я пока нигде не применяется @ !
{
//здесь для скорости используется fullen вместо логичных len
SetRAM(memory[mem].rampntr,memory[mem].fullen,val,1);
}

void USERandPENstartpoz(void)
//начальная позиция наблюдателя и PEN
{
//юзера в оригин земли и выше на рост человека USERZ0
USERearfixXYZgrad(0,0,USERZ0,0);
//скос взгляда нулевой
PEN.zgrad=0;
PEN.pozshift=YES;
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
  PEN.ear=ConvD3pozFromAP2Earth(delta,panel[pan].ear);
  PEN.usershift=YES;
  }
else//PAN2USER
  {
  delta.x0=-delta.x0;
  delta.y0=-delta.y0;
  delta.z0=-delta.z0;
  //ставим _панель_ на delta перед юзером
  panel[pan].ear=ConvD3pozFromAP2Earth(delta,PEN.ear);
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
//начальная позиция наблюдателя и PEN
USERandPENstartpoz();
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

void SetPanelSize2Bmap(int pan)
//уст-ка размеров панели с охватом всего её битмапа 1:1
{
int bmap;
bmap=panel[pan].memtex;
panel[pan].flenx=panel[pan].fmulx*memory[bmap].lenx;
panel[pan].flenz=panel[pan].fmulz*memory[bmap].lenz;
}

int GetLockMEMconv(int owner)
//выделяет/закрепляет один конверт, возвращает номер его блока памяти
{
return GetLockMEM(owner,256);
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
//потом можно скорректировать эти пар-ры...

//круглость круга согласно рекомендуемым множителям из битмапа:
//метод растяжения панели по вертикали Z 1/1 -> 6/5
panel[pan].fmulz=fixmul(panel[pan].fmulz,memory[bmap].fmulz);

//mulx/mulz панели для корректировки ее размеров относительно
//битмапа должны устанавливаться перед вызовом данной функции !
SetPanelSize2Bmap(pan);//размеры панели с охватом всего битмапа
//а после этого можно изменить размеры панели...
}

void WarningWKP(char *str,int code)
//сообщение кода не фатальной ошибки, ожидание keypress
{
if (printwarn==OFF) return;
printf("%sError code is %i\n",str,code);
printf("Press any key for continue !\n");
WKP
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

void CursorOnPanel(int pan)
//прицепление курсора к панели pan
{
if (pan<=ISNOT) return;
//флаг autobestpoz поднимается и bestpozmode переключает AP!
autobestpoz=OFF;
curvisible=YES;//курсор визуализируется!
curnumpan=pan;
pointermode=1;//карандаш->курсор AP
curmousemode=1;//мышь будет перемещать курсор!
//установка параметров курсора
curpandx=curpandz=0;
curk0=curk1=curk0on=curk0off=curk1on=curk1off=0;
fcurstepx=panel[pan].fmulx;
fcurstepz=panel[pan].fmulz;
fcurlenx=fcurstepx*memory[curbm].lenx;
fcurlenz=fcurstepz*memory[curbm].lenz;
fcurpany=fcurstepx+fcurstepz;
curminx=curminz=0;
curmaxx= (fcurstepx>0)? (int)(panel[pan].flenx/fcurstepx):0;
curmaxz= (fcurstepz>0)? (int)(panel[pan].flenz/fcurstepz):0;
curpanx=curmaxx/2;
curpanz=curmaxz/2;
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
//устанавливает цвета палитры VGA от startcol до endcol;
//rgb - это указатель на массив троек байт r,g,b, r,g,b, ...
{
int i,scol,ecol;
scol=startcol*3;
ecol=endcol*3+3;
disable();
outportb(0x3C8,startcol);
for (i=scol; i<ecol; ++i) outportb(0x3C9,rgb[i]);
enable();
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

#define VRS_TIME_STEP  4//250 Гц дают шаг в 4 мсек !
void interrupt far NewTimerInterrupt(void)
//Период 55-мсекундного вызова OldTimerInterrupt ДОСа набирается
//из 4 мсек-го периода таймера VRS (250 Гц) так:
//3 шага из 14 периодов по 4 мс и 1 шаг из 13 периодов по 4 мс,
//что даст 4 шага ДОСа по 55 мсек.
{
static int maxstepcnt[4]={14,14,14,13};//пределы счета периодов шагов
static int numstep=0;//номер шага
static int stepcnt=0;//счетчик периодов внутри шага
word oldt0;
int sp;

sp=_SP;//следим за стэком !
if (SPmin>sp) SPmin=sp;
if (SPmax<sp) SPmax=sp;

vrs_time+=VRS_TIME_STEP;
//этот код нужен для защиты от сбоев в irq1 из-за несохранения EAX
oldt0=vrstime0;
vrstime0+=VRS_TIME_STEP;
//если переполнение в младшем слове времени
if (vrstime0<oldt0) ++vrstime1;

if (++stepcnt<maxstepcnt[numstep]) outportb(0x20,0x20);//EOI
else//вызываем старый обработчик irq0 с частотой ~18.2 Гц
  {
  stepcnt=0;
  if (++numstep>=4) numstep=0;
  OldTimerInterrupt();//там есть свой EOI
  }
}

void CompressMEM(void)
//сжатие RAM
{
long sortkey[MAXMEM];//параметры сортировки MEM
int sortind[MAXMEM];//номера MEM в порядке возрастания sortkey
byte far *ram;
byte huge *ramnew;//новое начало RAM для очередного блока памяти
byte huge *ramold;//старое начало RAM
word len;
int i,n;

//запись полных RAM-адресов блоков для сортировки
for (n=0; n<=maxmemnum; ++n)
  {
  sortind[n]=n;
  ram=memory[n].rampntr;
  sortkey[n]=(((long)FP_SEG(ram))<<4)+FP_OFF(ram);
  }
//сортировка блоков по порядку возрастания адресов
QuickSort(maxmemnum+1,sortkey,sortind);

//перемещение всех занятых блоков в RAM вниз, а всех дыр вверх:

ramnew=ramfullstart;//начало RAM для всего набора блоков памяти
//спускаем занятые блоки вниз
for (i=0; i<=maxmemnum; ++i)//перебираем массив сортированных номеров
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
long sortkey[MAXPAN];//параметры сортировки PAN,
int sortind[MAXPAN];//номера PAN в порядке возрастания sortkey;

//сортировка стеклянных панелей
spannum=0;
for (n=0; n<=maxpannum; ++n)
  {
  if (panel[n].flag!=YES) continue;
  if (panel[n].render!=YES) continue;
  //пропускаем _нестеклянные_ панели
  if (panel[n].neproz<=0) continue;
  if (n==pnclpan) continue;
  sortind[spannum]=n;
  sortkey[spannum]=panel[n].pen.y0;
  ++spannum;
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

int _fastcall hexcol(int rgb)
//выдает цвет syspal по 3-х разрядному гекс-числу h=0xRGB
{
return nnn[0xf&(rgb>>8)][0xf&(rgb>>4)][0xf&rgb];
}

int _fastcall color(int r16,int g16,int b16)
//цвета с 4-бит. компонентами rgb, в шкале интенсивностей 0...15 !
{
return Color(r16<<2,g16<<2,b16<<2);
}

void TablesSetup(void)
//загрузка с диска табличных массивов данных
{
_fmode=O_BINARY;
if (LoadSinTabFile("sinus.lii")!=OK)
  {
  printf("Error loading sinus.lii file !\n");
  EndProgram(101);
  }

if (LoadScod2kbTabFile("keyboard.lii")!=OK)
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

if (LoadPalTabFile("syspal.lii")!=OK) SyspalSetup();

if (LoadYcolTabFile("gradcol.lii")!=OK) GradcolSetup();
}

void GradcolSetup(void)
//рассчет таблицы gradcol[][] и запись на диск
{
int hnd;
ComputGradcol();
_fmode=O_BINARY;
hnd=creat("gradcol.lii",S_IREAD|S_IWRITE);
Write(hnd,gradcol,sizeof(gradcol));
close(hnd);
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

#define FARPOWER     25//яркость последней градации цвета в % от 100
#define MAXGRADCOL1 (MAXGRADCOL-1)
void ComputGradcol(void)
//рассчет таблицы градация яркости цветов gradcol[][col]
{
int i,i1,col,c3,r,g,b,r1,g1,b1;
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
  for (i=1; i<=MAXGRADCOL1; ++i)//0-й слой пропускаем!
    {
    i1=( i * (100-FARPOWER) ) / 100;
    //понижение яркости цветной картинки
    r1=( r * (MAXGRADCOL1-i1) )/MAXGRADCOL1;
    g1=( g * (MAXGRADCOL1-i1) )/MAXGRADCOL1;
    b1=( b * (MAXGRADCOL1-i1) )/MAXGRADCOL1;
    //соответствующий номер цвета syspal
    gradcol[i][col]=Color(r1,g1,b1);
    }
  }

//все цвета слоя gradcol[0] принудительно делаем оригинальными,
//иначе появляются небольшие изменения...
for (i=0; i<256; ++i) gradcol[0][i]=i;
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
        syspal[c3]  =i<<2;
        syspal[c3+1]=j<<2;
        syspal[c3+2]=k<<2;
        //из метки делаем номер цвета!
        nnn[i][j][k]=col;
        }
      }
//контроль: сейчас проверено, col=255 !
//printf("MAX color=%i !\n",col); WKP
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

  //копируем градации яркости<100% из наиболее близкого цвета syspal
  col=Color(rr,gg,bb);
  for (j=1; j<MAXGRADCOL; ++j) gradcol[j][i]=gradcol[j][col];

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

void LilaPaintSetup(void)
//раскодировка цветов лейбла
{
int i,col;
//преобразуем литеры "0123456789abc..." в плавные цвета syspal
//а пробелы в прозрачный цвет 0
for (i=0; i<LILALEN; ++i)
  {
  col=lila[i];
  if (col==32) lila[i]=0;
  else
    {
    col=let2i(col); lila[i]=red[col-3];
    }
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

void AP_OutVGAgra(void)
//вывод PEN-панели на физический монитор VGA в графмоде 320x200
  //надо сделать возможным вывод ЛЮБОЙ панели !!!
{
static int addr_AP=ADDR_ALL;
int msgrd,msgwr;
int kbcod,hit;
int hlen,parthlen,vlen,j,ie0,je0;
//флаг корректировки и физическая цветность системной палитры
static int physpalcorrflag=NO;
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
        //затираем графэкран пустым цветом
        SetRAM(MK_FP(0xA000,0),64000u,COLEMPTY,1);
        SetBorderColor(COLBORDR);
        //установка физической палитры экрана
        SetPalette(syspal,0,255);//все цвета !!!
        //для стартовой корр-ки физпалитры с повышением цветности
        physpalcorrflag=YES;
        colness=FIX1+FIX05;//цветность = 1.5 улучшает все и не искажает
          //даже наиболее чувствительные картинки...
        }
      else//уже графрежим
        SetRAM(MK_FP(0xA000,0),64000u,COLEMPTY,1);
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
          default: break;

          //регулировка физической цветности палитры системы
          case KeyCodRight://увеличение на 0.5
            colness+=FIX05; physpalcorrflag=YES; break;
          case KeyCodLeft://уменьшение на 0.5
            colness-=FIX05; physpalcorrflag=YES; break;

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
        for (j=0; j<vlen; ++j)
          CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),penram+hlen*j,hlen);
        break;
      case 2://двухшаговый рендер
        if (PEN.tmpmem>ISNOT) tmpram=memory[PEN.tmpmem].rampntr;
        else//сбойная ситуация- нет левого полукадра
          tmpram=penram;//просто повторяем правый...
        for (j=0; j<vlen; ++j)//строка из 2-х половинок!
          {
          //сначала левая половина строки, из первого шага рендера
          CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0),tmpram+parthlen*j,parthlen);
          //потом правая половина
          CopyRAM(MK_FP(0xA000,320*(je0-j)+ie0+parthlen),
                  penram+parthlen*j,parthlen);
          }
        if (PEN.tmpmem>ISNOT)
          {
          UnLockMEM(PEN.tmpmem);//только здесь открепляем блок!
          PEN.tmpmem=ISNOT;//признак отсутствия памяти полукадра
          }
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

void AP_Render(void)
{
int msgrd,msgwr;
int kbcod,hit;
static int addr_AP=ADDR_ALL;
static int backgroundvar;//вариант вызова программ фона
static int penbmpsize=4;//переключатель PEN-bmp размеров
int pan,i;

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
      starvar=1;
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
        case KeyCodUp:
        case KeyCodDown:
        case KeyCodLeft:
        case KeyCodRight:
          //переадресуем хоткеи в AP_OutVGAgra для регулировки цветности
          if (Alter==ON) msg[msgrd].dstaddr=ADDR_APOVGAg;
          //или регулировки здесь
          if ((Control==ON)&&(hit==KEYON))
            switch(kbcod)
              {
              //меняем цвет неба
              case KeyCodUp: if (++nebocol>255) nebocol=255; break;
              case KeyCodDown:  if (--nebocol<0) nebocol=0; break;
              //меняем скорость вращения дали
              case KeyCodLeft: --dalidadt; break;
              case KeyCodRight: ++dalidadt; break;
              }
          goto nextmsg;
        }

      if (hit==KEYON)
        switch(kbcod)
          {
          case KeyCodSpace://нулевые градации и колебания
            gradfx=gradfy=gradamp=gradcon=0; break;

          //регулировка постоянных градаций всех панелей:
          case KeyCodRBracket://это } для ручного увеличения яркости
            --gradcon; break;
          case KeyCodLBracket://это { для --//--  уменьшения --//--
            ++gradcon; break;

          //параметры градаций цветов внутри панели:
          case KeyCodA://амплитуда
            if (Control==OFF) ++gradamp;
            else              --gradamp;
            break;
          case KeyCodX://пространств-я частота по x
            if (Control==OFF) { if (Shift==OFF) ++gradfx; else gradfx+=8; }
            else              { if (Shift==OFF) --gradfx; else gradfx-=8;
                                if (gradfx<0)   gradfx=0; }
            break;
          case KeyCodY:// --//-- по y
            if (Control==OFF) { if (Shift==OFF) ++gradfy; else gradfy+=8; }
            else              { if (Shift==OFF) --gradfy; else gradfy-=8;
                                if (gradfy<0)   gradfy=0; }
            break;

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

          case KeyCodPoint://это > для увеличения экрана
            if (++penbmpsize>4) penbmpsize=4; PEN.usershift=YES; break;
          case KeyCodComma://это < для уменьшения экрана
            if (--penbmpsize<0) penbmpsize=0; PEN.usershift=YES; break;

          }
      if (hit==KEYOFF)
        switch(kbcod)
          {
          //стоп движения дали
          case KeyCodAdd1: dalidadt=0; break;
          //даль есть или нет
          case KeyCodAdd3: dalinali^=1; break;

          case KeyCodEnter://печать текущего параметра варианта дали
            printf("dali[var=%1i][par=%2i]=%i, WKP !\n",
              dalivar,dnumpar,dalipar[dalivar][dnumpar]);
            WKP break;

          case KeyCodP://печать всех параметров варианта bgvar
            for (i=0; i<8; ++i) printf("%3i ",bgpar[bgvar][i]);
            printf("\nbgvar=%i\n",bgvar); WKP break;

          case KeyCod0: case KeyCod3: case KeyCod6:
          case KeyCod1: case KeyCod4: case KeyCod7:
          case KeyCod2: case KeyCod5: case KeyCod8:
          case KeyCod9://номер варианта bgvar, дали или неба
            {int var;
            var=kbcod-KeyCod0;
            if (Control==ON)//4 варианта дали
              { dalivar=var; newdali=YES; }
            else
            if (Alter==ON) ;//это для неба
            else//10 вариантов bgpar
              { bgvar=var; newbgpar=YES; }
            }
            if (dalivar>=DALIVAR) dalivar=DALIVAR-1;
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

          case KeyCodB://вариант программ рендера фона
            backgroundvar=(++backgroundvar)%3; break;

          //разрешение регулировки градаций внутри панелей
          case KeyCodG: gradvarflag^=1; break;
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
    case 3: penbmplenx=220; penbmplenz=140; break;
    //двухшаговый режим подготовки полного экрана!
    case 4: penbmplenx=320; penbmplenz=200; numpart=2; break;
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
      case 0://специальный фон
        //новые параметры битмапов земли, дали
        PaintZbitm();
        PaintDbitm();
        if (spaceflag==ON)//если космос
          {
          RenderNebo();//рендер неба
          RenderStars();//рендер звезд
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

      default: break;//case 1: нет стирания фона

      case 2: SetRAM(penram,PEN.ramlen,COLEMPTY,1); break;//черный фон
      }
    //заполнение y-буфера const y-профилем фона
    SetYbuf(YBG);

    //рендер нестеклянных панелей
    for (pan=0; pan<=maxpannum; ++pan)
      {
      //пропускаем отсутствующие и полностью невидимые панели
      if (panel[pan].flag!=YES) continue;
      if (panel[pan].render!=YES) continue;
      //пропускаем стеклянные панели для верного затенения!
      if (panel[pan].neproz>0) continue;
      //а также панель карандаша чтобы не ударяться об него!
      if (pan==pnclpan) continue;
      RenderPanel(pan);
      }
    //рендер всех стеклянных панелей
    RenderSteklPanels();
    //определяем параметры клиппинга, без карандаша !
    if (clip==YES) ClipComput();
    pan=pnclpan;//теперь рендер панели карандаша
    if ((panel[pan].flag==YES)&&(panel[pan].render==YES)) RenderPanel(pan);

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

void RenderNebo(void)
//рендер одноцветного фонового неба
{
int j0,j1;

//строки неба на горизонте и выше до конца экрана: j0...j1
j0=jear;
if (j0<0) j0=0;
//а если космос - то строки с самого низа
if (spaceflag==ON) j0=0;
j1=penbmplenz-1;
if (j0>j1) return;
SetRAM(penram+partpenbmplenx*j0,partpenbmplenx*(j1-j0+1),nebocol,1);
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

void RandPaintBitmap(int bm,int x0,int z0,int x1,int z1,int *col,int len)
//закраска прямоуг. куска (x0,z0)...(x1,z1) битмапа bm
//случайными цветами из набора цветов col[0...(len-1)]
{
int i,x,z,lenx,lenz,maxx,maxz,tmem;
byte far *tmemram;
byte far *ram;
dword seed;

if ((x0>x1)||(z0>z1)||(len<=0)) return;
lenx=memory[bm].lenx; maxx=lenx-1;
lenz=memory[bm].lenz; maxz=lenz-1;
if ((x0>maxx)||(x1<0)) return;
if ((z0>maxz)||(z1<0)) return;
x0=max(x0,0); x1=min(x1,maxx);
z0=max(z0,0); z1=min(z1,maxz);
tmem=bm;//!
tmemram=memory[tmem].rampntr;
if (memory[tmem].rammode!=2) ConvertMEMmode(tmem,2);
//мода битмапа 2 !
seed=0;
ram=tmemram+x0*lenz;
for (x=x0; x<=x1; ++x)
  {
  for (z=z0; z<=z1; ++z)
    {
    //встроенный случайный датчик для скорости!
    seed=RANDUMUL*seed+RANDUADD;
    i=(int)(0x7FFF&(seed>>8));
    ram[z]=col[i%len];
    }
  ram+=lenz;
  }
}

int _fastcall Color(int r,int g,int b)
//возвращает цвет в syspal, примерно соответствующий 18-битному
//цвету с 6-бит. компонентами r,g,b в шкале интенсивностей 0...63
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

void CursorSetup(void)
//сетап ресурсов курсора
{
int i,col;

//битмап указки-курсора
curbm=GetLockMEM2(ADDR_VRS,CURLENX,CURLENZ,FIX1);
memory[curbm].transflag=YES;
memory[curbm].rammode=1;
//литеры формы курсора "0...9abcdef" в цвета syspal
for (i=0; i<CURLEN; ++i)
  {col=cursor[i];
  col= (col==32)? 0:gray[let2i(col)];
  memory[curbm].rampntr[i]=col;}

//панель указки-курсора
curpan=GetPAN(ADDR_VRS);
LockPAN(curpan);//лочим панель без ресурсов!
SetPanelRes(curpan,curbm,ISNOT);//даем ресурсы, без конверта!
panel[curpan].consti=MAXCONSTI;//бесконечная яркость!
}

void PencilSetup(void)
//сетап ресурсов карандаша, который похож на ручку...
{
int bm,i,x,x1,z,ze,lenx,lenz,leno,col;

//панель указки-карандаша:
lenz=20;
lenx=160;
leno=32;//длина острия-пера
bm=GetLockMEM2(ADDR_VRS,lenx,lenz,FIX1);
pnclbm=bm;
memory[bm].transflag=YES;

ze=lenz/2;
for (z=0; z<ze; ++z)//красим древко карандаша
  {
  col=redgrn[z+3];
  bmline(bm,0,z,       0,lenx,col);
  bmline(bm,0,lenz-1-z,0,lenx,col);
  }
for (z=0; z<ze; ++z)//перекрашиваем острие карандаша (перо)
  {
  for (x=0; x<leno; ++x)
    {
    i=z+(leno-1-x)/2;
    if (i>15) i=15;
    col=gray[i];
    bmpixel(bm,x,z,       col);
    bmpixel(bm,x,lenz-1-z,col);
    }
  }
ze=lenz/2-1;
for (z=0; z<ze; ++z)//стираем текстуру в прозрачной части острия
  {
  x1=((ze-z)*(leno-1))/ze;
  bmline(bm,0,z,       0,x1+1,0);
  bmline(bm,0,lenz-1-z,0,x1+1,0);
  }
//дырка на пере
PaintBitmap(bm,20,ze-1,23,ze+2,0);
bmline(bm,19,ze,90,2,0);  bmline(bm,24,ze,90,2,0);
//толщину пера чуть уменьшаем у древка
PaintBitmap(bm,leno-4,0,leno-1,1,0);
PaintBitmap(bm,leno-4,lenz-2,leno-1,lenz-1,0);

col=redgrn[8];//текст на древке
PaintText("ЛИЛА * ТМ",9,bm,lenx/2-30,lenz/2-4,PAINTFG,col,0);

//точка острия битмапа карандаша в середине его по z
pncldz=memory[bm].lenz/2;//а по x пока всегда==0 !!!
pnclpan=GetPAN(ADDR_VRS);
LockPAN(pnclpan);
SetPanelRes(pnclpan,bm,ISNOT);
PencilPoz(pnclpozflag);//положение карандаша и угол
pnclangx=grad2ang(30);
panel[pnclpan].consti=MAXCONSTI;
}

int GetPANtxt(int owner,int glen,int tlenx,int tlenz,int panlenx,int consti)
//выделяет панель размером panlenx (z соотв.) см и яркостью consti
//для последующей записи ~текстовых данных, размер
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
bm=GetLockMEM2(owner,blenx,blenz,FIX6D5);
if (bm<=ISNOT) return ISNOT;
memory[bm].transflag=YES;//прозрачный цвет будет иметься

//если битмап есть:
LockPAN(pan);//лочим панель без ресурсов !

//сначала меняем параметры панели
fpanlenx=itofix(panlenx);
//длина z панели пропорциональна формату битмапа
fpanlenz=fixmuldiv(fpanlenx,itofix(blenz),itofix(blenx));
panel[pan].fmulx=fpanlenx/blenx;
panel[pan].fmulz=fpanlenz/blenz;//размеры битмапа-на всю панель
panel[pan].consti=consti;
panel[pan].rand=-1;//у текстовых панелей флуктуаций стандартно нет!

//а потом присоединяем ресурсы:
SetPanelRes(pan,bm,ISNOT);//конверта пока нет !
//координаты панели и содержимое битмапа определяет вызывающий код!
return pan;
}

void Setup000(void)
//сетап нулевых ресурсов panel[0], memory[0]
{
int mem,pan;

//выделяем аварийный блок mem=0=ISNOT, размером 4x1 !
mem=GetLockMEM(ADDR_VRS,4);
//выделяем аварийную панель pan=0=ISNOT,
pan=GetPAN(ADDR_VRS);//текстура и конверт будут =ISNOT !

if ((ISNOT!=0)||(mem!=0)||(pan!=0))
  {
  printf("Setup000() fatal error !\n");
  EndProgram(3);
  }
LockPAN(pan);

//подготовка курсора и карандаша только после этого
CursorSetup();
PencilSetup();
}

int GetLockMEM2(int owner,int lenx,int lenz,fix fmulz)
//выделяет блок двумерной памяти для битмапа с lenx/lenz и fmulz
//(для круглости круга и нормальной высоты литер fmulz=FIX6D5);
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
if (bm==ISNOT)//ошибка, выделение аварийного блока
  {
  //устанавливаем некоторые битмаповые поля аварийного блока
  memory[bm].rammode=2;
  memory[bm].transflag=NO;
  //сбой errmem печатается в GetLockMEM() !
  return ISNOT;
  }
//else OK, устанавливаем поля заголовка битмапа
memory[bm].lenx=lenx;
memory[bm].lenz=lenz;
memory[bm].fmulz=fmulz;
memory[bm].rammode=2;
return bm;
}

