/*
    Модуль AP_First
*/

#include "vrsport.h"
#include    "vrs58.h"
#include "kbcod.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

int OpenNextDOCfile(int napr);
void helpfgbg(void);
void helppage(int type,char far *pgbuf,long pglen);
void helpstring(char far *hstr,int strlen,int z);
void DocLoad(void);
void HelpSetup(void);
void AP1workHelp(void);
void helpactpagetest(void);
void helptext(void);
void AP1workPP(void);
void AP1workLL(void);
void AP1andPANwork(void);

#define HELPTIME   1000//время переворота страницы хелпа, мсек
#define HELPGRAD    135//абсолютный угол раскрытого хелп-журнала
#define HELPSHIFTY   10//абс. сдвиг обложки от главной панели хелпа по y, см
#define HELPGRAD0     2//2...3 градации _одиночной_ страницы хелпа
#define HELPGRAD1     1//1...2 градации на каждую добавочную страницу;
#define HELPDXYZ    400//растояние открытия обложки юзером, см
#define HELPLAST      5//за * страниц до остановки листания обновляем текст!
#define HELPPAGE    100//в хелпе не более * страниц;
#define HELPPGLEN  1000//длина страницы не более 40*25 литер;
#define FLAB      (FIX1+1024)//коэф-т регулировки размеров панели лабиринта

byte col88[64];//цвета текста хелпа
int helpbitm;//битмап _текстуры_ хелпа
int helpvarbg,helpvarfg;//варианты bg/fg обычного текста хелпа
int helpvarfg0=11,helpvarfg1=6;//fg-варианты интеркопи-бумаги и -текста
int helpconv0,helpconv1;//конверты интеркопи-бумаги и -текста

d3poz helppoz={-1000*FIX1,0,0,0};//позиция хелпа в системе AP_First
//PORT: позиция книжки перед юзером (после F1) - координаты юзера не меняются
static d3poz helppozusr;
static int helppozusrflag=NO;

int helpmem;//блок памяти с _текстом_ хелпа
long helpsize;//длина всего текста хелпа в байтах
//смещение первого байта каждой страницы [0...lasthelppage]
word shifthelppage[HELPPAGE+1];

//обложка хелпа = страница 0,
int lasthelppage;//текст хелпа = стр. 1...lasthelppage>=1;
int helpnewpage,helpactpage;//new/act номера страниц хелпа;
//панели хелпа: 0 панель 'обложка', 1 листаемая панель ('лист'),
int helppan0,helppan1,helppan2;//   2 'главная' панель для чтения текста;

d3poz llshift,ap1shift;
fix ap1flenx=FIX320,ap1flenz=FIX240;
fix ap1dadt,ap1dydt;
int ap1iplus,llfunct,llstate;
int ppfind=1;//для начальной загрузки pp-файла
int ap1shiftmode,llshiftmode=2;

void AP_First(void)
//первая ПП: хелп, лабиринт (груда панелей), звук...
//здесь более верный метод отработки сообщений, чем в других ПП !!!
{
static int addr_AP=ADDR_ALL;
int msgrd,msgwr,msgcmd;

//просматриваем весь буфер сообщений
msgrd=msgread;
msgwr=msgwrite;
while (msgrd!=msgwr)//сканируем весь буфер сообщений
  {
  //если сообщение не 'для всех' и не 'для этой AP', то пропуск его
  if ( !((msg[msgrd].dstaddr==ADDR_ALL)||(msg[msgrd].dstaddr==addr_AP)) )
    goto nextmsg;

  msgcmd=msg[msgrd].cmd;
  switch (msgcmd)//расшифровываем сообщение
    {
    case MSG_AP_SET_ADDR://принять адрес AP
      addr_AP=GetIntMSG(msgrd); break;

    case MSG_AP_CSTART://начать работу AP 'с нуля'
      AP[addr_AP].state=AP_WORK;
      //ставим систему AP в начальную позицию
      AP[addr_AP].ear=ap1startpoz;
      AP[addr_AP].earshift=YES;
//    if (helppan2==ISNOT) HelpSetup();
//    см KeyCodF1 !
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
      break;

    default://отрабатываем все остальные сообщения
      if (AP[addr_AP].state==AP_WORK)//состояние AP рабочее
        AP_FirstMSG(addr_AP,msgrd);
      else//пропускаем их
        printf("AP_First state is not AP_WORK !\n");
      break;
    }

nextmsg:
  if (msg[msgrd].srcaddr==addr_AP) { if (msg[msgrd].count>0) msg[msgrd].cmd=0; }
  else if (msg[msgrd].dstaddr==addr_AP) msg[msgrd].cmd=0;
  if (++msgrd>=MSGBUF) msgrd=0;
  }

if (AP[addr_AP].state==AP_WORK)//если состояние AP 'рабочее'
  {
  AP1workPP();//загрузка pp-файла

  //загрузка ll-файла во временный лабиринт
  //работа со временным лабиринтом
  AP1workLL();

  //[от]сцепление системы AP с юзером
  //вращение системы AP по углу (см ap1dadt юзера в s-модуле)
  //случайные сдвиги всех панелей AP
  AP1andPANwork();

  AP1workHelp();//работа с хелп-журналом
  }
}

void AP1workPP(void)
//загрузка pp-файла
{
     if (ppfind==2)   ReadNextPPfile(1);//циклическая
else if (ppfind==-2)  ReadNextPPfile(-1);// --//--
else if (ppfind!=0) { ReadNextPPfile(ppfind); ppfind=0; }//однократная
}

void AP1workLL(void)
//загрузка ll-файла во временный лабиринт
//работа со временным лабиринтом
{
int deltafile;
//загрузка ll-файла во временный лабиринт
switch(llfunct)
  {
  case 1://предыдущий файл
  case 2://текущий файл
  case 3://следующий файл
  case 10://циклически предыдущий файл
  case 11://циклически текущий файл
  case 12://циклически следующие файлы
    if (llstate!=0)//старый лабиринт еще в работе
      {
      DelLab(); llstate=0;//уничтожение его
      }
    //определяем имя ll-файла
    deltafile= (llfunct>3)? llfunct-11:llfunct-2;
    if (FindLLfile(deltafile)==OK)
      {
      ReadLLfile(ap1flenx,ap1flenz,ap1iplus);
      //отмечаем состояние работы с лабиринтом
      llstate=1;
      }
    break;
  default: break;
  }
//отмечаем отработку функции, кроме циклических чтений файлов
if (llfunct<10) llfunct=0;

if (llstate!=0)//работа со временным лабиринтом
  {
  switch(llshiftmode)
    {
    default: break;
    case 2://оригин лабиринта перемещаем на llshift от юзера
      AP[ADDR_Labir].ear=ShiftD3poz(llshift,PEN.ear);
      AP[ADDR_Labir].earshift=YES;
      break;
    case 1://запоминаем сдвиг лабиринта от юзера
      llshift=DeltaD3poz(AP[ADDR_Labir].ear,PEN.ear);
      llshiftmode=2;//и переходим к режиму сцепления
      break;
    }

  if (labdscrollx!=0) DeltaScrollLab();//вносим изменения в скроллинг
  }

labdscrollx=0;//всегда обнуляем d-скролл!
}

void DeltaScrollLab(void)
//вносим изменения в скроллинг панелей лабиринта, чтобы скролл
//был виден до оставления лабиринта в мире!
{
int n;
for (n=0; n<=maxpannum; ++n)
  if (panel[n].owner==ADDR_Labir) panel[n].scrollx+=labdscrollx;
}

void ScrollLab0(void)
//скорость скроллинга=0
{
int n;
for (n=0; n<=maxpannum; ++n)
  if (panel[n].owner==ADDR_Labir) panel[n].scrollx=0;
}

void AP1andPANwork(void)
//[от]сцепление системы AP с юзером
//вращение системы AP по углу (см ap1dadt юзера в s-модуле)
//случайные сдвиги всех панелей AP
{
//[от]сцепление системы AP с юзером
switch(ap1shiftmode)
  {
  default: break;
  case 2://оригин AP перемещаем на llshift от юзера
    AP[ADDR_APFirst].ear=ShiftD3poz(ap1shift,PEN.ear);
    AP[ADDR_APFirst].earshift=YES;
    break;
  case 1://запоминаем сдвиг AP от юзера
    ap1shift=DeltaD3poz(AP[ADDR_APFirst].ear,PEN.ear);
    ap1shiftmode=2;//и переходим к режиму сцепления
    break;
  }

//вращение системы AP по углу (см dadt юзера в vrsNNs.c)
if (ap1dadt!=0)
  {
  static int ost_da,angx2bit;
  angle angx;
  fix da;
  angx=AP[ADDR_APFirst].ear.angx|angx2bit;
  da=ap1dadt*cadrtime;
  ost_da+=(int)(da-(da/256)*256);
  da/=256;
  if (ost_da>=+256) { ++da; ost_da-=256; }
  if (ost_da<=-256) { --da; ost_da+=256; }
  angx+=(angle)da;
  angx2bit=angx&3;
  AP[ADDR_APFirst].ear.angx=angx&0xFFFC;
  AP[ADDR_APFirst].earshift=YES;
  }

//перемещение системы AP по y
if (ap1dydt!=0)
  {
  AP[ADDR_APFirst].ear.y0+=10*ap1dydt*cadrtime;
  AP[ADDR_APFirst].earshift=YES;
  }
}

void helpactpagetest(void)
//установка параметров страниц хелпа:
//угол листа и видимость его и обложки
{
if (helpactpage>0)//обложка раскрыта
  {
  panel[helppan0].flag=YES;//обложка
  panel[helppan1].flag=NO;//лист
  }
else//обложка закрыта
  {
  panel[helppan0].flag=NO;//вместо обложки:
  panel[helppan1].flag=YES;//лист,
  //паралельно главной панели хелпа
  panel[helppan1].ap.angx=panel[helppan2].ap.angx;
  }
}

void HelpSetup(void)
//инсталляция панелей и загрузка с диска хелпа;
//текстура всех страниц хелпа в битмапе стены !
//считаем что панели хелпа всегда есть...
{
//_спец конверты_ для интеркопи-бумаги и текста
helpconv0=GetLockMEMconv(ADDR_APFirst,NO);
helpconv1=GetLockMEMconv(ADDR_APFirst,NO);

//PORT: хелпу - СВОЙ битмап (копия стены): в оригинале helpbitm=wallbitm
//и текст хелпа рисовался в общий битмап стены - стены становились буквами
{
int lenx=memory[wallbitm].lenx,lenz=memory[wallbitm].lenz;
helpbitm=GetLockMEM2(ADDR_APFirst,lenx,lenz,
                     memory[wallbitm].fmulz,memory[wallbitm].transflag);
if (helpbitm>ISNOT) CopyMEM(helpbitm,wallbitm);
else helpbitm=wallbitm;
}
//загрузка хелпа в буфер и определение числа страниц текста
DocLoad();

//главная панель хелпа
helppan2=GetPAN(ADDR_APFirst);
SetPanelPar(helppan2,FULPAN,YES,0,0,0,0,0,SIDES);
//PORT: книжка либо перед юзером (после F1), либо на постоянном месте
panel[helppan2].ap= (helppozusrflag)? helppozusr:helppoz;
SetPanelRes(helppan2,helpbitm,ISNOT);
LockPAN(helppan2);

//остальные 2 панели копируем

helppan0=GetPAN(ADDR_APFirst);
CopyPAN(helppan0,helppan2);
panel[helppan0].ap.y0-=itofix(HELPSHIFTY)/2;
LockPAN(helppan0);

//лист дальше обложки, иначе он экранирует её!
helppan1=GetPAN(ADDR_APFirst);
CopyPAN(helppan1,helppan2);
panel[helppan1].ap.y0-=itofix(HELPSHIFTY);
LockPAN(helppan1);

//helpnewpage=helpactpage=0:
//вырубаем закрытую обложку, вместо неё будет лист !
panel[helppan0].flag=NO;
//градации сначала не нужны - текстура не прозрачная!
}

void AP1workHelp(void)
//вся работа с хелп-журналом
{
static int actgradus;//текущий угол листа в градусах
long deltagradus;
int deltapage,dxyz;
d3poz delta;

if (panel[helppan2].flag==NO)//вырубаем все панели хелпа
  {
  panel[helppan0].flag=NO;
  panel[helppan1].flag=NO;
  return;
  }
//else работаем с видимым хелпом

helpnewpage=max(helpnewpage,0);
helpnewpage=min(helpnewpage,lasthelppage);

//положение главной панели на случай колебаний
//PORT: книжка либо перед юзером (после F1), либо на постоянном месте
panel[helppan2].ap= (helppozusrflag)? helppozusr:helppoz;
panel[helppan2].earvalid=NO;
//остальные 2 панели сначала копируем
CopyPAN(helppan0,helppan2);
CopyPAN(helppan1,helppan2);
//положение оригина и угол обложки
panel[helppan0].ap.y0-=itofix(HELPSHIFTY)/2;
//обложка всегда открыта, а если обложку надо закрыть - это
//имитируется выключением обложки и оставлением листа после
//его закрытия видимым
panel[helppan0].ap.angx-=grad2ang(HELPGRAD);
//положение оригина листа
panel[helppan1].ap.y0-=itofix(HELPSHIFTY);
//видимость обложки и листа, а также угол листа см ниже

//градации панелей:
//на листе всегда одна страница
panel[helppan1].gradcon=HELPGRAD0;
//на обложке        1+(helpactpage-1) страниц
panel[helppan0].gradcon=HELPGRAD0+HELPGRAD1*(helpactpage-1);
//на главной панели 1+(lasthelppage-helpactpage) страниц
panel[helppan2].gradcon=HELPGRAD0+HELPGRAD1*(lasthelppage-helpactpage);

//направление листания и число страниц для него
deltapage=helpnewpage-helpactpage;

if (deltapage==0)//нет листания
  {
  //установка параметров страниц хелпа:
  helpactpagetest();//угол листа и видимость его и обложки
  goto end;
  }
//else делаем листание

deltagradus=-(  ( HELPGRAD*(long)cadrtime*(long)deltapage ) / HELPTIME  );
//защита от большого и малого поворота листа
if (deltagradus> MAXINT2) deltagradus= MAXINT2;
if (deltagradus<-MAXINT2) deltagradus=-MAXINT2;
if (deltagradus==0)       deltagradus= (deltapage>0)? -1:1;

//делаем верный стартовый угол очередного листания
if ((actgradus==0)&&(deltagradus>0)) actgradus=-HELPGRAD;
if ((actgradus==-HELPGRAD)&&(deltagradus<0)) actgradus=0;
//поворот листа
actgradus+=(int)deltagradus;

if (actgradus>=0)//предыдущая страница листа покрыла главную панель
  {
  //пролистываем одну или более страниц назад
  helpactpage-=1+actgradus/HELPGRAD;
  helpactpagetest();
  actgradus=0;
  }
else
if (actgradus<=-HELPGRAD)//следующая страница листа покрыла обложку
  {
  //пролистываем одну или более страниц вперёд
  helpactpage+=(-actgradus)/HELPGRAD;
  helpactpagetest();
  actgradus=0;
  }
else//лист ещё между обложкой и главной панелью
  {
  //текущая страница не меняется!
  if ( (helpactpage>1) || ((helpactpage==1)&&(deltapage>0)) )
    //обложка раскрыта  и  не закрывается
    {
    panel[helppan0].flag=YES;//обложка
    panel[helppan1].flag=YES;//и лист !!!
    }
  else//обложка закрывается или открывается
    {
    panel[helppan0].flag=NO;//вместо обложки:
    panel[helppan1].flag=YES;//лист, угол листа см выше !!!
    }
  //текущий угол панели листа:
  panel[helppan1].ap.angx=grad2ang(actgradus);
  goto end;
  }

//за HELPLAST страниц до окончания листания
if (abs(deltapage)<=HELPLAST) helptext();//выводим текущую страницу текста

end:
//определяем удаление юзера от хелпа
delta=DeltaD3poz(panel[helppan2].ear,PEN.ear);
dxyz=abs(fixtoi(delta.x0))+abs(fixtoi(delta.y0))+abs(fixtoi(delta.z0));
//при приближении юзера к хелпу раскрываем обложку
if ((dxyz<HELPDXYZ)&&(helpnewpage==0)) helpnewpage=1;
}

void AP_FirstMSG(int addr_AP,int msgrd)
//отрабатка прикладных сообщений AP_First;
//при вызове этой функции состояние AP_First ==AP_WORK !
{
int msgcmd,kbcod,hit,k;

msgcmd=msg[msgrd].cmd;
switch (msgcmd)//расшифровываем сообщение
  {
  case MSG_AP_STOP://приостановить работу AP
    AP[addr_AP].state=AP_STOP;
    break;

  case MSG_AP_INPUT_OFF://выход из AP
    break;

  case MSG_AP_INPUT_ON://выбор AP
    break;

  case MSG_KBPC_DATA:
    hit=msg[msgrd].data[0];
    kbcod=msg[msgrd].data[1];
    if (hit==KEYON)
      switch(kbcod)
        {
        //при установки лабиринта:
        //размах флуктуаций пикселей
        case KeyCodV: ++labrfluct; break;
        case KeyCodC: --labrfluct; break;
        //скорость скроллинга
        case KeyCodF: ++labdscrollx; break;
        case KeyCodD: --labdscrollx; break;
        //амплитуда волны градаций по x панели
        case KeyCodBackSlash: ++labgradampx; break;//это \
        case KeyCodInvCommas: --labgradampx; break;//это '
        //градация панели
        case KeyCodComma: ++labgradcon; break;
        case KeyCodPoint: --labgradcon; break;

        //изменения градаций от Y панели
        case KeyCodY: ++gradampy; break;//амплитуда
        case KeyCodH: --gradampy; break;
        case KeyCodU: gradleny+=stepvalue;//длина волны
          if (gradleny<1) gradleny=1; break;
        case KeyCodT: gradleny-=stepvalue;
          if (gradleny<1) gradleny=1; break;

        //прозрачность стекла всех панелей:
        case KeyCodPlus:  --neprozra; break;
        case KeyCodMinus: ++neprozra; break;
        //постоянная яркость всех панелей:
        case KeyCodRBracket: --gradconst; break;//это }
        case KeyCodLBracket: ++gradconst; break;//это {

        //регулировка смещения индекса группы панели лабиринта:
        case KeyCodAddMinus: --ap1iplus; break;
        case KeyCodAddPlus:  ++ap1iplus; break;

        //регулировка x и/или z размеров полной панели лабиринта:
        case KeyCodAdd4: ap1flenx=fixdiv(ap1flenx,FLAB); break;
        case KeyCodAdd6: ap1flenx=fixmul(ap1flenx,FLAB); break;
        case KeyCodAdd2: ap1flenz=fixdiv(ap1flenz,FLAB); break;
        case KeyCodAdd8: ap1flenz=fixmul(ap1flenz,FLAB); break;
        case KeyCodAdd1: ap1flenx=fixdiv(ap1flenx,FLAB);
                         ap1flenz=fixdiv(ap1flenz,FLAB); break;
        case KeyCodAdd9: ap1flenx=fixmul(ap1flenx,FLAB);
                         ap1flenz=fixmul(ap1flenz,FLAB); break;

        //загрузка ll-файла во временный лабиринт
        case KeyCodF4: if (Control==OFF) llfunct=1; break;//предыдущий файл
        case KeyCodF5: if (Control==OFF) llfunct=2; break;//текущий файл
        case KeyCodF6: if (Control==OFF) llfunct=3; break;//следующий файл

        //загрузка pp-файла в битмап стены
        case KeyCodF2: if (Control==OFF) ppfind=-1; break;//предыдущий файл
        case KeyCodF3: if (Control==OFF) ppfind=1; break;//следующий файл

        //вращение и перемещение системы AP
        case KeyCodRight:
        case KeyCodLeft:
          if (Control==ON)
            ap1dadt+= (kbcod==KeyCodLeft)? stepvalue:-stepvalue; break;
        case KeyCodUp:
        case KeyCodDown:
          if (Control==ON)
            ap1dydt+= (kbcod==KeyCodUp)? stepvalue:-stepvalue; break;

        //листаем хелп
        case KeyCodPgUp://предыдущая или первая страница
          if (Control==OFF) --helpnewpage;
          else              helpnewpage=1;
          break;
        case KeyCodPgDn://следующая или последняя страница
          if (Control==OFF) ++helpnewpage;
          else              helpnewpage=lasthelppage;
          break;

        //небольшое саунд-демо
        case KeyCodTilda: soundseed+= (Control==ON)? -1:1;
          printf("soundseed=%i\n",soundseed); break;//это ~
        case KeyCod1:
        case KeyCod2:
        case KeyCod3: k=kbcod-KeyCod1+2;
          if (Control==OFF) SoundPars(soundseed,7,k,64,0);
          else              SoundPars(soundseed,7,k,64,10); break;
        case KeyCod4:
        case KeyCod5:
        case KeyCod6: k=kbcod-KeyCod4+2;
          if (Control==OFF) SoundPars(soundseed,14,k,32,0);
          else              SoundPars(soundseed,14,k,32,10); break;
        case KeyCod7:
        case KeyCod8:
        case KeyCod9: k=kbcod-KeyCod7+2;
          if (Control==OFF) SoundPars(soundseed,28,k,32,0);
          else              SoundPars(soundseed,28,k,32,10); break;

        }
    else//KEYOFF
      switch(kbcod)
        {
        case KeyCodF1://дизайн хелпа, юзер к хелпу
          if (helppan2==ISNOT) HelpSetup();
          panel[helppan2].flag=YES;//видимость хелпа
          if ((Alter==ON)&&(Control==ON)) DocLoad();//грузим новый док-файл
          else//одна из них OFF
            {
            //меняем варианты bg/fg цветов хелпа
            if (Alter==ON)   ++helpvarbg;
            if (Control==ON) ++helpvarfg;
            }
          if (Shift==ON)
            {
            if (CapsLock==OFF) ++helpvarfg0;
            else               ++helpvarfg1;
            }
          //PORT: книжка появляется в 180 см впереди юзера, по центру
          //взгляда, на уровне глаз, лицом к юзеру; координаты юзера
          //не меняются (в оригинале был Bestpoz к хелпу).
          //ap панели - это координаты в системе AP_First, поэтому
          //земные координаты книжки конвертируем в систему AP_First!
          {
          d3poz delta=fixXYZang2D3poz(
            -fixdiv(panel[helppan2].flenx,itofix(2)),itofix(180),
            -fixdiv(panel[helppan2].flenz,itofix(2)),0);
          helppozusr=ConvD3pozFromEarth2AP(ShiftD3poz(delta,PEN.ear),
                                           AP[ADDR_APFirst].ear);
          helppozusrflag=YES;
          }
          helptext();//печатаем текущую страницу хелпа
          break;

        case KeyCodF9://сила динамической не-яркости;
          ++graddinflag;
          PEN.usershift=YES;//для смены яркости без движения
          break;

        //запрет и разрешение звука
        case KeyCodS: soundflag^=1; SoundOnOff(); break;

        //ЛилаМайя (эхо-зеркало на стене) !
        case KeyCodF11: VGAechoSwitch(); break;

        case KeyCodF12://флаг палитры
          if (++palfromppfile>=3)//восстанавливаем syspal
            {
            if (LoadSyspalFile("syspal.lii")!=OK) SyspalSetup();
            SetPalette(syspal,0,255);
            newbgpar=YES;//это для цветов 1...8;
            palfromppfile=0;
            }
          break;

        //[не]штатный рендер прозрачного цвета:
        case KeyCodI: if (++rendtransflag>2) rendtransflag=0; break;

        case KeyCodO: labglass^=1; break;//стекло лабиринта...

        case KeyCodF10: DeChoiceAP(); break;//отмена выбора AP

        //флаг учёта постоянных градаций панелей
        case KeyCodK: gradvarflag^=1; break;

        //флаг рендера градаций по x панелей
        case KeyCodL: gradvarflag^=2; break;

        //флаг рендера градаций от Y панели
        case KeyCodJ: gradvarflag^=4; break;

        //при установке лабиринта:
        //нулевая градация панели
        case KeyCodDiv: labgradcon=0; break;//это /
        //амплитуда=0 волны градаций по x панели
        case KeyCodSemicolon: labgradampx=0; break;
        case KeyCodB: labrfluct=0; break;//размах флуктуаций пикселей=0
        case KeyCodG: ScrollLab0(); break;//скорость скроллинга=0

        //отражение битмапа стены от середины по x или z
        case KeyCodM: MirrorBMAP(wallbitm,YES,NO); break;
        case KeyCodN: MirrorBMAP(wallbitm,NO,YES); break;

        //длина=~10 м, амплитуда=0 волны градаций от Y панели
        case KeyCodSpace: gradleny=1024; gradampy=0; break;

        //прозрачность и яркость 100%
        case KeyCodP: neprozra=0; gradconst=0; break;

        //нулевой сдвиг лабиринта от юзера:
        case KeyCodAdd0: llshift=d3null; break;
        //см также сдвиг лабиринта от 'оригина AP[ADDR_Labir]'
        //внутри функции ReadLLfile() !!!

        //фиксация смещения индекса группы панели лабиринта:
        case KeyCodAddEnter: ap1iplus= (ap1iplus==0)? 46:0; break;

        //начальные размеры полной панели лабиринта:
        case KeyCodAdd5: ap1flenx=FIX320; ap1flenz=FIX240; break;

        //циклическая загрузка ll-файлов
        case KeyCodF4: if (Control==ON) llfunct=(llfunct==10)? 0:10; break;
        case KeyCodF5: if (Control==ON) llfunct=(llfunct==11)? 0:11; break;
        case KeyCodF6: if (Control==ON) llfunct=(llfunct==12)? 0:12; break;

        case KeyCodF7://оставление временного лабиринта в мире напостоянку
          Lab2world();
          //при циклическом чтении продолжаем работу с лабиринтом
          if (llfunct<10) llstate=0;
          //следующий лабиринт сначала будет сцеплён с юзером !
          llshiftmode=2; break;

        case KeyCodF8://уничтожение временного лабиринта
          DelLab();
          llstate=0;//нет работы с лабиринтом
          //сброс функций циклического чтения ll-файла
          llfunct=0; break;

        //циклическая загрузка pp-файлов
        case KeyCodF2:
          if (Control==ON)
            {
            if (Alter==ON) WriteScreen();//экран в pp-файл!
            else ppfind=(ppfind==-2)? 0:-2;
            }
          break;
        case KeyCodF3: if (Control==ON) ppfind=(ppfind==2)?  0:2; break;

        case KeyCodTab: ClipSwitch(); break;//проницаемость панелей

        case KeyCodScrlLck:
          if (Control==OFF)
            {
            //переключение моды перемещения лабиринта
            if (++llshiftmode>2) llshiftmode=0;
            }
          else//[от]сцепление системы AP_First с юзером
            if (++ap1shiftmode>2) ap1shiftmode=0;
          break;

        //стоп вращения и перемещения AP
        case KeyCodDel: ap1dadt=ap1dydt=0; break;
        //начальная позиция системы AP
        case KeyCodHome:
          AP[addr_AP].ear=ap1startpoz;
          AP[addr_AP].earshift=YES;
          break;
        }
    break;
  }
}

void helptext(void)
//выводим текущую страницу текста хелпа
{
char far *helpbuf;//начало текста хелпа
char far *pgbuf;
long pglen;

//текущий вариант цветов хелпа, фон хелпа
helpfgbg();

helpbuf=(char far *)memory[helpmem].rampntr;
//начало текста страницы
pgbuf=helpbuf+shifthelppage[helpactpage];
//длина текста страницы
if (helpactpage<lasthelppage)
  pglen=(long)shifthelppage[helpactpage+1]-(long)shifthelppage[helpactpage];
else//последняя страница
  pglen=helpsize-(long)shifthelppage[helpactpage];

if (pglen>HELPPGLEN) pglen=HELPPGLEN;

//выводим страницу в три приёма:

//сначала все строки интеркопи-бумаги
helppage(0,pgbuf,pglen);

//потом поверх все строки интеркопи-литер
helppage(1,pgbuf,pglen);

//потом поверх все строки обычных литер
helppage(2,pgbuf,pglen);
}

void helppage(int type,char far *pgbuf,long pglen)
//выводим строки страницы хелпа согласно их типу type
{
int z,strlen,findlen,pglentoend,let,leti;
char far *newstr;
char far *oldstr;

//выводим страницу построчно сверху вниз
z=192;
oldstr=pgbuf;
pglentoend=(int)pglen;
while (pglentoend>0)
  {
  //ищем литеру CR
  findlen= (pglentoend<40)? pglentoend:40;
  newstr=memchr(oldstr,ASCIICR,findlen);
                     //нет литеры
  strlen= (newstr==NULL)? findlen:(int)(newstr-oldstr);
  if (strlen<=0) goto next;//пустая строка
  let=*oldstr;//первая литера строки
  leti=let2i(let);//её 65-ричный эквивалент
  switch(type)//тип нужной строки
    {
    case 0://строка интеркопи-бумаги
      if (let==ASCII32) break;
      if (leti>28) helpstring(oldstr,strlen,z);
      break;
    case 1://строка интеркопи-литер
      if (let==ASCII32) break;
      if (leti<=28) helpstring(oldstr,strlen,z);
      break;
    default:
    case 2://строка обычных литер
      if (let==ASCII32) helpstring(oldstr,strlen,z);
      break;
    }
next:
  //если CR была, пропускаем CR/LF:
  oldstr+= (newstr==NULL)? strlen:(strlen+2);
  //осталось до конца страницы
  pglentoend=(int)(pglen-(oldstr-pgbuf));
  z-=8;//опускаемся на строку ниже
  if (z<0) z=0;//лишние строки выводим поверх нижней!
  }
}

void helpfgbg(void)
//текущий вариант цветов хелпа, фон хелпа
{
int i,j,k,col,colp[2];

//зацикливание вариантов
if (helpvarfg0>15) helpvarfg0=0;
if (helpvarfg1>18) helpvarfg1=0;
if (helpvarfg>10) helpvarfg=0;
if (helpvarbg>12) helpvarbg=0;

//заполняем фон хелпа:
if (helpvarbg<7)
  {
  switch(helpvarbg)//одноцветный фон
    {
    case 0: col=0; break;
    case 1: col=gray[7]; break;
    case 2: col=gray[12]; break;
    case 3: col=red[8]; break;
    case 4: col=blu[8]; break;
    case 5: col=redblu[8]; break;
    case 6: col=grnblu[8]; break;
    }
  PaintFullBitmap(helpbitm,col);
  }
else//if (helpvarbg>=7)
  {
  switch(helpvarbg)//случайный фон (мелкая бумага)
    {
    case  7: colp[0]=red[7]; colp[1]=red1[7]; break;
    case  8: colp[0]=red[6]; colp[1]=red[7]; break;
    case  9: colp[0]=grnblu[7]; colp[1]=grnblu1[7]; break;
    case 10: colp[0]=grnblu[6]; colp[1]=grnblu[7]; break;
    case 11: colp[0]=blu[7]; colp[1]=blu1[7]; break;
    case 12: colp[0]=blu[6]; colp[1]=blu[7]; break;
    }
  RPBpixel(helpbitm,0,0,320,200,colp,0,1);
  }

if (helpvarbg>=7)//_добавляем_ спец ЛИЛА-фон
  {
  int c0,c1;
  switch(helpvarbg)
    {
    case  7: goto next;
    case  8: c0=red1[6]; c1=red1[7]; break;
    case  9: goto next;
    case 10: c0=grnblu1[6]; c1=grnblu1[7]; break;
    case 11: goto next;
    case 12: c0=blu1[6]; c1=blu1[7]; break;
    }
  for (j=0; j<200; j+=16)//литерная гравировка
    for (i=0; i<320; i+=32)
      {
      PaintText("Л",1,helpbitm,i+1,j,PAINTFG,c1,0);
               PaintText("Л",1,helpbitm,i,j,PAINTFG,c0,0);
      PaintText("И",1,helpbitm,i+9,j+4,PAINTFG,c1,0);
               PaintText("И",1,helpbitm,i+8,j+4,PAINTFG,c0,0);
      PaintText("Л",1,helpbitm,i+17,j+8,PAINTFG,c1,0);
               PaintText("Л",1,helpbitm,i+16,j+8,PAINTFG,c0,0);
      PaintText("А",1,helpbitm,i+25,j+12,PAINTFG,c1,0);
               PaintText("А",1,helpbitm,i+24,j+12,PAINTFG,c0,0);
      }
  }
next:

//делаем трафарет цветов 8x8 _не_ для интеркопи
for (j=0; j<8; ++j)
  for (i=0; i<8; ++i)
    {
    k=8+j+i;
    switch(helpvarfg)
      {
      default: break;//постоянные цвета не здесь
      case  3: col= (k<16)?    gray[k]:gray[31-k];   break;
      case  4: col= (k<16)? gray[17-k]:gray[k-14];   break;
      case  5: col= (k<16)?     red[k]:red[31-k];    break;
      case  6: col= (k<16)?  grnblu[k]:grnblu[31-k]; break;
      case  7: col= (k<16)?     blu[k]:blu[31-k];    break;
      case  8: col= (k<16)?     red[k]:blu[31-k];    break;
      case  9: col= (k<16)?     red[k]:grnblu[31-k]; break;
      case 10: col= (k<16)?     blu[k]:grnblu[31-k]; break;
      }
    col88[i*8+j]=col;
    }

//теперь готовим конверты вариантов интеркопи

//бумага
if (helpconv0<=ISNOT) return;

for (i=0; i<256; ++i)
  {
  j=i/8;
  k= (j<16)? j:(31-j);
  switch(helpvarfg0)
    {
    case  0: col=gray[1+i/43]; break;
    case  1: col=gray[4+i/43]; break;
    case  2: col=gray[7+i/43]; break;
    case  3: col=gray[10+i/43]; break;

    case  4: col= (k<4)? 0:gray[k];   break;
    case  5: col= (j<8)? 0:gray[j/2]; break;
    case  6: col= (k<4)? 0:gray[15-k];   break;
    case  7: col= (j<8)? 0:gray[15-j/2]; break;

    case  8: col= (k<4)? 0:red[k];   break;
    case  9: col= (j<8)? 0:red[j/2]; break;
    case 10: col= (k<4)? 0:grnblu[k];   break;
    case 11: col= (j<8)? 0:grnblu[j/2]; break;
    case 12: col= (k<4)? 0:blu[k];   break;
    case 13: col= (j<8)? 0:blu[j/2]; break;
    case 14: col= (k<4)? 0:redblu[k];   break;
    case 15: col= (j<8)? 0:redblu[j/2]; break;
    }
  memory[helpconv0].rampntr[i]=col;
  }

//текст
if (helpconv1<=ISNOT) return;

for (i=0; i<256; ++i)
  {
  j=i/8;
  k= (j<16)? j:(31-j);
  switch(helpvarfg1)
    {
    case  0: col= (j<8)? 0:gray[0]; break;

    case  1: col= (k<4)? 0:gray[k];   break;
    case  2: col= (j<8)? 0:gray[j/2]; break;
    case  3: col= (k<4)? 0:gray[15-k];   break;
    case  4: col= (j<8)? 0:gray[15-j/2]; break;
    case  5: col= (k<4)? 0:red[k];   break;
    case  6: col= (j<8)? 0:red[j/2]; break;
    case  7: col= (k<4)? 0:grnblu[k];   break;
    case  8: col= (j<8)? 0:grnblu[j/2]; break;
    case  9: col= (k<4)? 0:blu[k];   break;
    case 10: col= (j<8)? 0:blu[j/2]; break;
    case 11: col= (k<4)? 0:redblu[k];   break;
    case 12: col= (j<8)? 0:redblu[j/2]; break;

    case 13: if (j<16) col= (k<4)?      0:red[k];
             else      col= (k<4)? red[6]:red[k]; break;
    case 14: if (j<16) col= (k<4)?      0:grn[k];
             else      col= (k<4)? grn[6]:grn[k]; break;
    case 15: if (j<16) col= (k<4)?      0:blu[k];
             else      col= (k<4)? blu[6]:blu[k]; break;
    case 16: if (j<16) col= (k<4)?         0:redblu[k];
             else      col= (k<4)? redblu[6]:redblu[k]; break;
    case 17: if (j<16) col= (k<4)?         0:redgrn[k];
             else      col= (k<4)? redgrn[6]:redgrn[k]; break;
    case 18: if (j<16) col= (k<4)?         0:grnblu[k];
             else      col= (k<4)? grnblu[6]:grnblu[k]; break;
    }
  memory[helpconv1].rampntr[i]=col;
  }
}

void DocLoad(void)
//[пере]загрузка *.doc файла в mem-буфер
{
char far *helpbuf;//начало текста [хелпа]
char far *oldpage;
char far *newpage;
int pglen,findlen,hnd;
long helplentoend;
static int first=YES;

if (first==YES)//в самый первый раз
  {
  //грузим хелп док
  hnd=open("help.doc",O_RDONLY|O_BINARY,S_IREAD);
  first=NO;
  }
else//перебираем док файлы по одному
  {
  UnLockMEM(helpmem);//стираем старый блок с текстом доки
  hnd=OpenNextDOCfile(1);//открываем файл следующей доки
  }
if (hnd<0) return;//нет файла

//выделяем буфер текста размером с файл
helpsize=filelength(hnd);
if (helpsize<=0) goto end;
if (helpsize>MAXMEMLEN) helpsize=MAXMEMLEN;
helpmem=GetLockMEM(ADDR_APFirst,(word)helpsize);
if (helpmem<=ISNOT) { helpsize=0; goto end; }
//грузим текст
helpbuf=(char far *)memory[helpmem].rampntr;
Read(hnd,helpbuf,(word)helpsize);

//определение числа страниц и смещений страниц хелпа

shifthelppage[0]=0;//титульный лист
oldpage=helpbuf;
lasthelppage=0;
helplentoend=helpsize;
while (lasthelppage<HELPPAGE)
  {
  //ищем литеру конца страницы
  findlen= (helplentoend<HELPPGLEN)? (int)helplentoend:HELPPGLEN;
  newpage=memchr(oldpage,ASCIIFF,findlen);
  if (newpage==NULL)//нет литеры FF
    {
    pglen=findlen;
    oldpage+=pglen;
    }
  else//FF есть
    {
    //FF преобразовываем в точку
    *newpage='.';
    //пропускаем три литеры (FF/CR/LF)
    pglen=(int)(newpage-oldpage);
    oldpage+=pglen+3;
    }
  shifthelppage[++lasthelppage]=(word)(oldpage-helpbuf);
  //осталось до конца текста
  helplentoend=helpsize-(oldpage-helpbuf);
  //пустая страница
  if (helplentoend<=0) { --lasthelppage; break; }
  }
end:
close(hnd);
}

int OpenNextDOCfile(int napr)
//возвращает хендл открытого *.doc файла:
//при napr>0 следующего файла, при <0 предыдущего файла;
{
static struct ffblk fbuf;
static int start=YES,oldnapr=0;
static char oldfile[16];
char file[16];
int findok;

if (oldnapr<0) napr=0;

if (start==YES) findok= (findfirst("*.doc",&fbuf,0)==0)? YES:NO;
else
if (napr>0)
  {
  strcpy(oldfile,fbuf.ff_name);
  if (findnext(&fbuf)==0) findok=YES;
  else findok= (findfirst("*.doc",&fbuf,0)==0)? YES:NO;
  }

strcpy(file,fbuf.ff_name);
if ((napr<0)&&(start==NO)) strcpy(file,oldfile);

if (findok==NO) { start=YES; return -1; }
oldnapr=napr;
start=NO;
return open(file,O_RDONLY|O_BINARY,S_IREAD);
}

void helpstring(char far *hstr,int strlen,int z)
//печать строки hstr длиной strlen на высоте z битмапа хелпа
//позже разбить ф-ю на 2-3 варианта согласно типу...
{
int bm1,bm2,lenx1,lenz1,lenx2,lenz2,step,randfl;
long seed;

if (*hstr==ASCII32)//строка начинается с пробела
  //вариант печати без интеркопи
  {
  switch(helpvarfg)
    {
    case 0: PaintText(hstr,strlen,helpbitm,0,z,PAINTFG,gray[15],0); break;
    case 1: PaintText(hstr,strlen,helpbitm,0,z,PAINTFG,gray[ 0],0); break;
    case 2: PaintText(hstr,strlen,helpbitm,0,z,PAINTFG,       0,0); break;
    default: PaintText88(hstr,strlen,helpbitm,0,z,PAINTFG,col88,0); break;
    }
  return;
  }
//else не с пробела

//вариант печати для интеркопи !!!

//строка должна начинаться с цифры шага интерполяции
step=let2i(*hstr);
if (step<=28) randfl=NO;
else//признак случайной интеркопи-текстуры
  {
  randfl=YES;
  step-=27;//т.о. литера T==29 -> шаг 2
  }
step=max(step,2);
step=min(step,28);//более 28: даже 1 буква не помещается в 64 к!
//перемещаем начало строки на 1 литеру
++hstr;
if (--strlen<=0) return;
if (strlen>20) strlen=20;
//битмап для одной строки с strlen литерами
//и 1 пикселем запаса по всем краям:
lenx1=8*strlen+2;
lenz1=8+2;
bm1=GetLockMEM2(0,lenx1,lenz1,0,0);
//битмап для интеркопи:
lenx2=lenx1*step-(step-1);
lenz2=lenz1*step-(step-1);
bm2=GetLockMEM2(0,lenx2,lenz2,0,0);
//делаем текстуру bm1 и интерполяцию в bm2
SetMEMtoConstval(bm1,0);
if (randfl==NO)//текстура текста
  {
  PaintText(hstr,strlen,bm1,1,1,PAINTFG,255,0);
  //интерполяция
  InterCopyBitmap(bm2,bm1,step);
  //перекрашиваем интерполяцию в нужные цвета текста
  RepaintBitmap(bm2,memory[helpconv1].rampntr);
  //копируем результат в текстуру хелпа с порогом 1
  //для некопирования дыр в тексте
  CopyBitmap(helpbitm,0,z,bm2,0,0,lenx2,lenz2,1);
  }
else//~случайная текстура бумаги
  {
  int i,let,rcol[32];
  for (i=0; i<32; ++i) rcol[i]=8*i+7;
  //~политерно красим случайно пиксели
  for (i=0; i<strlen; ++i)
    {
    let=hstr[i];
    if (let==ASCII32) continue;
    seed=let;//зерно зависит от самой литеры!
    SRPBpixel(&seed,bm1,1+i*8,1,8+i*8,8,rcol,0,31);
    }
  //интерполяция
  InterCopyBitmap(bm2,bm1,step);
  //перекрашиваем интерполяцию в нужные цвета бумаги
  RepaintBitmap(bm2,memory[helpconv0].rampntr);
  //копируем результат в текстуру хелпа с порогом 1
  //для некопирования дыр в бумаге
  CopyBitmap(helpbitm,0,z,bm2,0,0,lenx2,lenz2,1);
  }
//открепляем блоки (в обратном порядке)
UnLockMEM(bm2);
UnLockMEM(bm1);
}

