/*
    СИ-код фильтра системы (с ADDR_VRS),
    др. системных и некоторых AP-функций.
*/

#include "vrsport.h"
#include    "vrs58.h"
#include "kbcod.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

//здесь сидят также следующие глобальные функции
int FindCelpan(void);
void SetPnclpan2cpan(int cpan);
void SetPnclpan2user(void);
long R2(d3poz p1,d3poz p2);
d3poz GetPanelCentr(int pan,d3poz origin);
void SetPanelCentr(int pan,d3poz centrappoz);
void SetPanelPoz(int pan,d3poz appoz);
void DeChoiceAP(void);
void ClipSwitch(void);
void ClipComput(void);

//внутренние функции модуля, не включённые в h-файл
void FlyOS(void);
void CopyPAN2OS(int srcpan);
d3poz SHift(d3poz delta,long maxs);
fix fm(long d,long s);
void PrintSystemMessages(void);
void UserMousePars(void);
void UserSpaceShift(void);
void PencilSpaceShift(void);
void ChoiceAP(int apnum);
void Exitpan(void);

#define SPEED     100//скорость полета ножа по оси x,y,z, м/сек
#define DIST2   20000//(дистанция завершения полета, см)^2
//тут диапазон от 0 до 1000:
#define OSPEEDcX  125//постоянная часть скорости осколков по X
#define OSPEEDcZ  250//постоянная --//-- по Z
#define OSPEEDrX  250//случайная  --//-- по X
#define OSPEEDrY  250//случайная  --//-- по Y
#define OSPEEDrZ  500//случайная  --//-- по Z
#define OSPEEDrA 1000//случайная  --//-- по углу
#define OSgZ      100//константа ~ускорения силы тяжести по Z

//число осколков 50% и 100% по гориз, вертик и полное:
#define OSNUMX2   8
#define OSNUMZ2   8
#define OSNUMX (2*OSNUMX2)
#define OSNUMZ (2*OSNUMZ2)
#define OSNUM  (OSNUMX*OSNUMZ)// 16*16 = 256

int far ospan[OSNUM];//панели осколков
byte far osflag[OSNUM];//флаг состояния осколков:
  //0=осколок в полете, 1=удар об землю, 2=осколок на земле;
d3vel far osvel[OSNUM];//текущие условные скорости осколков

int fireflag;//состояние оружия (ножа):
  //0=нож возле юзера, 1=хоткей "огонь", 2=полет ножа, 3=нож достиг цели!
int celpan=ISNOT;//панель-цель
angle celangx;//ap-угол панели-цели
int celflag;//0=нет взрыва или он кончился, 1/2=идет взрыв цели;

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

#define USERWIDTH2  16//полуширина (и полувысота) юзера для клиппинга,
//в пикселях PEN-bmp (для ширины экрана 320, пропорц-но penbmplenx);

#define CLIPMINY  (100*FIX1)//расстояние клиппинга (min дистанция до
  //непрозрачного объекта спереди от юзера)!
//процент конвертации невозможного сдвига юзера вперед:
#define CLIPSHIFT  10//в боковой сдвиг
#define CLIPBACK   20//в отскакивание назад

int speedprint=NO;//флаг печати частоты кадров;
int printsize,printpan=ISNOT;//номер размера и номер панели печати координат
int exitflag=0,exitpan=ISNOT;//флаг и номер панели для выхода из Lila
int usermmm;//user moves mouse mode
//накопители смещений юзера по вводу клавиатуры и мыши
fix inputda,inputdx,inputdy,inputdz;
//смещения юзера из-за ввода клавиатуры и мыши за кадр
fix userdx,userdy,userdz,userda;
fix dxdt,dydt,dzdt,dadt;//накопленные ~скорости смещений юзера
fix old_dz;//старый сдвиг по высоте из-за dzdt;
int lctrlflag=KEYOFF;//индикатор нажатого состояния LeftCtrl
int rctrlflag=KEYOFF;//--//-- RightCtrl
int laltflag=KEYOFF,raltflag=KEYOFF;//L/R Alt
int lshftflag=KEYOFF,rshftflag=KEYOFF;//L/R Shift

//расстояния от юзера до (ближайшего) объекта
//впереди: fdy0=слева, 1=в центре (min по вертикали!), 2=справа!
fix fdy0=MAXLONG,fdy1=MAXLONG,fdy2=MAXLONG;


void SetPanelCentr(int pan,d3poz centrappoz)
//[пере]установка панели: центр панели в позицию centr в системе AP
{
d3poz shift;
//это отрицательный сдвиг оригина от центра панели !
shift=fixXYZang2D3poz(-panel[pan].flenx/2,0,-panel[pan].flenz/2,0);
panel[pan].ap=ShiftD3poz(shift,centrappoz);
panel[pan].earvalid=NO;
}

void SetPanelPoz(int pan,d3poz appoz)
//[пере]установка оригина панели в позицию poz в системе AP
{
panel[pan].ap=appoz;
panel[pan].earvalid=NO;
}

void SetPnclpan2user(void)
//ставим панель ножа перед юзером
{
d3poz vrspoz,delta;
//ставим панель ножа на delta перед юзером
delta=XYZgrad2D3poz(0,200,-50,-70);
//вибрации панели по y-расстоянию
delta.y0+=sysfsinus;//sysfsinus=-1...1 fix
//vrs неподвижна отн-но земли:
vrspoz=ShiftD3poz(delta,PEN.ear);
SetPanelPoz(pnclpan,vrspoz);
}

void SetPnclpan2cpan(int cpan)
//оригин панели ножа в центр панели cpan
{
angle penang,earang;
//оригин VRS-pnclpan в центр панели cpan в системе земли
SetPanelPoz(pnclpan,GetPanelCentr(cpan,panel[cpan].ear));
//перпендикулярно видимой стороне панели-цели
penang=panel[cpan].pen.angx;//угол в системе PEN
earang=panel[cpan].ear.angx;//угол в системе земли
if ((ANG90G<penang)&&(penang<ANG270G))//виден тыл
  panel[pnclpan].ap.angx=earang+ANG90G;
else//виден фронт
  panel[pnclpan].ap.angx=earang-ANG90G;
}

void ClipSwitch(void)
//переключение флага клиппинга
{
clip=(++clip)&1;
//для безошибочного клиппинга все дальности сначала максимальны
fdy0=fdy1=fdy2=MAXLONG;
}

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

void UserSpaceShift(void)
//перемещает юзера с учетом клиппинга
{
fix dymin,shift,dshift,dx0;
d3poz delta;
int uklon;

//delta = смещение USER-системы (наблюдателя) в USER-системе,
//преобразуется из USER-системы в систему земли и присваивается
//обратно USER-системе в два приема - для верного клиппинга !

//смещение юзера для 'плаванья по цветам' земли сначала =0
DYuser=0;
//сначала линейное смещение наблюдателя
if ((userdx!=0)||(userdy!=0)||(userdz!=0)
    ||(dxdt!=0)||(dydt!=0)
    ||(clip==YES))
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
      Sound(SNDCLIPTOLHOK);
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
      Sound(SNDCLIPUDAR);
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

  //проверяем окончательно есть ли линейный сдвиг юзера
  if ((delta.x0!=0)||(delta.y0!=0)||(delta.z0!=0))
    {
    //сдвиг USER-системы на вектор delta
    PEN.ear=ShiftD3poz(delta,PEN.ear);
    //координаты USER-системы изменились!
    PEN.usershift=YES;
    //смещение юзера для 'плаванья по цветам' земли:
    DYuser=delta.y0;
    }
  }

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
  act_dz=dzdt*(sysfsinus/50);
  PEN.ear.z0+=act_dz-old_dz;
  PEN.usershift=YES;
  old_dz=act_dz;
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
          if (APforinput!=ADDR_VRS)//ввод у AP: отмена выбора AP
            {
            //сначала добавляем мессаг для AP
            AddShortMSG(ADDR_VRS,APforinput,MSG_AP_INPUT_OFF);
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

      if (APforinput!=ADDR_VRS)//ввод подключен не к VRS, а к AP !
        {
        msg[msgrd].dstaddr=APforinput; goto nextmsg;//переадресуем код в AP
        }
      //else ввод у VRS

      switch(kbcod)//VRS переадресует эти хоткеи в AP
        {
        case KeyCod0:
        case KeyCod1: case KeyCod2: case KeyCod3:
        case KeyCod4: case KeyCod5: case KeyCod6:
        case KeyCod7: case KeyCod8: case KeyCod9:
          //частота колебаний ГСК переключается здесь !!!
          if ((hit==KEYOFF)&&(Shift==ON)) Fsys=kbcod-KeyCod1+1;
        case KeyCodDiv:
        case KeyCodPoint:
        case KeyCodComma: msg[msgrd].dstaddr=ADDR_APRend; goto nextmsg;

        case KeyCodF1: case KeyCodF4: case KeyCodF7:
        case KeyCodF2: case KeyCodF5: case KeyCodF8:
        case KeyCodF3: case KeyCodF6: case KeyCodF9:
        case KeyCodPlus:
        case KeyCodMinus:
        case KeyCodPgUp:
        case KeyCodPgDn:  msg[msgrd].dstaddr=ADDR_APFirst; goto nextmsg;
        }

      if (hit==KEYON)
        switch(kbcod)
          {
          default: break;

          case KeyCodSpace: if (fireflag==0) fireflag=1; break;//"огонь" !

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

          case KeyCodScrlLck://сцепка системных панелей с юзером
            locksyspan2user^=1; break;

          case KeyCodBS://вид ножа
            if (++pnclvisualmode>2) pnclvisualmode=0; break;

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
                  ChoiceAP(ADDR_APFirst);//сообщаем выбор в AP_First
                  break;

                case KeyCodF12://выбор AP_Render
                  ChoiceAP(ADDR_APRend); break;

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

//перемещение юзера клавой и мышью:

userdx=userdy=userdz=userda=0;
if (APforinput==ADDR_VRS)//если ввод с клавиатуры у VRS
  {
  //смещение юзера от клавиатуры
  userdx=KBDSHIFTMULX*inputdx;
  userdy=KBDSHIFTMULY*inputdy;
  userdz=KBDSHIFTMULZ*inputdz;
  userda=-KBDSHIFTMULA*inputda;//угол растет против часов
  }

//мышь перемещает юзера
inputdx=inputdy=inputdz=inputda=0;
UserMousePars();
//дополнительное смещение юзера от мыши
userdx+=MOUSHIFTMULX*inputdx*stepvalue;
userdy+=MOUSHIFTMULY*inputdy*stepvalue;
userdz+=MOUSHIFTMULZ*inputdz*stepvalue;
userda-=(inputda*stepvalue)/MOUSHIFTDIVA;

//перемещение юзера в пространстве из-за ввода или клиппинга
UserSpaceShift();

//ставим нож в нужное место пространства
PencilSpaceShift();

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
    pantext(printpan,6,fg,24,2,"пп%2i",APforinput);

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
SetPanelPar(exitpan,FULPAN,YES,20,0,0,0,0,SIDES);
panfill(exitpan,0,0,0,0,0);//стекло !
pangran(exitpan,glen,GALL,gray,4,11,1);
pantext(exitpan,glen,fg,1,1,"EXIT......F10");
pantext(exitpan,glen,fg,1,2,"cancel....ESC");
pantext(exitpan,glen,fg,1,3,"help.......F1");
locksyspan2user=ON;//будет сцепление юзера с панелью!
}

void DeChoiceAP(void)
//отмена выбора AP, но без мессага !
{
if (APforinput==ADDR_VRS) return;
//else отмена выбора AP
Sound(SNDVRSIN);
APforinput=ADDR_VRS;//нет выбранной AP
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
//теперь сообщаем о выборе AP для ввода
AddShortMSG(ADDR_VRS,apnum,MSG_AP_INPUT_ON);

APforinput=apnum;//ввод переводим на AP
}

long R2(d3poz p1,d3poz p2)
//возвращает квадрат расстояния от точки p1 до p2, в (см*см) !
{
long dx,dy,dz;
dx=fixtoi(p1.x0-p2.x0);
dy=fixtoi(p1.y0-p2.y0);
dz=fixtoi(p1.z0-p2.z0);
//для больших dx...dz (напр 32000) будет переполнение !
return labs(dx*dx+dy*dy+dz*dz);
}

d3poz GetPanelCentr(int pan,d3poz origin)
//возвращает позицию центра панели в той же системе отсчета,
//в которой выражен ее origin: panel[pan].ap, .ear или .pen
{
d3poz shift;
//это положительный сдвиг центра от оригина панели !
shift=fixXYZang2D3poz(panel[pan].flenx/2,0,panel[pan].flenz/2,0);
return ShiftD3poz(shift,origin);
}

int FindCelpan(void)
//возвращает панель-цель, простирающуюся через середину экрана,
//но только с овнером AP1 !
{
int n,pan,xmiddl;
long minr2,r2;
d3poz centr;
//отбор панелей на основе их xbe-координат, однако при
//неполной геомоде панелей будут неточности...
pan=ISNOT;//нет панели-цели
minr2=MAXLONG;
xmiddl=penbmplenx/2;
for (n=0; n<=maxpannum; ++n)
  {
  //пропускаем отсутствующие, полностью невидимые и не-AP1 панели
  if ((panel[n].flag!=YES)
      ||(panel[n].render!=YES)
      ||(panel[n].owner!=ADDR_APFirst)) continue;
  if (n==pnclpan) continue;//пропускаем панель ножа
  //панель на экране должна проходить через его середину
  if ((panel[n].xbelft>xmiddl)||(panel[n].xbergt<xmiddl)) continue;
  //ищем ближайшую панель по радиусу от ее центра к центру PEN
  centr=GetPanelCentr(n,panel[n].pen);//центр панели в системе PEN
  r2=R2(centr,d3null);//квадрат растояния центра панели от центра PEN
  if (r2<minr2) { minr2=r2; pan=n; }
  }
return pan;
}

d3poz SHift(d3poz delta,long maxs)
//по вектору полного расстояния до цели delta и
//скаляру текущего максимального сдвига maxs
//считаем и возвращаем вектор текущего сдвига
{
long dx,dy,dz,adx,ady,adz,maxad,sx,sy,sz;
d3poz shift;
shift=d3null;
//компоненты delta в см
dx=fixtoi(delta.x0);
dy=fixtoi(delta.y0);
dz=fixtoi(delta.z0);
//модули компонент
adx=labs(dx);
ady=labs(dy);
adz=labs(dz);
//макс-й модуль
maxad=max(adx,max(ady,adz));
if (maxad<=0) goto end;
//макссдвиг разлагаем на компоненты
sx=(maxs*adx)/maxad;
sy=(maxs*ady)/maxad;
sz=(maxs*adz)/maxad;
//находим компоненты сдвига ножа
shift.x0=fm(dx,sx);
shift.y0=fm(dy,sy);
shift.z0=fm(dz,sz);
end:
return shift;
}

fix fm(long d,long s)
//возвращает itofix( (не более s) и (не более delta) ),
//считая знаки положительными, для отриц-х аналогично !
{
int sign,res;
long ad,as;
sign= (d<0)? -1:1;
as=labs(s);
ad=labs(d);
res=(int)min(ad,as);
return itofix(sign*res);
}

void CopyPAN2OS(int srcpan)
//копируем панель srcpan в несколько осколков
{
int i,j,k,pan,blx,blz;
fix fx,fz,flx,flz;
d3poz delta,poz,srcpoz;

printwarn=OFF;//нехватка панелей не приведет к мессагу и стопу
for (i=0; i<OSNUM; ++i)//выделяем все осколки
  {
  pan=GetPAN(ADDR_OS);
  ospan[i]=pan;
  osflag[i]=0;//осколок в полете
  if (pan<=ISNOT) continue;//для осколка нет панели!
  //копируем источник и лочим осколки
  CopyPAN(pan,srcpan);
  //восстанавливаем овнера
  panel[pan].owner=ADDR_OS;
  LockPAN(pan);
  }
printwarn=ON;
//меняем размеры, положение и смещение текстуры осколков
flx=panel[srcpan].flenx;
flz=panel[srcpan].flenz;
fx=flx/OSNUMX;
fz=flz/OSNUMZ;
delta=d3null;
srcpoz=panel[srcpan].ap;
blx=panel[srcpan].xb1-panel[srcpan].xb0+1;
blz=panel[srcpan].zb1-panel[srcpan].zb0+1;
for (i=0; i<OSNUMX; ++i)
  {
  for (j=0; j<OSNUMZ; ++j)
    {
    k=OSNUMX*j+i;//порядковый номер осколка
    pan=ospan[k];
    if (pan<=ISNOT) continue;//для осколка нет панели!
    panel[pan].flenx=fx;//размеры осколков
    panel[pan].flenz=fz;
    delta.x0=fx*i;
    delta.z0=fz*j;
    poz=ShiftD3poz(delta,srcpoz);
    SetPanelPoz(pan,poz);
    panel[pan].xb0=panel[srcpan].xb0+(blx*i)/OSNUMX;
    panel[pan].zb0=panel[srcpan].zb0+(blz*j)/OSNUMZ;
    }
  }
}

void PencilSpaceShift(void)
//ставим нож нужного вида в нужное место пространства
{
d3poz centr,delta,shift,newpoz;
long s,r2;
int i;

switch(pnclvisualmode)//вид ножа
  {
  default:
  case 0://нож невидим
    panel[pnclpan].flag=NO;
    fireflag=0; celpan=ISNOT;//нет броска, нет цели
    return;

  case 2://нож со стеклом: для указания на цель
    panel[pnclpan].flag=YES;
    SetPanelPar(pnclpan,FULPAN,YES,15,0,0,0,0,SIDES);
    fireflag=0;//нет броска
    celpan=FindCelpan();//ищем панель-цель в середине экрана
    if (celpan==ISNOT)//нет цели
         SetPnclpan2user();//нож возле юзера
    else SetPnclpan2cpan(celpan);//нож в центр панели-цели
    return;

  case 1://нож без стекла: для кидания и взрыва цели
    panel[pnclpan].flag=YES;
    SetPanelPar(pnclpan,FULPAN, NO, 0,0,0,0,0,SIDES);
    break;
  }

//нож для кидания и взрыва цели

switch (fireflag)//фазы огня
  {
  default:
  case 0://нет огня
    SetPnclpan2user();//нож возле юзера
    break;

  case 1://хоткей "огонь"
    SetPnclpan2user();//нож сначала возле юзера
    celpan=FindCelpan();//ищем панель-цель в середине экрана
    if (celpan==ISNOT) fireflag=0;//цели нет
    else//цель есть, огонь!
      {
      Sound(SNDFIRE);
      fireflag=2;
      }
    break;

  case 2://огонь: полет ножа к цели
    //центр панели-цели в системе земли
    centr=GetPanelCentr(celpan,panel[celpan].ear);
    //квадрат растояния центра панели-цели от оригина ножа
    r2=R2(centr,panel[pnclpan].ear);
    if (r2<DIST2) fireflag=3;//критерий достижения цели
    else//полет к цели
      {
      //разность позиций (центр celpan)-(оригин pnclpan)
      delta=DeltaD3poz(centr,panel[pnclpan].ear);
      //макс. путь ножа в сантиметрах за время кадра в миллисек-х
      s=(SPEED*(long)cadrtime)/10;
      //находим нужный сдвиг ножа
      shift=SHift(delta,s);
      //сдвиг позиции pnclpan на shift
      newpoz=ShiftD3poz(shift,panel[pnclpan].ear);
      SetPanelPoz(pnclpan,newpoz);
      }
    break;

  case 3://начало взрыва цели
    Sound(SNDVZRYV);
    //удаляем все старые осколки
    for (i=0; i<OSNUM; ++i) UnLockPAN(ospan[i]);
    //копируем панель-цель в панели осколков
    CopyPAN2OS(celpan);
    //запоминаем угол панели-цели
    celangx=panel[celpan].ap.angx;
    panel[celpan].flag=NO;//делаем панель цели невидимой
//    UnLockPAN(celpan);//если ее совсем убирать - глюки с хелпом!
    celpan=ISNOT;
    celflag=1;//начало взрыва
    fireflag=0;//нож готов для огня!
    break;
  }

if (celflag>0) FlyOS();//разлет осколков при взрыве
}

void FlyOS(void)
//разлет осколков
{
int pan,i,j,k,ii,jj;
long time,seed;
d3poz delta,poz;
angle panangx;
int allonear;

if (celflag==1)//начало взрыва
  {
  seed=vrs_time;//зерно случайности на этот взрыв
  //запоминаем начальные векторы скоростей осколков
  for (i=0; i<OSNUMX; ++i)
    {
    for (j=0; j<OSNUMZ; ++j)
      {
      k=OSNUMX*j+i;//порядковый номер осколка
      pan=ospan[k];
      if (pan<=ISNOT) continue;//для осколка нет панели!

      //постоянные компоненты смещения по X-Z
      ii=i-OSNUMX2; if (i>=OSNUMX2) ++ii;
      jj=j-OSNUMZ2; if (j>=OSNUMZ2) ++jj;
      //диапазон -32000...+32000
      delta.x0=(32*OSPEEDcX*(long)ii)/OSNUMX2;
      delta.z0=(32*OSPEEDcZ*(long)jj)/OSNUMZ2;

      //случайные компоненты смещения по Y и углу
      seed=RANDUMUL*seed+RANDUADD;
      delta.y0=(OSPEEDrY*(seed>>16))/1000;
      seed=RANDUMUL*seed+RANDUADD;
      delta.angx=(angle)((OSPEEDrA*labs(seed>>16))/500);

      //добавляем случайные компоненты смещения по X-Z
      seed=RANDUMUL*seed+RANDUADD;
      delta.x0+=(OSPEEDrX*(seed>>16))/1000;
      seed=RANDUMUL*seed+RANDUADD;
      delta.z0+=(OSPEEDrZ*(seed>>16))/1000;

      //начальный вектор скорости осколка в условных единицах!
      osvel[k]=delta;
      }
    }
  celflag=2;
  }

//полет по параболам до падения _всех_ осколков на землю!

allonear=YES;//предполагаем, что все осколки на земле
time=cadrtime;
for (i=0; i<OSNUMX; ++i)
  {
  for (j=0; j<OSNUMZ; ++j)
    {
    k=OSNUMX*j+i;//порядковый номер осколка
    pan=ospan[k];
    if (pan<=ISNOT) continue;//для осколка нет панели!
    if (osflag[k]>=2) continue;//осколок лежит на земле!
    //не на земле
    allonear=NO;
    if (spaceflag==0)//земля: влияние силы тяжести на скорость
      osvel[k].z0-=(OSgZ*time)/10;
    //else космос
    delta=osvel[k];//скорость в условных единицах !!!
    //сдвиг осколка за время кадра
    delta.x0*=time*10;
    delta.y0*=time*10;
    delta.z0*=time*10;
    delta.angx=0;//сначала без вращения

    if (osflag[k]==0)//осколок еще не упал
      {
      poz=panel[pan].ap;//старое положение осколка
      panangx=poz.angx;//старый угол
      //меняем угол осколка на первоначальный угол цели для
      //сдвига его в плоскости цели
      poz.angx=celangx;
      poz=ShiftD3poz(delta,poz);//новое положение
      if ((spaceflag==0)&&(poz.z0<0))//удар об землю!
        {
        poz.z0=0;
        osflag[k]=1;//для остановки осколка на следующем витке
        }
      //текущий поворот
      delta.angx=(angle)((osvel[k].angx*time)/1000);
      //новый угол осколка=старый угол плюс текущий поворот
      poz.angx=panangx+delta.angx;
      SetPanelPoz(pan,poz);
      }
    else//осколок будет неподвижно лежать на земле
      {
      //делаем осколок системным и копируем позицию земли в ap
      panel[pan].owner=ADDR_VRS;
      panel[pan].ap=panel[pan].ear;
      osflag[k]=2;
      }
    }
  }

//если все осколки на земле - конец взрыва
if (allonear==YES) celflag=0;
}

