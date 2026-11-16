/*
    СИ-код фильтра системы (с ADDR_VRS),
    др. системных и некоторых AP-функций.
*/

#include "vrsport.h"
#include    "vrs57.h"
#include "kbcod.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

void DeChoiceAP(void);
void ClipSwitch(void);
void ClipComput(void);

//внутренние функции модуля, не включённые в h-файл
void PrintSystemMessages(void);
void UserMousePars(void);
void UserSpaceShift(void);
void PencilMousePars(void);
void CursorMousePars(void);
void PencilSpaceShift(void);
void CursorSpaceShift(void);
int FindNearPanel(void);
void ChoiceAP(int apnum);
void Exitpan(void);


//чувствительность перемещений юзера к вводу клавиатуры
#define KBDSHIFTMULX  (FIX1/2)
#define KBDSHIFTMULY  (FIX1)
#define KBDSHIFTMULZ  (FIX1/4)
#define KBDSHIFTMULA   8//умножение
// --//-- к вводу мыши
#define MOUSHIFTMULX  (FIX1/64)
#define MOUSHIFTMULY  (FIX1/32)
#define MOUSHIFTMULZ  (FIX1/128)
#define MOUSHIFTDIVA   2//деление !

#define FDTMUL (FIX1+FIX1/4)//=1.25, коэфф-т mul/div для dxdt, dydt, dadt


int speedprint=NO;//флаг печати частоты кадров;
int printsize,printpan=ISNOT;//номер размера и номер панели печати координат
int exitflag=0,exitpan=ISNOT;//флаг и номер панели для выхода из Lila
int usermmm;//user moves mouse mode
//накопители смещений юзера по вводу клавиатуры и мыши
fix inputda,inputdx,inputdy,inputdz;
//смещения юзера из-за ввода клавиатуры и мыши за кадр
fix userdx,userdy,userdz,userda;
fix dxdt,dydt,dzdt,dadt;//накопленные ~скорости смещений юзера
int Fdz=6;//частота колебаний юзера по z-высоте = 2^(Fdz-6) Гц
fix old_dz;//старый сдвиг по высоте из-за dzdt;
int lctrlflag=KEYOFF;//индикатор нажатого состояния LeftCtrl
int rctrlflag=KEYOFF;//--//-- RightCtrl
int laltflag=KEYOFF,raltflag=KEYOFF;//L/R Alt
int lshftflag=KEYOFF,rshftflag=KEYOFF;//L/R Shift


//расстояния от юзера до (ближайшего) объекта
//впереди: fdy0=слева, 1=в центре (min по вертикали!), 2=справа!
fix fdy0=MAXLONG,fdy1=MAXLONG,fdy2=MAXLONG;


void DeChoiceAP(void)
//отмена выбора AP, но без мессага !
{
if (curnumap==ADDR_VRS) return;
//else отмена выбора AP
Sound(SNDVRSIN);
curnumap=ADDR_VRS;//нет выбранной AP
curnumpan=ISNOT;//нет выбранной панели
pointermode=0;//переключение курсор->карандаш
}

void ClipSwitch(void)
//переключение флага клиппинга
{
clip=(++clip)&1;
//для безошибочного клиппинга все дальности сначала максимальны
fdy0=fdy1=fdy2=MAXLONG;
}

#define USERWIDTH2  16//полуширина (и полувысота) юзера для клиппинга,
//в пикселях PEN-bmp (для ширины экрана 320, пропорц-но penbmplenx);
void ClipComput(void)
//определение удаления пикселей вблизи центра экрана;
//чтобы не ударяться о близко приклееные к юзеру панели,
//надо рендерить все эти панели _после_ ClipComput()!!!
{
//юзер простирается по ширине [][0...2] и по высоте [0...2][] !
static int yb[3][3];//~расстояние до пикселя экрана:
//[*][0]=слева, [*][1]=в центре, [*][2]=справа по ширине,
//[0][*]=снизу, [1][*]=в центре, [2][*]=сверху по высоте;
fix dy[3][3];//истинное расстояние в fix-единицах;
int i,j,i0,lenx,lenx2,lenz2,userw2,y;
//PORT: в DOS int был 16-битный; y-буфер пишется рендером как short,
//поэтому и читать его здесь нужно как short (как в vrs58s.c)!
short far *ybuffer;

//полуширина юзера с учетом размера экрана
userw2=(USERWIDTH2*penbmplenx)/320;
lenx=partpenbmplenx;//с учетом многошагового рендера!
lenx2=lenx/2;
lenz2=penbmplenz/2;
ybuffer=(short far *)memory[PEN.ymem].rampntr;

//запоминаем закодированные расстояния
//PEN.ymem имеет моду 0: слева вправо, потом вверх

if (numpart==1)//одношаговый рендер
  {
  for (j=0; j<=2; ++j)//цикл по вертикали
    {
    i0=(lenz2+(j-1)*userw2)*lenx+lenx2-userw2;
    for (i=0; i<=2; ++i)//цикл по горизонтали
      yb[j][i]=ybuffer[i0+i*userw2];
    }
  }

if (numpart==2)//двухшаговый рендер
  {
  if (actpart==0)//1-й шаг
    {
    for (j=0; j<=2; ++j)//цикл по вертикали
      {
      i0=(lenz2+(j-1)*userw2)*lenx+lenx-1-userw2;
      for (i=0; i<=1; ++i)//цикл по горизонтали
        yb[j][i]=ybuffer[i0+i*userw2];
      }
    }
  else//2-й шаг
    {
    for (j=0; j<=2; ++j)//цикл по вертикали
      {
      i0=(lenz2+(j-1)*userw2)*lenx+userw2;
      yb[j][2]=ybuffer[i0];
      }
    }
  }

//все готово: одношаговый рендер или 2-й шаг двухшагового рендера
if ((numpart==1)||((numpart==2)&&(actpart==1)))
  {
  //вычисляем действительные расстояния
  for (i=0; i<=2; ++i)
    {
    for (j=0; j<=2; ++j)//цикл по вертикали!
      {
      y=yb[j][i];
      if (y>0) dy[j][i]=itofix(y);
      else//отрицательное расстояние - область растянутых y
        dy[j][i]=DYFG-((-(long)y)<<YFGSHIFT);
      }

    //находим ближайший пиксель из 3-х вертикальных
    dy[0][i]=min(dy[0][i],dy[1][i]);
    dy[0][i]=min(dy[0][i],dy[2][i]);
    }
  //выходная горизонталь из минимумов
  fdy0=dy[0][0];
  fdy1=dy[0][1];
  fdy2=dy[0][2];
  }
}

#define CLIPMINY  (20*FIX1)//расстояние клиппинга (min дистанция до
//непрозрачного объекта спереди от юзера)!
//процент конвертации невозможного сдвига юзера вперед:
#define CLIPSHIFT   5//в боковой сдвиг
#define CLIPBACK   10//в отскакивание назад
void UserSpaceShift(void)
//перемещает юзера с учетом клиппинга
{
fix dymin,shift,dshift,dx0;
d3poz delta;
int uklon;

//delta = смещение USER-системы (наблюдателя) в USER-системе,
//преобразуется из USER-системы в систему земли и присваивается
//обратно USER-системе в два приема - для верного клиппинга !

//сначала линейное смещение наблюдателя
if ((userdx!=0)||(userdy!=0)||(userdz!=0)||(dxdt!=0)||(dydt!=0)||(clip==YES))
  {
  //статический сдвиг должен быть таким
  delta=fixXYZang2D3poz(userdx,userdy,userdz,0);
  //смещения из-за линейных скоростей юзера
  delta.x0+=10*dxdt*cadrtime;
  delta.y0+=10*dydt*cadrtime;

  if ((clip==YES)&&(delta.y0>=0))//если нужен клиппинг и юзер
    //сам не пятится назад, то смотрим на препятствия спереди
    {
    int numa,numb;//сортированные по расстояниям номера точек:
      //numa=номер ближайшей точки (0...2), numb=номер следующей;

    //ближайший к юзеру объект на расстоянии dymin
    dymin=min(fdy0,fdy1);
    if (dymin==fdy0) numa=numb=0;
    else             numa=numb=1;

    dymin=min(dymin,fdy2);
    if (dymin==fdy2) numa=2;
    else//fdy2 не минимум, надо скорректировать numb:
      {
      if (numa==0)//сравниваем точки 1 и 2
        numb= (fdy1<fdy2)? 1:2;
      else//numa=1, сравниваем точки 0 и 2
        numb= (fdy0<fdy2)? 0:2;
      }

    //линейные смещения юзера из-за движения его или объекта
    if (dymin<CLIPMINY)//объект ближе допустимого
      {
      Sound(SNDTOLHOK);
      dshift=delta.y0;//(не отрицательная!) длина усеченного сдвига вперед
      //'предварительный сдвиг вперед' будет сдвигом назад к точке
      delta.y0=dymin-CLIPMINY;//мин. допустимого расстояния до объекта
      }
    else//объект позволяет сделать некоторый сдвиг вперед
    if ((dymin-delta.y0)<CLIPMINY)
      //объект будет ближе допустимого после сдвига
      {
      shift=dymin-CLIPMINY;//'предварительный сдвиг вперед'
      dshift=delta.y0-shift;//длина усеченного сдвига вперед
      delta.y0=shift;
      }
    else dshift=0;//нет корректировки смещения юзера

    if (dshift>0)//есть корректировка
      {
      Sound(SNDUDAR);
      //определяем боковой уклон объекта перед юзером
      if (fdy0==fdy2)//если левый и правый край на одинаковом y
        uklon=0;//уклона нет!
      else//уклон есть:
        {
        //два случая _одинаково ближайших_ центра и одного края
        if ((fdy0==fdy1)&&(fdy1<fdy2)) uklon=+1;//левый край ближе
        else
        if ((fdy1==fdy2)&&(fdy1<fdy0)) uklon=-1;//правый край ближе
        else//все остальные случаи
          {
          //определяем уклон по _номерам_ ближайших точек
          if (numa<numb) uklon=+1;//левый край ближе
          else           uklon=-1;//правый край ближе
          }
        }

      //добавляем сдвиг в сторону
      dx0=(dshift*CLIPSHIFT)/100;//величина сдвига в сторону
      if (uklon>0) delta.x0+=dx0;//сдвиг вправо
      if (uklon<0) delta.x0-=dx0;//сдвиг влево

      //'окончательный сдвиг вперед'
      delta.y0-=(dshift*CLIPBACK)/100;//минус отскакивание назад
      }
    }

  //сдвиг USER-системы на вектор delta
  PEN.ear=ShiftD3poz(delta,PEN.ear);
  PEN.usershift=YES;//координаты USER-системы изменились!

  //смещение юзера для 'плаванья по цветам' земли:
  DYuser=delta.y0;
  }
else DYuser=0;

//затем делаем поворот наблюдателя
if ((userda!=0)||(dadt!=0))
  {
  static int ost_da,angx2bit;
  angle angx;
  fix da;
  //восстанавливаем младшие 2 бита угла юзера для плавности
  angx=PEN.ear.angx|angx2bit;
  da=dadt*cadrtime;
  ost_da+=(int)(da-(da/256)*256);
  da/=256;
  if (ost_da>=+256) { ++da; ost_da-=256; }
  if (ost_da<=-256) { --da; ost_da+=256; }
  //полное вращение угла юзера
  angx+=(angle)(userda+da);
  angx2bit=angx&3;//новые 2 бита
  //огрубляем угол юзера до ANGSTEP, иначе щели панелей
  PEN.ear.angx=angx&0xFFFC;//могут значительно увеличиться!
  PEN.usershift=YES;
  }

if ((dzdt!=0)||(old_dz!=0))//добавляем колебания по высоте:
  {
  fix act_dz;
  //word vrstime0 это младшие 16 бит времени в миллисекундах!
  act_dz=dzdt*(fsinus(vrstime0<<Fdz)/50);
  //при Fdz=6 частота z-колебаний=(1 за 1024 мсек)=~1 Герц!
  PEN.ear.z0+=act_dz-old_dz;
  PEN.usershift=YES;
  old_dz=act_dz;
  }
}

void CursorSpaceShift(void)
{
d3poz delta;

if (curnumpan<=ISNOT) return;

//у[вос]станавливаем флаги видимости обоих панелей указки
panel[curpan].flag=curvisible;//курсор переключает сама AP!
panel[pnclpan].flag=NO;
//текущие размеры курсора
panel[curpan].flenx=fcurlenx;
panel[curpan].flenz=fcurlenz;
//масштабные множители битмап->панель курсора
panel[curpan].fmulx=fcurlenx/memory[curbm].lenx;
panel[curpan].fmulz=fcurlenz/memory[curbm].lenz;
//вектор сдвига оригина выбранной панели в оригин курсора
delta.x0=fcurstepx*curpanx;
delta.y0=-fcurpany;
delta.z0=fcurstepz*curpanz;
delta.angx=0;
//чтобы курсор не плясал из-за перемещения юзера - определяем
//pen-координаты выбранной панели по последним координатам юзера
panel[curnumpan].pen=ConvD3pozFromEarth2AP(panel[curnumpan].ear,PEN.ear);
//теперь pen-координаты курсора
panel[curpan].pen=ShiftD3poz(delta,panel[curnumpan].pen);

//если выбранная панель в мире, но невидима для рендера
if ((panel[curnumpan].flag==YES)&&(panel[curnumpan].render==NO))
  //возвращаем юзера в позицию наблюдения панели
  if (autobestpoz==ON) Bestpoz(USER2PAN,bestpozmode,curnumpan);
}

void CursorMousePars(void)
{
if (curnumpan<=ISNOT) return;

//перемещение курсора по выбранной панели AP, только если она есть
//и в мире, и на экране
if ((panel[curnumpan].flag==YES)&&(panel[curnumpan].render==YES))
  {
  curpandx=mousedx;  curpandz=mousedy;
  curpanx+=curpandx; curpanz+=curpandz;
  curk0=mouseb0;     curk1=mouseb1;
  curk0on=mouseb0on; curk0off=mouseb0off;
  curk1on=mouseb1on; curk1off=mouseb1off;
  }
}

void UserMousePars(void)
//параметры сдвига юзера благодяря мыши
{
static int gradflag=OFF;
int napr;

if (mouseb1off>0)
  {
  if (gradflag==OFF) usermmm^=1;
  else               gradflag=OFF;
  }

if ((mouseb0==0)&&(mouseb1!=0))
  {
  if (abs(mousedy)>=10)
    {
    gradflag=ON;
    PEN.zgrad+=mousedy/10;
    PEN.pozshift=YES;
    }
  mousedx=mousedy=0;
  }

napr= (mouseb1==0)? 1:-1;

if (usermmm==0)//режим 0
  {
  inputda=mousedx;//поворот по горизонтали
  inputdy=mousedy;//движение вперед-назад
  //по b0 добавляем 'авто-движение в направлении' вперёд-назад
  if (mouseb0!=0) inputdy+=napr*cadrtime;
  }
else//режим 1
  {
  inputdx=mousedx;//движение влево-вправо
  inputdz=mousedy;//движение вверх-вниз
  //по b0 добавляем 'авто-движение в направлении' вверх-вниз
  if (mouseb0!=0) inputdz+=napr*cadrtime;
  }
}

int FindNearPanel(void)
//поиск панели вблизи карандаша, возвращает её номер или ISNOT;
//панели системы (ADDR_VRS) и псевдо-AP всегда игнорируются !!!
{
long r2,minr2,maxr2,dx,dy,dz;
d3poz pnclo,panelc;//позиции острия карандаша и центра панели
d3poz delta;//сдвиг центра панели от оригина её
int pan,n;

mouseb0on=mouseb0off=mouseb1on=mouseb1off=0;
mousedx=mousedy=0;//сброс всех кликов и перемещений

pan=ISNOT;//нет выбранной панели
//поиск и выбор панели (все координаты в PEN системе):
//это панель с min квадратом расстояния от своего центра до острия
//карандаша и не более PNCLMAXR^2, если только она видима на экране !
pnclo=panel[pnclpan].pen;
pnclo.z0+=pncldz*panel[pnclpan].fmulz;
minr2=MAXLONG;
maxr2=PNCLMAXR*PNCLMAXR;
for (n=0; n<=maxpannum; ++n)
  {
  //игнорируем панели:
  if (panel[n].flag==NO) continue;//отсутствующие в мире, это надо
  //проверять т.к. при flag=NO могут остаться старые флаги render=YES
  if (panel[n].render==NO) continue;//невидимые сейчас
  if ((panel[n].owner<=ADDR_VRS)||  //системные и панели
      (panel[n].owner> ADDR_LAST_AP)) continue;//псевдо-AP: все!
  delta.x0=panel[n].flenx/2;
  delta.y0=0;
  delta.z0=panel[n].flenz/2;
  delta.angx=0;
  panelc=ShiftD3poz(delta,panel[n].pen);
  dx=fixtoi(pnclo.x0-panelc.x0);
  dy=fixtoi(pnclo.y0-panelc.y0);
  dz=fixtoi(pnclo.z0-panelc.z0);
  r2=dx*dx+dy*dy+dz*dz;
  if (r2>maxr2) continue;
  if (r2<minr2) { minr2=r2; pan=n; }
  }
return pan;
}

void ChoiceAP(int apnum)
//активизация AP=apnum и сообщение о выборе в AP
{
if ((apnum<=ADDR_VRS)||(apnum>ADDR_LAST_AP)) return;

AP[apnum].taskflag=ON;//включение выбранной AP в виток
//проверяем текущее состояние и активизируем AP
if (AP[apnum].state==AP_STOP)
  AddShortMSG(ADDR_VRS,apnum,MSG_AP_WSTART);
else
if (AP[apnum].state==AP_START)
  AddShortMSG(ADDR_VRS,apnum,MSG_AP_CSTART);
//теперь сообщаем о выборе панели
AddShortMSG(ADDR_VRS,apnum,MSG_AP_INPUT_ON);

curnumap=apnum;//курсор переводим на AP
//curnumpan должен устанавливаться ( =ISNOT или >ISNOT) до того !
pointermode=1;//карандаш->курсор AP: если нет выбранной панели,
//то вместо карандаша и курсора мышь будет перемещать юзера!
curmousemode=0;//мышь будет перемещать юзера, иное установит AP !
}

void SystemMessageFiltr(void)
//фильтр и отработка системных сообщений, напр.:
//-выход из Lila;
//-перемещение наблюдателя (который расположен в оригине отдельной
// USER-системы и смотрит по оси +y);
{
int kbcod,hit,msgrd,msgwr;//для локального сканирования буфера сообщений
int n;

inputda=inputdy=inputdx=inputdz=0;//обнуляем все счетчики перемещений

//читаем буфер сообщений
msgrd=msgread;
msgwr=msgwrite;
while (msgrd!=msgwr)//сканируем весь буфер сообщений
  {
//  printf("MSG=%i  readpoz=%i\n",msg[msgrd].cmd,msgrd);

  //если адрес получателя не равен адресу VRS, и если сообщение
    //предназначено не для всех, то пропуск его
  if ((msg[msgrd].dstaddr!=ADDR_VRS)&&(msg[msgrd].dstaddr!=ADDR_ALL))
    goto nextmsg;

  //расшифровываем сообщение
  switch (msg[msgrd].cmd)
    {
    default: break;

    case MSG_AP_INPUT_ON://пока у VRS нет панелей для выбора
      //просто отменяем выбор через имитацию Esc на след-м витке:
      Add2byteMSG(ADDR_VRS,ADDR_VRS,MSG_KBPC_DATA,KEYOFF,KeyCodEsc);
      break;
    case MSG_AP_INPUT_OFF: break;

    case MSG_KBPC_DATA:
      //анализируем сообщение данных с клавиатуры
      hit=msg[msgrd].data[0];
      kbcod=msg[msgrd].data[1];
      //стирание exitpan по отжатию любой клавиши кроме Esc и F10
      if ((exitpan!=ISNOT)&&(hit==KEYOFF)&&
          (kbcod!=KeyCodEsc)&&(kbcod!=KeyCodF10))
        {
        Sound(SNDVRSIN);
        UnLockPAN(exitpan);
        exitpan=ISNOT;
        exitflag=0;//отмена выхода из Lila
        }
      switch(kbcod)
        {
        //этот хоткей всегда у системы и только у системы !!!
        case KeyCodEsc://отмена выбора AP и др.
          if (hit!=KEYOFF) goto nextmsg;
          //через Control триггеруем видимость лейбла
          if (Control==ON) { logovisible^=1; goto nextmsg; }
          if (curnumap!=ADDR_VRS)//ввод у AP: отмена выбора AP
            {
            //сначала добавляем мессаг для AP
            AddShortMSG(ADDR_VRS,curnumap,MSG_AP_INPUT_OFF);
            DeChoiceAP();//там сидит Sound(SNDVRSIN) !
            goto nextmsg;
            }
          //ввод у VRS
          if (exitpan!=ISNOT)//есть exitpan: стирание её
            {
            Sound(SNDVRSIN);
            UnLockPAN(exitpan);
            exitpan=ISNOT;
            exitflag=0;//отмена выхода из Lila
            }
          else//нет exitpan: вывод её
            {
            Exitpan();//вывод exitpan
            exitflag=1;//готовность к выходу из Lila
            }
          goto nextmsg;

        //отслеживание состояний общефункц-х клавиш:

        case KeyCodCapsLock: if (hit==KEYOFF) CapsLock^=1;
          //лейбл показывает состояние CapsLock
          logovisible= (CapsLock==OFF)? OFF:ON; break;

        case KeyCodLCtrl:  case KeyCodRCtrl:
          if (kbcod==KeyCodLCtrl) lctrlflag=hit; else rctrlflag=hit;
          if ((lctrlflag==KEYON)||(rctrlflag==KEYON)) Control=ON;
          else Control=OFF; break;

        case KeyCodLAlt:   case KeyCodRAlt:
          if (kbcod==KeyCodLAlt) laltflag=hit; else raltflag=hit;
          if ((laltflag==KEYON)||(raltflag==KEYON)) Alter=ON;
          else Alter=OFF; break;

        case KeyCodLShift: case KeyCodRShift:
          if (kbcod==KeyCodLShift) lshftflag=hit; else rshftflag=hit;
          if ((lshftflag==KEYON)||(rshftflag==KEYON)) Shift=ON;
          else Shift=OFF; break;

        }

      if (curnumap!=ADDR_VRS)//ввод подключен не к VRS, а к AP !
        {
        msg[msgrd].dstaddr=curnumap; goto nextmsg;//переадресуем код в AP
        }
      //else ввод у VRS

      switch(kbcod)//VRS переадресует эти хоткеи в AP
        {
        case KeyCodDiv:
        case KeyCodPoint:
        case KeyCodComma: msg[msgrd].dstaddr=ADDR_APRend; goto nextmsg;

        case KeyCodF1: case KeyCodF4: case KeyCodF7:
        case KeyCodF2: case KeyCodF5: case KeyCodF8:
        case KeyCodF3: case KeyCodF6: case KeyCodF9:
        case KeyCodPlus:
        case KeyCodMinus:
        case KeyCod0:
        case KeyCodPgUp:
        case KeyCodPgDn:  msg[msgrd].dstaddr=ADDR_APFirst; goto nextmsg;
        }

      if (hit==KEYON)
        switch(kbcod)
          {
          default: break;

          case KeyCodAdd9://будет перерасчет xbe _всех_ панелей !
            ++PEN.zgrad; PEN.pozshift=YES; break;//скос взгляда вверх
          case KeyCodAdd3: --PEN.zgrad; PEN.pozshift=YES; break;//вниз

          //различные хоткеи перемещения юзера, зависят от
          //CapsLock, Shift, Control, Alter
          case KeyCodRight: case KeyCodLeft:
          case KeyCodUp:    case KeyCodDown:
          case KeyCodAdd2:  case KeyCodAdd4:
          case KeyCodAdd6:  case KeyCodAdd8:
            if (Control==OFF)
              switch(kbcod)//перемещения
                {
                case KeyCodUp:   inputdy+=stepvalue; break;
                case KeyCodDown: inputdy-=stepvalue; break;

                case KeyCodAdd6: inputdx+=stepvalue; break;
                case KeyCodAdd4: inputdx-=stepvalue; break;

                case KeyCodAdd8: inputdz+=stepvalue; break;
                case KeyCodAdd2: inputdz-=stepvalue; break;

                case KeyCodRight: inputda+=stepvalue; break;
                case KeyCodLeft:  inputda-=stepvalue; break;
                }
            else//Control==ON
              switch(kbcod)// ~скорости
                {
                case KeyCodUp:   dydt+=stepvalue; break;
                case KeyCodDown: dydt-=stepvalue; break;

                case KeyCodAdd6: dxdt+=stepvalue; break;
                case KeyCodAdd4: dxdt-=stepvalue; break;

                case KeyCodAdd8: dzdt+=stepvalue; break;
                case KeyCodAdd2: if ((dzdt-=stepvalue)<0) dzdt=0; break;

                case KeyCodLeft:  dadt+=stepvalue; break;
                case KeyCodRight: dadt-=stepvalue; break;
                }
            break;

          //пропорциональное изменение скоростей, кроме dzdt
          case KeyCodAddPlus:
            dxdt=fixmul(dxdt,FDTMUL); dydt=fixmul(dydt,FDTMUL);
            dadt=fixmul(dadt,FDTMUL); break;
          case KeyCodAddMinus:
            dxdt=fixdiv(dxdt,FDTMUL); dydt=fixdiv(dydt,FDTMUL);
            dadt=fixdiv(dadt,FDTMUL); break;

          }

      if (hit==KEYOFF)
        switch(kbcod)
          {
          //запрет и разрешение звука
          case KeyCodS: soundflag^=1; SoundOnOff(); break;

          //частота колебаний юзера по высоте
          case KeyCod1: case KeyCod2: case KeyCod3:
          case KeyCod4: case KeyCod5: case KeyCod6:
          case KeyCod7: case KeyCod8: case KeyCod9:
            Fdz=kbcod-KeyCod1+1; break;

          case KeyCodAdd0://перемещение юзера в самый центр мира,
            USERandPENstartpoz(); break;//с прямым взглядом на север

          case KeyCodAdd5://юзер смотрит точно на 0,90,180,270 градусов
            {static int sw; sw=++sw%4; PEN.ear.angx=ANG90G*sw;}
            PEN.usershift=YES; break;

          case KeyCodAdd7://скос взгляда=-30,0,+30 град.
            {static int sw; sw=++sw%3; PEN.zgrad=30*(sw-1);}
            PEN.pozshift=YES; break;

          case KeyCodAddDel://юзер на уровень земли/роста/двойного роста
            {static int sw; sw=++sw%3; PEN.ear.z0=FUSERZ0*sw;}
            PEN.usershift=YES; break;

          case KeyCodAddEnter://сброс всех скоростей юзера в ноль
            dxdt=dzdt=dydt=dadt=0; break;

          case KeyCodEnter://поиск панели и 'выбор'
            curnumpan=FindNearPanel();
            //близкая панель найдена: 'выбор' AP
            if (curnumpan>ISNOT) ChoiceAP(panel[curnumpan].owner);
            break;

          case KeyCodScrlLck://сцепка системных панелей с юзером
            locksyspan2user^=1; break;

          case KeyCodBS://операции с карандашем
            if (Control==ON)
              {
              pnclvisible^=1;//не-видимость
              //невидимый карандаш лишается мыши
              if (pnclvisible==NO) pnclmousemode=0;
              }
            else//Control=OFF
            if (Alter==OFF)//перемещение мышью юзера или карандаша
              {
              pnclmousemode^=1;
              //карандаш с мышью будет виден
              if (pnclmousemode==1) pnclvisible=YES;
              }
            else//смена фиксир. позиции карандаша
              {
              pnclpozflag=3&++pnclpozflag; PencilPoz(pnclpozflag);
              pnclvisible=YES;//карандаш виден
              }
            break;

          case KeyCodAddMul: CompressMEM(); break;//сжатие памяти!

          case KeyCodAddDiv://порт: проверка насыщения fixdiv вместо int0
            printf("fixdiv saturate=%li !\n",lfixmuldiv(MAXLONG,MAXLONG,1));
            WKP(); break;

          case KeyCodTab: ClipSwitch(); break;//проницаемость панелей

          case KeyCodF: speedprint^=1; break;//печать частоты кадров

          //[не]видимость всех панелей Первой ПП или системных
          case KeyCodIns: case KeyCodDel:
            if (Control==OFF)
              {
              for (n=0; n<=maxpannum; ++n)
                if (panel[n].owner==ADDR_APFirst)
                  {
                  panel[n].flag= (kbcod==KeyCodIns)? YES:NO;
                  panel[n].earvalid=NO;//надо, если ap сдвинулась
                  //и остановилась во время невидимости панелей !
                  }
              }
            else
              {
              for (n=0; n<=maxpannum; ++n)
                if (panel[n].owner==ADDR_VRS)
                  {
                  panel[n].flag= (kbcod==KeyCodIns)? YES:NO;
                  panel[n].penvalid=NO;
                  }
              }
            break;

          case KeyCodP://вкл-откл панели печати координат юзера
            if (printpan==ISNOT)//нет панели и т.о. печати
              {
              printsize^=1;
              locksyspan2user=ON;
              //выделяем панель для печати
              if (printsize==0)
                {
                printpan=GetPANtxt(ADDR_VRS,4,28,1,100);
                pangran(printpan,4,GALL,&gray[9],0,0,0);//рамка неплавная
                }
              else
                {
                printpan=GetPANtxt(ADDR_VRS,6,28,4,140);
                pangran(printpan,6,GALL,blu,12,0,-1);
                }
              }
            else//есть панель
              {
              //удаляем панель и её ресурсы
              UnLockPAN(printpan); printpan=ISNOT;
              }
            break;

          //хоткеи выбора AP или ColdSTART/END (через Control):
          case KeyCodF10: case KeyCodF11: case KeyCodF12:
            if (Control==OFF)
              switch(kbcod)//выбор AP
                {
                case KeyCodF11://выбор AP_First
                  curnumpan=ISNOT;//панель для курсора не существует
                  ChoiceAP(ADDR_APFirst);//сообщаем выбор в AP_First
                  break;

                case KeyCodF12://выбор AP_Render
                  curnumpan=ISNOT; ChoiceAP(ADDR_APRend); break;

                case KeyCodF10://запрос на выход и выход из VRS
                  if (exitflag>0) EndProgram(0);//выход по второму F10
                  Exitpan();//вывод exitpan
                  exitflag=1;
                  break;
                }
            else//Control==ON
              switch(kbcod)//ColdSTART/END фикс. или всех AP
                {
                case KeyCodF11://AP_First
                  AddShortMSG(ADDR_VRS,ADDR_APFirst,MSG_AP_END);
                  if (AP[ADDR_APFirst].state!=AP_WORK)
                    AddShortMSG(ADDR_VRS,ADDR_APFirst,MSG_AP_CSTART);
                  break;
                case KeyCodF12://рестарт всех обязятельных AP
                  Sound(SNDLILASTART);
                  AddShortMSG(ADDR_VRS,ADDR_APFirst,MSG_AP_END);
                  AddShortMSG(ADDR_VRS,ADDR_APFirst,MSG_AP_CSTART);
                  AddShortMSG(ADDR_VRS,ADDR_APRend,MSG_AP_END);
                  AddShortMSG(ADDR_VRS,ADDR_APRend,MSG_AP_CSTART);
                  AddShortMSG(ADDR_VRS,ADDR_APOVGAg,MSG_AP_END);
                  AddShortMSG(ADDR_VRS,ADDR_APOVGAg,MSG_AP_CSTART);
                  break;
                }
            break;
          }
      break;
    }

  nextmsg://текущее сообщение:
  if (msg[msgrd].dstaddr==ADDR_VRS)//к VRS
    {
    if (msg[msgrd].srcaddr==ADDR_VRS)//от VRS
      {
      //стираем мессаг, если он с _прошлого_ витка
      if (msg[msgrd].count>=1) msg[msgrd].cmd=0;
      }
    else//не от VRS: от AP или от ADDR_NOBODY==KeyboardBufferScan()
      msg[msgrd].cmd=0;
    }

  //переход к следующему сообщению
  if (++msgrd>=MSGBUF) msgrd=0;
  }
//после скана ввода постоянно определяем stepvalue
stepvalue=(1+7*CapsLock)*(8+56*Shift)/(1+7*Alter);

//если мышь есть, определяем все ее параметры
if (mousepresent==YES) MouseStatus();

//перемещение юзера клавой или мышью
userdx=userdy=userdz=userda=0;
if (  (curnumap==ADDR_VRS) ||//если ввод с клавиатуры у VRS, или
      //если мышь так или иначе подключена к юзеру
      ( ((pointermode==0)&&(pnclmousemode==0)) ||
        ((pointermode==1)&&(curmousemode==0)) )  )
  {
  if (curnumap==ADDR_VRS)//если ввод с клавиатуры у VRS
    {
    //смещение юзера от клавиатуры
    userdx=KBDSHIFTMULX*inputdx;
    userdy=KBDSHIFTMULY*inputdy;
    userdz=KBDSHIFTMULZ*inputdz;
    userda=-KBDSHIFTMULA*inputda;//угол растет против часов
    }

  //мышь перемещает юзера если она:
  if ( ((pointermode==0)&&(pnclmousemode==0)) //не у карандаша или
     ||((pointermode==1)&&(curmousemode==0)) )//не у курсора
    {
    inputdx=inputdy=inputdz=inputda=0;
    UserMousePars();
    //дополнительное смещение юзера от мыши
    userdx+=MOUSHIFTMULX*inputdx*stepvalue;
    userdy+=MOUSHIFTMULY*inputdy*stepvalue;
    userdz+=MOUSHIFTMULZ*inputdz*stepvalue;
    userda-=(inputda*stepvalue)/MOUSHIFTDIVA;
    }
  }
//перемещение юзера в пространстве из-за ввода или клиппинга
UserSpaceShift();

if ((pointermode==0)&&(pnclmousemode==1))//если мышь перемещает карандаш
  PencilMousePars();
else//или если мышь перемещает курсор по выбранной панели
if ((curnumpan>ISNOT)&&(pointermode==1)&&(curmousemode==1))
  CursorMousePars();

//_всегда_ ограничиваем:
//перемещение пера по экрану
fpnclx=max(fpnclx,0); fpnclx=min(fpnclx,FIX1);
fpnclz=max(fpnclz,0); fpnclz=min(fpnclz,FIX1);
//удаление пера от экрана
fpncly=max(fpncly,FPNCLMINY); fpncly=min(fpncly,FPNCLMAXY);
//и перемещение курсора по выбранной панели
if (curnumpan>ISNOT)
  {
  curpanx=max(curpanx,curminx); curpanx=min(curpanx,curmaxx);
  curpanz=max(curpanz,curminz); curpanz=min(curpanz,curmaxz);
  }

if (pointermode==0)//если указка в моде карандаша
  PencilSpaceShift();//ставим его в нужное место пространства

//то же самое для курсора: но панели curnumpan может и не быть !
if ((pointermode==1)&&(curnumpan>ISNOT)) CursorSpaceShift();

PrintSystemMessages();//вывод системных сообщений
}

#define SUMTIME  (100*VRS_TIME_STEP)//время усреднения частоты кадров
#define PRINTTIME (10*VRS_TIME_STEP)//промежуток печати частоты кадров
void PrintSystemMessages(void)
//проверка наличия и вывод системных сообщений при помощи
//панелей и прямо в видео-RAM
{
int fg,x,y,z,a;
int thour,tmin,tsec,thsec,tyear,tmonth,tday,tdow;//порт dos-времени
static char str[12];
static int sumtime=0,printtime=0;
static long sumcadr=0;

//печать частоты кадров: это количество витков VRS в секунду
if (speedprint==YES)
  {
  ++sumcadr;
  if ((sumtime+=cadrtime)>=SUMTIME)
    {
    //частота*10 в строку str[0...6]:
    sprintf(str,"%7li",(10000*sumcadr)/sumtime);
    //вставляем точку перед дробной частью:
    str[7]=str[6]; str[6]='.';
    sumcadr=sumtime=0;
    }
  if ((printtime+=cadrtime)>=PRINTTIME)
    {
    //печать текста прямо в видео-RAM на боку !
    PaintText(str,8,VGAMEM,0,0,PAINTFGBG,255,0);
    printtime=0;
    }
  }

if (exitpan>ISNOT)//отслеживание позиции панели выхода
  {
  //защита от сброса кем-то флага видимости панели
  panel[exitpan].flag=YES;
  //сцепление с панелью: панель к юзеру, битмап в пиксель!
  if (locksyspan2user==ON) Bestpoz(PAN2USER,MODEBMP,exitpan);
  }

if (printpan>ISNOT)//печать координат наблюдателя и нек др.
  {
  x=fixtoi(PEN.ear.x0);
  y=fixtoi(PEN.ear.y0);
  z=fixtoi(PEN.ear.z0);
  a=wang2grad(PEN.ear.angx);
  fg=gray[15];

  if (printsize==0)//малая панель с толщиной рамки (4-1)
    {
    panfill(printpan,3,blu[6],0,0,0);//фон шире текста на 1 пиксель!
    pantext(printpan,4,fg,0,0,"%3i° %+6ix %+6iy %+6iz",a,x,y,z);
    }
  else//расширенная печать, рамка толщиной 6, 28 литер в строке
    {
    GetDosTime(&thour,&tmin,&tsec,&thsec,&tyear,&tmonth,&tday,&tdow);
    panfill(printpan,6,blu[6],177,0,8);//фон - полупрозрачная сетка

    pantext(printpan,6,fg,0, 0,"%3i° %+6ix %+6iy %+6iz",a,x,y,z);

    pantext(printpan,6,fg,0, 1,"%3i пан %-3i",MAXPAN,MAXPAN-freepans);
    pantext(printpan,6,fg,0, 2,"%3i мем %-3i",MAXMEM,MAXMEM-freemems);
    pantext(printpan,6,fg,12,1,"%6li байт",ramfullfree);
    pantext(printpan,6,fg,12,2,"%6li непр",ramfree);
    pantext(printpan,6,fg,24,1,"ввод");
    pantext(printpan,6,fg,24,2,"пп%2i",curnumap);

    pantext(printpan,6,fg,0, 3,"%02i.%02i.%4i",tday,tmonth,tyear);
    pantext(printpan,6,fg,20,3,"%2i:%02i:%02i",thour,tmin,tsec);
    }

  if (locksyspan2user==ON)
    {
    Bestpoz(PAN2USER,MODEBMP,printpan);//панель в центр и немного ниже:
    if (printsize==0) panel[printpan].ear.z0-=5*panel[printpan].flenz;
    else              panel[printpan].ear.z0-=(3*panel[printpan].flenz)/2;
    }

  panel[printpan].flag=YES;
  }
}

void Exitpan(void)
//вывод exitpan
{
int fg,glen;
Sound(SNDLILAEXIT);
glen=8;
fg=gray[15];
exitpan=GetPANtxt(ADDR_VRS,glen,15,5,150);
SetPanelPar(exitpan,FULPAN,YES,10,0,0,0,0,SIDES);
panfill(exitpan,0,0,0,0,0);//стекло !
pangran(exitpan,glen,GALL,gray,4,11,1);
pantext(exitpan,glen,fg,1,1,"F1.......HELP");
pantext(exitpan,glen,fg,1,2,"F10......EXIT");
pantext(exitpan,glen,fg,1,3,"ESC....CANCEL");
locksyspan2user=ON;//будет сцепление юзера с панелью!
}

void PencilSpaceShift(void)
//перемещение карандаша в пространстве благодаря сдвигу
//карандаша или сдвигу юзера
{
int izmax,izmin;
fix fiz,flenz;
fix fx,fy,fz,fk;

//у[вос]станавливаем флаги видимости обоих панелей указки
panel[pnclpan].flag=pnclvisible;//текущая видимость карандаша
panel[curpan].flag=NO;

//при всевозможных пертурбациях (перемещение карандаша по y,
//любой сдвиг юзера, смена скоса взгляда...) острие пера-карандаша
//всегда остается в относительной точке экрана (fpnclx,fpnclz);
//это остриё сдвинуто от оригина его битмапа на pncldz (pncldx=0)!

//это fx при pencil y = PEN y
fx=PEN.poz.x0+fixmul(PEN.lenx,fpnclx);
fz=PEN.poz.z0+fixmul(PEN.lenz,fpnclz);

fy=fpncly;
if (PEN.poz.y0!=0)
  {
  fk=fixdiv(fy,PEN.poz.y0);//коэф-т уменьшения
  fx=fixmul(fx,fk);
  fz=fixmul(fz,fk);
  }
else//это бывает в самом начале Lila
  fk=fx=fy=fz=0;//невидимость за счет y=0

//PEN координаты _острия_ панели карандаша
panel[pnclpan].pen.x0=fx;
panel[pnclpan].pen.y0=fy;
panel[pnclpan].pen.z0=fz;//для оригина: -(сдвиг острия), см ниже!
panel[pnclpan].pen.angx=pnclangx;

//iz=нужная высота панели карандаша _на экране_ !!
izmax=penbmplenz/6;//нормировка на размер экрана !
izmin=izmax/4;// iz=izmax-(izmax-izmin)*fy/fymax !
//используем fix для плавности
fiz=itofix(izmax)-fixmuldiv(itofix(izmax-izmin),fy,FPNCLMAXY);
//действительная высота её
flenz=fixmul(fiz,fk);
//размеры и масштаб панели
panel[pnclpan].flenz=flenz;
panel[pnclpan].fmulx=panel[pnclpan].fmulz=flenz/memory[pnclbm].lenz;
panel[pnclpan].flenx=panel[pnclpan].fmulx*memory[pnclbm].lenx;

//переход от острия к _оригину_ панели карандаша
panel[pnclpan].pen.z0-=pncldz*panel[pnclpan].fmulz;
}

void PencilMousePars(void)
//параметры сдвига карандаша благодяря мыши
{
static int gradflag=OFF;//ON=регулировка угла!
int napr;//направление: 1=вперед, -1=назад
int pdx,pdy,pdz,pda;
fix fk;

pdx=pdy=pdz=pda=0;

if (mouseb1off>0)//по клику b1
  {
  if (gradflag==OFF)//если регулировка угла закончена,
       pnclmmm^=1;//переключение режима мыши
  else gradflag=OFF;//иначе конец регулировки угла !
  }

if ((mouseb0==0)&&(mouseb1!=0))//если нажата _только_ b1
  {
  if (abs(mousedx)>=10)//если сдвиг мыши по x не очень мал
    {
    gradflag=ON;// (начата) регулировка угла
    pda=-mousedx/10;//вращение карандаша по углу
    }
  mousedx=mousedy=0;//других регулировок не будет!
  }

napr= (mouseb1==0)? 1:-1;//направление определяет состояние b1 !

//перемещение карандаша
if (pnclmmm==0)//режим 0
  {
  pdx=mousedx;//движение влево-вправо
  pdy=mousedy;//движение вперед-назад
  //по b0 добавляем 'авто-движение в направлении'
  if (mouseb0!=0) pdy+=napr*cadrtime;
  }
else//режим 1
  {
  pdx=mousedx;//движение влево-вправо
  pdz=mousedy;//движение вверх-вниз
  if (mouseb0!=0) pdy+=napr*cadrtime;
  }

if (fpncly>=FMINYSTEPSW)//карандаш далеко:
  fk=FIX1;//(коэф-т шага по y) fk=1 см/1 сдвиг мыши
else//карандаш близко, пропорционально уменьшаем fk
  fk=fixmuldiv(FIX1,fpncly,FMINYSTEPSW);
//защита от слишком малого fk
if (fk<FMINSTEPY) fk=FMINSTEPY;
fpncly+=fk*pdy;

if (penbmplenx>0) fpnclx+=iifixdiv(pdx,penbmplenx);
if (penbmplenz>0) fpnclz+=iifixdiv(pdz,penbmplenz);
pnclangx+=grad2ang(pda);
}

