/*
   Lila ASM386-СИ код рендеров (панели, дали и пр.) !

   Надо:
   -при наличии exeptions, или для рендера без cli, или для увеличения
    надежности/отлаживаемости кода следует произвести изменения:
    -не использовать ss/sp вообще;
    -хранить в ds,es,fs,gs только сегменты реальных данных;
    -надо писать ds:[(e)bp], иначе по умолчанию берется ss, но для
     смещений в других регистрах ds можно и нужно опускать;

   Важно:
   -без компиляторной оптимизации код короче и быстрее!
   -все 7 предупреждений компилятора игнорировать!
*/

//PORT: #pragma inline removed - all code is plain C now
#include <dos.h>
#include <limits.h>
#include "vrsport.h"
#include "vrs58.h"

int j0;//это здесь для ручного ASM ...
byte far *dram;
byte near *pram;
void RenderD(void)
//рендер дали: далёкие горы, облака и проч.
//горизонтальный масштаб учитывает текущее val_h !
{
int jsmin,i,k,j,hlen,vlen,js,je,jscol,dlen,dh,dshift,col;
static int dhorshift;
byte far *daliram;
angle angx;
fix fm;

if ((dalinali==NO)||(dalibitm<=ISNOT)) return;

daliram=memory[dalibitm].rampntr;
vlen=penbmplenz;
hlen=penbmplenx;
//parthlen=partpenbmplenx;

if (spaceflag==OFF)//если не космос
  //имитация заслонения более близкой дали совсем близкой землёй:
  {
  //столбцы дали будут выводиться с js-уровня по крайней мере
  jsmin=jearuser+20;//на 20 строк выше уровня близкой земли !
  //однако когда близкая земля выше горизонта js min = jearuser
  if (jsmin>jear) jsmin=jear;
  if (jsmin<jearuser) jsmin=jearuser;
  }
else jsmin=0;//нет заслонения

//поворот дали
if (actpart==0)
  {
  daliang+=dalidadt*cadrtime;
  angx=PEN.ear.angx+(angle)(daliang>>4);
  //horshift для дали - см AP_Render
  dhorshift=
    fixtoi(fixmul(itofix(val_h),
      fixmul(iifixdiv(31416,10000),
        iifixdiv(angx/2,ANG90G))));
  dhorshift=(horshift360-dhorshift)%horshift360;
  }

//рендер участка дали текущего полукадра

//горизонтальный масштаб дали
fm=fixmul(iifixdiv(200,val_h),iifixdiv(STRLEN,MAXHORSHIFT));
for (i=0; i<partpenbmplenx; ++i)//цикл по горизонтали
  {
  //когда dhorshift=0, начало дали dalistr[0] должно быть
  //в середине экрана, т.е. на столбце с i=hlen/2 !
  if (actpart==0)//левый полукадр (или весь кадр)
    k=fixtoi(  fixmul( fm, itofix(i+dhorshift-hlen/2) )  );
  else//actpart=1, правый полукадр
    k=fixtoi(  fixmul( fm, itofix(partpenbmplenx+i+dhorshift-hlen/2) )  );
  if (k<0)       k+=STRLEN;
  if (k>=STRLEN) k-=STRLEN;

  dh=dalistrh[k];
  dlen=dalistrl[k];
  dshift=dalistrs[k];

  js=jear+dalidjear+dh;
  je=js+dlen-1;
  if ((je<js)||(je<0)||(js>=vlen)) continue;
  jscol=dshift-js;//стартовое j для цвета
  //далее js может измениться !!!
  if (js<0)     js=0;
  if (je>=vlen) je=vlen-1;
  if (js<jsmin) js=jsmin;

  j0=js-je;
  dram=daliram+jscol+js;
  pram=penram+partpenbmplenx*js+i;
  if (dtranscol>=0)//в дали есть прозрачный цвет
    {
/*
    for (j=j0; j<=0; ++j)
      {
      if ( (col=*dram++) != dtranscol ) *pram=col;
      pram+=partpenbmplenx;
      }
*/
    for (j=j0; j<=0; ++j)
      {
      if ( (col=*dram++) != dtranscol ) *pram=col;
      pram+=partpenbmplenx;
      }
    }
  else//нет прозрачного цвета!
    {
/*
    for (j=j0; j<=0; ++j)//цикл по вертикали
      {
      //цвет из битмапа дали в penram
      *pram=*dram++;
      pram+=partpenbmplenx;
      }
*/
    for (j=j0; j<=0; ++j)//cycl po vertikali
      {
      //cvet iz bitmapa dali v penram
      *pram=*dram++;
      pram+=partpenbmplenx;
      }
    }
  }
}

//переменные внутреннего цикла рендера
byte near *gradcolor;
byte near *rram;
byte far *convpal;
byte far *tram;
short far *ybuf;
int colpix,zz,numz,fypan;
fix Fzbe,F1yp;
dword regesp,regebp,regesi,regedi;
word regss,reges;
void RenderPanel(int srcpan)
//рендер панели srcpan в память PEN-панели penram[]
{
byte far *txtrram;//src память с текстурой панели
short far *ybuffer;//Z-буфер
int rendtransfl,rendconvpal,xbe,xbelft,xbergt,zbebtm,zbetop,zb0;
int blenz,rendlenx,xb0,xb1,dxb10,dxb102,dxb10p,dxb10p2,xb,dxb;
int srcmem,memconv,con1,rendsw,heo,heonum,heomode;
int gradampx,grcol,grcolx,grcoly,grcol0,rendglass;
//fix-ация:
fix fmulx,fmulx1,fmulz,maxfmulz,srcpanflenx,srcpanflenz;
fix Fdn,Xb1,Xp1,Xs,C0,C1,F0yp,Yfg,Yp,Flenx,Flenx2,Flenz;
fix gradlenx,Fz,Fz0[2],Fz1[2];
static long seed;
long rfluct;

xbelft=panel[srcpan].xbelft;
xbergt=panel[srcpan].xbergt;

if (numpart==2)//двухшаговый рендер
  {
  if (actpart==0)//левая половина кадра
    { if (xbergt>=partpenbmplenx) xbergt=partpenbmplenx-1; }
  else//правая половина
    { if (xbelft< partpenbmplenx) xbelft=partpenbmplenx;   }
  //если панель не попадает на текущую половину
  if (xbergt<xbelft) return;
  }

heomode=panel[srcpan].heomode;
if (heomode==0) return;//все 4 лепестка панели невидимы

srcmem=panel[srcpan].memtex;
//преобразование моды развертки src памяти
if (memory[srcmem].rammode!=2) ConvertMEMmode(srcmem,2);

//сначала предполагаем рендер без преобразования цветов
rendconvpal=NO;
//прозрачный рендер зависит от transflag src битмапа
rendtransfl=memory[srcmem].transflag;
//блок памяти таблицы преобразования цветов src панели
memconv=panel[srcpan].memconv;
if (memconv>ISNOT)//есть конверт, рендер с преобразованием цветов
  {
  rendconvpal=YES;
  //таблицу преобразования цветов берем из memconv панели
  convpal=memory[memconv].rampntr;
  //прозрачный рендер зависит _также_ от transflag конверта
  rendtransfl|=memory[memconv].transflag;
  }
//принудительный рендер прозрачного цвета:
if (rendtransflag>0) rendtransfl=rendtransflag-1;

txtrram=memory[srcmem].rampntr;
//PEN.ymem Ram-лочить здесь не обязательно, это делает вызывающий код !
ybuffer=(short far *)memory[PEN.ymem].rampntr;
X0=panel[srcpan].pen.x0;
Y0=panel[srcpan].pen.y0;
Z0=panel[srcpan].pen.z0;
Alpha=panel[srcpan].pen.angx;
fsin=fsinus(Alpha);
fcos=fcosin(Alpha);
srcpanflenx=panel[srcpan].flenx;
srcpanflenz=panel[srcpan].flenz;

fmulx=panel[srcpan].fmulx;
//защита от сбоев:
if (fmulx<=0) fmulx=1;
fmulx1=fixdiv(FIX1,fmulx);
fmulz=panel[srcpan].fmulz;
//защита от сбоев:
if (fmulz<=0) fmulz=1;
maxfmulz=MAXLONG/val_h;//чтобы не переполнилось C1!
if (fmulz>maxfmulz) fmulz=maxfmulz;

rendlenx=panel[srcpan].rendlenx;
xb0=panel[srcpan].xb0;
xb1=panel[srcpan].xb1;
//приводим xb0, xb1 к интервалу 0...(rendlenx-1),
xb0%=rendlenx;
xb1%=rendlenx;
//обратная запись - надо для возможного скролла
panel[srcpan].xb0=xb0;
panel[srcpan].xb1=xb1;
dxb10=xb1-xb0+1;
dxb102=2*dxb10;
dxb10p=rendlenx+xb1-xb0+1;
dxb10p2=2*dxb10p;
blenz=memory[srcmem].lenz;
zb0=panel[srcpan].zb0;
//zb1 пока не используется...

grcol0=GRADCOLNOM;//градация панели=номиналу
if ((gradvarflag&1)!=0)
  grcol0+=panel[srcpan].gradcon;//добавляем постоянную градацию панели

rendglass=panel[srcpan].glass;
//стеклянная панель: добавляем общую не-прозрачность
if (rendglass==YES) grcol0+=neprozra;
else//нестеклянная
  {
  //добавляем общую не-яркость
  grcol0+=gradconst;
  //добавляем динамическую не-яркость
  grcol0+=panel[srcpan].graddin;
  }

if (gradvarflag>1)//есть волны градаций по x и/или y панели
  {
  gradlenx=panel[srcpan].gradlenx;
  gradampx=panel[srcpan].gradampx;
  if (gradlenx==0) gradlenx=1;
  }
else//нет регулировки градаций внутри панели
  {
  if (grcol0>MAXGRADCOL1) grcol0=MAXGRADCOL1;
  if (grcol0<0)           grcol0=0;
  gradcolor=&gradcol[grcol0][0];
  }

//определение номера ветви рендера
rendsw=0;
if (rendtransfl==YES) rendsw+=1;//с проверкой на прозрачный цвет
if (rendconvpal==YES) rendsw+=2;//с использованием таблицы convpal
if (rendglass==YES)   rendsw=4;//стеклянная панель!

rfluct=panel[srcpan].rfluct;
if (rfluct>0) rfluct+=randfluct;//добавка флуктуаций !

//идеальные размеры панелей минус запас !
Flenx=srcpanflenx-ZAPASX;
Flenz=srcpanflenz-ZAPASZ;
if (Flenx<1) Flenx=1;
if (Flenz<1) Flenz=1;
Flenx2=Flenx>>1;

//см формулы (68-2):
C0=itofix(zb0)-fixdiv(Z0,fmulz);
C1=val_h*fmulz;

//обходим часть PEN-bmp с src панелью, проектируя точку bmp на src
for (xbe=xbelft; xbe<=xbergt; ++xbe)
  {
  Xs=itofix(xbe+penpozx0);
  Fdn=fixmul(Xs,fsin)-val_h*fcos;
  if (Fdn!=0)//здесь очень редко возможно int0 переполнения при делении
    Xp1=fixmulsumdiv(itofix(val_h),X0,-Xs,Y0,Fdn);
//  Xp1=(val_h*X0-Xs*Y0)/(Xs*sin-val_h*cos);
  else//Fdn~=0 бывает для луча зрения, ~параллельного плоскости панели,
    Xp1=Flenx2;//тогда просто берем ~середину панели...

  //защита от сильной грязи из-за возможных больших ошибок Xp1
  if ((Xp1<0)||(Xp1>Flenx)) continue;
  //дальность ребра Xp1
  Yp=Y0+fixmul(Xp1,fsin);
  //эта проверка также необходима из-за возможных ошибок Xp1
  if (Yp<FIXYPMIN) continue;

  //регулировка градаций внутри панели
  if (gradvarflag>1)
    {
    grcolx=grcoly=0;

    if ((gradvarflag&2)!=0)//колебания градаций яркости по x панели
      {
      if (gradlenx>0)
        {
        //яркость _понижается_ с оригина вправо
        grcolx=(int)(255&(fixdiv(Xp1,gradlenx)>>8));
        grcolx= (grcolx<128)? grcolx:(255-grcolx);
        }
      else
      if (gradlenx<0)
        {
        //яркость _повышается_ с оригина вправо
        grcolx=(int)(255&(fixdiv(Xp1,-gradlenx)>>8));
        grcolx= (grcolx<128)? (255-grcolx):grcolx;
        }
      }

    if ((gradvarflag&4)!=0)//колебания градаций яркости по Y панели
      {
      if (gradleny>0)
        {
        //яркость понижается с близкого в даль
        grcoly=(int)(255&((Yp/gradleny)>>8));
        grcoly= (grcoly<128)? grcoly:(255-grcoly);
        }
      else grcoly=0;
      }

    //постоянная градация плюс переменные
    grcol=grcol0+((gradampx*grcolx)>>7)+((gradampy*grcoly)>>7);
    //определяем слой цветов данной градации
    if (grcol>MAXGRADCOL1) grcol=MAXGRADCOL1;
    if (grcol<0) grcol=0;
    gradcolor=&gradcol[grcol][0];
    }

  //рассчёт лепестковой геометрии панели
  if (heomode==FULPAN)//все лепестки есть
    {
    heonum=1; Fz1[0]=Flenz; Fz0[0]=0;
    }
  else//нет части лепестков
    {
    if (Xp1<=Flenx2)//левая половина панели
      {
      Fz=fixmuldiv(Flenz,Xp1,Flenx);
      switch(heomode&14)//правый (0-й) лепесток игнорируем
        {
        case  0: heonum=0; continue;//ничего
        case  2: heonum=1; Fz1[0]=Flenz;    Fz0[0]=Flenz-Fz; break;//top
        case  4: heonum=1; Fz1[0]=Flenz-Fz; Fz0[0]=Fz; break;//lft
        case  6: heonum=1; Fz1[0]=Flenz;    Fz0[0]=Fz; break;//top+lft
        case  8: heonum=1; Fz1[0]=Fz;       Fz0[0]=0; break;//btm
        case 10: heonum=2; Fz1[0]=Fz;       Fz0[0]=0;
                 Fz1[1]=Flenz;    Fz0[1]=Flenz-Fz; break;//btm+top
        case 12: heonum=1; Fz1[0]=Flenz-Fz; Fz0[0]=0; break;//btm+lft
        case 14: heonum=1; Fz1[0]=Flenz;    Fz0[0]=0; break;//btm+lft+top
        }
      }
    else//правая половина
      {
      Fz=fixmuldiv(Flenz,Flenx-Xp1,Flenx);
      switch(heomode&11)//левый (2-й) лепесток игнорируем
        {
        case  0: heonum=0; continue;//ничего
        case  2: heonum=1; Fz1[0]=Flenz;    Fz0[0]=Flenz-Fz; break;//top
        case  1: heonum=1; Fz1[0]=Flenz-Fz; Fz0[0]=Fz; break;//rgt
        case  3: heonum=1; Fz1[0]=Flenz;    Fz0[0]=Fz; break;//top+rgt
        case  8: heonum=1; Fz1[0]=Fz;       Fz0[0]=0; break;//btm
        case 10: heonum=2; Fz1[0]=Fz;       Fz0[0]=0;
                 Fz1[1]=Flenz;    Fz0[1]=Flenz-Fz; break;//btm+top
        case  9: heonum=1; Fz1[0]=Flenz-Fz; Fz0[0]=0; break;//btm+rgt
        case 11: heonum=1; Fz1[0]=Flenz;    Fz0[0]=0; break;//btm+rgt+top
        }
      }
    }

  //переходим к битмапу по X
  Xb1=fixmul(Xp1,fmulx1);
  //флуктуации пикселей по x
  if ((rfluct>0)&&((fluctflag&1)>0))
    {
    seed=RANDUMUL*seed+RANDUADD;
    //пиксель будет колебаться вправо от номинала
    Xb1+=(rfluct*((seed>>16)&0x1fff));//случайное от 0 до 0x1fff !
    }
  dxb=pfixtoi(Xb1);//Xb1>=0 !
  if (simsimflag==OFF)//циклическая развёртка слева направо
    {
    if (dxb10>0)//xb0<=xb1
      //развертка битмапа xb0...xb1,
      xb=xb0+dxb%dxb10;
    else//xb0>xb1
      //развертка xb0...(rendlenx-1)->потом->0...xb1,
      //что сильно облегчает динамический скроллинг битмапа !
      xb=(xb0+dxb%dxb10p)%rendlenx;
    }
  else//симметричная развертка битмапа вправо, потом влево
    {
    if (dxb10>0)//xb0<=xb1
      {
      //развертка битмапа xb0...xb1,xb1...xb0
      dxb%=dxb102;
      dxb= (dxb<dxb10)? dxb:(dxb102-1-dxb);
      xb=xb0+dxb;
      }
    else//xb0>xb1
      {
      //развертка xb0...(rendlenx-1)->потом->0...xb1, потом обратно
      dxb%=dxb10p2;
      dxb= (dxb<dxb10p)? dxb:(dxb10p2-1-dxb);
      xb=(xb0+dxb)%rendlenx;
      }
    }

  //грубое Yp для далеких точек панели
  fypan=pfixtoi(Yp);
  //для близких y близи точность Yp для рендера увеличиваем так:
  Yfg=Yp-DYFG;
  //если Yp<DYFG, то fypan=128*(Yp-DYFG)/65536, что в 128 раз растягивает
  //диапазон Yp=0...DYFG в сторону отрицательных расстояний, причем на
  //DYFG см происходит скачек fypan до 0, далее равномерно до -128*DYFG
  //сдвиг вправо на 9 бит, что на 7 бит точнее обычных 16
  if (Yfg<0) fypan=-(int)((-Yfg)>>YFGSHIFT);

  for (heo=0; heo<heonum; ++heo)
    {
    //PEN-pan Z-концы от src панели на ребре Xp1, соответствующем xs
    //тут переполнение при делении практически невероятно...
//  Zpetop=lfixmuldiv(val_h,Z0+Flenz,Yp)-penpozz0; старый вариант
//  Zpebtm=lfixmuldiv(val_h,Z0,      Yp)-penpozz0; без heomode !!
    Zpetop=lfixmuldiv(val_h,Z0+Fz1[heo],Yp)-penpozz0;
    Zpebtm=lfixmuldiv(val_h,Z0+Fz0[heo],Yp)-penpozz0;

    //защита по z на PEN-pan, иначе возможно переполнение!!!
    if ((Zpetop<0)||(Zpebtm>=penpanlenz)) continue;
    if (Zpebtm<0) Zpebtm=0;
    if (Zpetop>penpanlenz) Zpetop=penpanlenz;
    //переходим к PEN-bmp: *5/6
    zbebtm=pfixtoi(Zpebtm*penbmpfmulz1);
    zbetop=pfixtoi(Zpetop*penbmpfmulz1);
    //точное ограничение z-развертки размерами экрана PEN-bmp
    if (zbetop>=penbmplenz) zbetop=penbmplenz-1;
    numz=zbetop-zbebtm;
    if (numz<0) continue;//z-конец получился ниже начала

    //подходим к внутреннему циклу
    F0yp=C0+fixmuldiv(itofix(penpozz0),Yp,C1);
    F1yp=fixmuldiv(penbmpfmulz,Yp,C1);

    //точка zb=0 текстуры битмапа столбца xbe
    tram=txtrram+xb*blenz;
    //fix начальный сдвиг по zbe
    Fzbe=F0yp+zbebtm*F1yp;

    //защита от неверного попадания в текстуру src битмапа:
    //коррекция слишком низкого старта
    if (Fzbe<0) Fzbe=0;
/*
    //коррекция слишком высокого конца:
    //приводит к ~1-пиксельным искажениям текстуры битмапа вблизи
    //и сбоку панели, т.к. коррекция есть только на тех столбцах,
    //на которых виден верхний конец панели... @
    //добавление от 16 апреля: коррекция убрана из-за диких
    //искажений высоких панелей и панелей с неполной модой
    //геометрии !!! @@@
    {
    fix dzb,dzr;
    //длина текстуры битмапа от его нижней точки (видимой на экране)
    //до конца всего битмапа
    dzb=itofix(blenz-1)-Fzbe;
    //разность между длиной рендера=numz*F1yp и этой длиной текстуры
    dzr=numz*F1yp-dzb;
    if ((dzr>0)&&(numz>0))//если рендер длиннее текстуры
      F1yp=fixdiv(dzb,itofix(numz));//уменьшаем длину рендера
    }
*/
    //флуктуации пикселей по z
    if ((rfluct>0)&&((fluctflag&2)>0))
      {
      seed=RANDUMUL*seed+RANDUADD;
      //нижний пиксель будет колебаться вверх от номинала
      Fzbe+=(rfluct*((seed>>16)&0x1fff));//случайное от 0 до 0x1fff !
      //уменьшаем шаг для неперехлёста вверху, число точек = numz+1 !
      if (numz>0) F1yp-=(rfluct*0x1fff)/numz;
      }

    //начальная точка буфера рендера и y-буфера
    con1=xbe+zbebtm*partpenbmplenx;
    //двухшаговый рендер: правая половина требует сдвига начала в ноль
    if ((numpart==2)&&(actpart==1)) con1-=partpenbmplenx;
    rram=penram+con1;
    ybuf=ybuffer+con1;

    //внутренний цикл рендера по z
    zz=-numz;//начальное zz всех веток цикла!
    //PORT: cli and register juggling are not needed (flat memory)
    switch (rendsw)
      {
      case 0:/*
        for (zz=-numz; zz<=0; ++zz)
          {
          if (fypan<*ybuf)
            {
            colpix=*(tram+(int)(Fzbe>>16));
            *rram=gradcolor[colpix];
            *ybuf=fypan;
            }
          rram+=partpenbmplenx;
          ybuf+=partpenbmplenx;
          Fzbe+=F1yp;
          }*/
        for (; zz<=0; ++zz)
          {
          if (fypan<*ybuf)
            {
            colpix=*(tram+(int)(Fzbe>>16));
            *rram=gradcolor[colpix];
            *ybuf=fypan;
            }
          rram+=partpenbmplenx;
          ybuf+=partpenbmplenx;
          Fzbe+=F1yp;
          }
        break;

      case 1:/*
        for (zz=-numz; zz<=0; ++zz)
          {
          if (fypan<*ybuf)
            {
            colpix=*(tram+(int)(Fzbe>>16));
            if (colpix!=0)
              {
              *rram=gradcolor[colpix];
              *ybuf=fypan;
              }
            }
          rram+=partpenbmplenx;
          ybuf+=partpenbmplenx;
          Fzbe+=F1yp;
          }*/
        for (; zz<=0; ++zz)
          {
          if (fypan<*ybuf)
            {
            colpix=*(tram+(int)(Fzbe>>16));
            if (colpix!=0)
              {
              *rram=gradcolor[colpix];
              *ybuf=fypan;
              }
            }
          rram+=partpenbmplenx;
          ybuf+=partpenbmplenx;
          Fzbe+=F1yp;
          }
        break;

      case 2:/*
        for (zz=-numz; zz<=0; ++zz)
          {
          if (fypan<*ybuf)
            {
            colpix=convpal[*(tram+(int)(Fzbe>>16))];
            *rram=gradcolor[colpix];
            *ybuf=fypan;
            }
          rram+=partpenbmplenx;
          ybuf+=partpenbmplenx;
          Fzbe+=F1yp;
          }*/
        for (; zz<=0; ++zz)
          {
          if (fypan<*ybuf)
            {
            colpix=convpal[*(tram+(int)(Fzbe>>16))];
            *rram=gradcolor[colpix];
            *ybuf=fypan;
            }
          rram+=partpenbmplenx;
          ybuf+=partpenbmplenx;
          Fzbe+=F1yp;
          }
        break;

      case 3:
        //penram   имеет моду 0: слева вправо, потом вверх
        //текстура имеет моду 2: слева вверх, потом вправо
        //развертка PEN-битмапа по вертикали снизу вверх
  /*    for (zz=-numz; zz<=0; ++zz)
          {
          if (fypan<*ybuf)//если точка панели по y ближе точки ybuf,
            //она не загорожена предыдущим выводом
            {
            //преобразуем цвет текстуры в цвет пикселя syspal
            colpix=convpal[*(tram+(int)(Fzbe>>16))];
            //если цвет пикселя _не совпадает с прозрачным_
            if (colpix!=0)//вместо COLTRANS ставим здесь 0 !
              {
              //пропускаем цвет через фильтр и заносим пиксель в буфер цвета
              *rram=gradcolor[colpix];
              //меняем дистанцию в y-буфере на удаленность пикселя
              *ybuf=fypan;
              }
            }
          //шаг rram/ybuf =partpenbmplenx
          rram+=partpenbmplenx;
          ybuf+=partpenbmplenx;
          Fzbe+=F1yp;//шаг текстуры=F1yp
          }*/
        for (; zz<=0; ++zz)
          {
          if (fypan<*ybuf)
            {
            colpix=convpal[*(tram+(int)(Fzbe>>16))];
            if (colpix!=0)
              {
              *rram=gradcolor[colpix];
              *ybuf=fypan;
              }
            }
          rram+=partpenbmplenx;
          ybuf+=partpenbmplenx;
          Fzbe+=F1yp;
          }
        break;

      case 4://стекло !
  /*    for (zz=-numz; zz<=0; ++zz)
          {
          if (fypan<*ybuf)
            {
            colpix=*(tram+(int)(Fzbe>>16));
            //если текстура=стекло, затемняем старый пиксель экрана,
            //но не меняем y-буфер !!!
            if (colpix==0) *rram=gradcolor[*rram];
            else//рисуем пиксель непрозр-й текстуры и меняем y-буфер
              {
              *rram=colpix;
              *ybuf=fypan;
              }
            }
          rram+=partpenbmplenx;
          ybuf+=partpenbmplenx;
          Fzbe+=F1yp;
          }*/
        for (; zz<=0; ++zz)
          {
          if (fypan<*ybuf)
            {
            colpix=*(tram+(int)(Fzbe>>16));
            if (colpix==0) *rram=gradcolor[*rram];
            else
              {
              *rram=colpix;
              *ybuf=fypan;
              }
            }
          rram+=partpenbmplenx;
          ybuf+=partpenbmplenx;
          Fzbe+=F1yp;
          }
        break;

      }
    //PORT: register save/restore and cli/sti are not needed

    }
  }
}

