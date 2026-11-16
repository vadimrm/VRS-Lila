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

#pragma  inline
#include <dos.h>
#include <values.h>
#include "vrs49.h"


#define MAXGRADCOL1   MAXGRADCOL-1
//переменные внутреннего цикла рендера
byte near *gradcolor;
byte near *rram;
byte far *convpal;
byte far *tram;
int far *ybuf;
int colpix,zz,numz,fypan;
fix Fzbe,F1yp;
dword regesp,regebp,regesi,regedi;
word regss,reges;
void RenderPanel(int srcpan)
//рендер панели srcpan в память PEN-панели penram[]
{
byte far *txtrram;//src память с текстурой панели
int far *ybuffer;//Z-буфер
int srcmem,blenz,blenx,xb0,xb1,dxb10,dxb10p,zb0,xb,dxb;
int usecolortrans,useconvpal,xbe,xbelft,xbergt,sw;
int memconv,zbebtm,zbetop,delta,con1,grcol,grcol0;
//fix-ация:
fix fmulx,fmulx1,fmulz,maxfmulz,srcpanflenx,srcpanflenz;
fix Fdn,Xb1,Xp1,Xs,C0,C1,F0yp,Yfg,Yp,Flenx,Flenz;
int nepr,steklo;
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

srcmem=panel[srcpan].memtex;
//преобразование моды развертки src памяти
if (memory[srcmem].rammode!=2) ConvertMEMmode(srcmem,2);

//номер блока памяти таблицы преобразования цветов src панели
memconv=panel[srcpan].memconv;
if (memconv>ISNOT)//есть memconv, цвета панели меняются
  {
  //рендер с преобразованием цветов и с прозрачным цветом!
  useconvpal=usecolortrans=YES;
  //таблицу преобразования цветов берем из memconv панели
  convpal=memory[memconv].rampntr;
  }
else//цвета панели не меняются
  {
  useconvpal=NO;//рендер без преобразования цветов
  //прозрачный рендер зависит от наличия прозрачности в src битмапе
  usecolortrans=memory[srcmem].transflag;
  }

txtrram=memory[srcmem].rampntr;
//PEN.ymem Ram-лочить здесь не обязательно, это делает вызывающий код !
ybuffer=(int far *)memory[PEN.ymem].rampntr;
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

blenx=memory[srcmem].lenx;
xb0=panel[srcpan].xb0;
xb1=panel[srcpan].xb1;
//приводим xb0, xb1 к интервалу 0...(blenx-1),
xb0%=blenx;
xb1%=blenx;
dxb10=xb1-xb0+1;
dxb10p=blenx+xb1-xb0+1;
blenz=memory[srcmem].lenz;
zb0=panel[srcpan].zb0;
//zb1 пока не используется...

nepr=panel[srcpan].neproz;
if (nepr>0)//стеклянная панель: градации прозрачности
  {
  steklo=YES;
  nepr+=neprozra;//добавляем общую непрозрачность
  if (nepr<0) nepr=0;
  if (nepr>MAXGRADCOL1) nepr=MAXGRADCOL1;
  //вместо градаций яркости будет непрозрачность
  gradcolor=&gradcol[nepr][0];
  }
else//нестеклянная: градации яркости
  {
  steklo=NO;
  //постоянная градация панели: в рендере яркое отрицательно!
  grcol0=grcol=gradcon-panel[srcpan].consti;
  //на случай _не_регулировки градаций внутри панели
  if (grcol>MAXGRADCOL1) grcol=MAXGRADCOL1;
  if (grcol<0) grcol=0;
  gradcolor=&gradcol[grcol][0];
  }

rfluct=panel[srcpan].rand;
if (rfluct>=0) rfluct+=randfluct;//добавка флуктуаций !

//идеальные размеры панелей минус запас !
Flenx=srcpanflenx-ZAPASX;
Flenz=srcpanflenz-ZAPASZ;
if (Flenx<0) Flenx=0;
if (Flenz<0) Flenz=0;

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
    Xp1=Flenx>>1;//тогда просто берем ~середину панели...

  //защита от сильной грязи из-за возможных больших ошибок Xp1
  if ((Xp1<0)||(Xp1>Flenx)) continue;
  //дальность ребра Xp1
  Yp=Y0+fixmul(Xp1,fsin);
  //эта проверка также необходима из-за возможных ошибок Xp1
  if (Yp<FIXYPMIN) continue;

  //регулировка градаций внутри панели
  if ((steklo==NO)&&(gradvarflag==YES))
    {
    //пространственные колебания градаций
    grcol=255&pfixtoi(gradfx*(Xp1>>8)+gradfy*(Yp>>8));
    grcol= (grcol<128)? grcol:(255-grcol);
    //постоянная градация плюс переменная
    grcol=grcol0+((gradamp*grcol)>>8);
    //определяем слой цветов данной градации
    if (grcol>MAXGRADCOL1) grcol=MAXGRADCOL1;
    if (grcol<0) grcol=0;
    gradcolor=&gradcol[grcol][0];
    }

  //PEN-pan Z-концы от src панели на ребре Xp1, соответствующем xs
  //тут переполнение при делении практ. невероятно...
  Zpebtm=lfixmuldiv(val_h,Z0,      Yp)-penpozz0;
  Zpetop=lfixmuldiv(val_h,Z0+Flenz,Yp)-penpozz0;
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
  if (dxb10>0)//xb0<=xb1
    //циклическая развертка битмапа xb0...xb1,
    xb=xb0+dxb%dxb10;
  else//xb0>xb1
    //циклическая развертка xb0...(lenx-1)->потом->0...xb1,
    //что сильно облегчает динамический скроллинг битмапа !
    xb=(xb0+dxb%dxb10p)%blenx;

  //грубое Yp для далеких точек панели
  fypan=pfixtoi(Yp);
  //для близких y близи точность Yp для рендера увеличиваем так:
  Yfg=Yp-DYFG;
  //если Yp<DYFG, то fypan=128*(Yp-DYFG)/65536, что в 128 раз растягивает
  //диапазон Yp=0...DYFG в сторону отрицательных расстояний, причем на
  //DYFG см происходит скачек fypan до 0, далее равномерно до -128*DYFG
  //сдвиг вправо на 9 бит, что на 7 бит точнее обычных 16
  if (Yfg<0) fypan=-(int)((-Yfg)>>YFGSHIFT);

  //подходим к внутреннему циклу
  F0yp=C0+fixmuldiv(itofix(penpozz0),Yp,C1);
  F1yp=fixmuldiv(penbmpfmulz,Yp,C1);

  //точка zb=0 текстуры битмапа столбца xbe
  tram=txtrram+xb*blenz;
  //fix начальный сдвиг по zbe
  Fzbe=F0yp+zbebtm*F1yp;

  //защита от зашкала на битмапе снизу
  if ((Fzbe<0)&&(F1yp!=0))//пропускаем _все_ лишние пиксели снизу экрана
    {
    delta=pfixtoi(fixdiv(-Fzbe,F1yp))+1;
    if ((numz-=delta)<0) continue;
    zbebtm+=delta;
    Fzbe=F0yp+zbebtm*F1yp;
    }
  //защита от зашкала на битмапе сверху
  //разность между верхом развертки битмапа и верхом его текстуры
  delta=pfixtoi(Fzbe+numz*F1yp)-(blenz-1);
  if (delta>0) if (--numz<0) continue;//-1 пиксель сверху экрана,
  //для полнобитмаповой панели это ~исключает неточность вверху !

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

  //определение номера ветви рендера
  sw=0;
  if (usecolortrans==YES) sw+=1;//с проверкой на прозрачный цвет
  if (useconvpal==YES)    sw+=2;//с использованием таблицы convpal
  if (steklo==YES) sw=4;

  //внутренний цикл рендера по z
  zz=-numz;//начальное zz всех веток цикла!
  asm{
     //из-за использования стековых регистров необходимо в течении
     //всего z-цикла (это обычно не более 1 мсек) запретить irq!!!
     cli
     //заталкиваем рабочие регистры z-цикла в RAM (кроме eax...edx)
     mov   dword ptr DGROUP:regesp,esp
     mov   dword ptr DGROUP:regebp,ebp
     mov   dword ptr DGROUP:regesi,esi
     mov   dword ptr DGROUP:regedi,edi
     mov   word ptr DGROUP:regss,ss
     mov   word ptr DGROUP:reges,es
     }
  switch (sw)
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
      asm{
         mov   bp, word ptr DGROUP:rram //byte near *rram -> (ds):bp
         les   bx,dword ptr DGROUP:tram //byte far  *tram -> es:bx
         mov   esp,0
         lfs   sp,dword ptr DGROUP:ybuf //int  far  *ybuf -> fs:(e)sp
         mov   ecx,dword ptr DGROUP:Fzbe
         mov   si,word ptr DGROUP:partpenbmplenx
         mov   ax,word ptr DGROUP:fypan
         mov   gs,ax
         mov   edx,dword ptr DGROUP:F1yp
         mov   edi,0
         mov   di,word ptr DGROUP:gradcolor
         dec   word ptr DGROUP:zz
         inc   word ptr DGROUP:zz //для проверки zz в конце цикла
	 jmp   short lab0n2
         }
      lab0n0:
      asm{
         mov   ax,gs                // fupan -> ax
         cmp   ax,word ptr fs:[esp] // (fypan < *ybuf) ?
         jge   short lab0n1        //jump if >=
	 mov   eax,ecx
	 shr   eax,16
         add   ax,bx
	 mov   al,byte ptr es:[eax]     //*(tram+(int)(Fzbe>>16)) -> al
         mov   ah,0                     //обнуляем eax кроме al
         mov   al,byte ptr ds:[edi+eax] //gradcolor[colpix] -> al
	 mov   byte ptr ds:[bp],al  //al -> *rram
	 mov   word ptr fs:[esp],gs //*ybuf=fypan
         }
      lab0n1:
      asm{
	 add   bp,si                //rram+=partpenbmplenx
         add   ecx,edx              //Fzbe+=F1yp
	 add   sp,si;   add   sp,si //ybuf+=partpenbmplenx
         inc   word ptr DGROUP:zz   //++zz
         }
      lab0n2://конец z-цикла
      asm jle  short lab0n0        //jump if zz <= 0
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
      asm{
         mov   bp, word ptr DGROUP:rram //byte near *rram -> (ds):bp
         les   bx,dword ptr DGROUP:tram //byte far  *tram -> es:bx
         mov   esp,0
         lfs   sp,dword ptr DGROUP:ybuf //int  far  *ybuf -> fs:(e)sp
         mov   ecx,dword ptr DGROUP:Fzbe
         mov   si,word ptr DGROUP:partpenbmplenx
         mov   ax,word ptr DGROUP:fypan
         mov   gs,ax
         mov   edx,dword ptr DGROUP:F1yp
         mov   edi,0
         mov   di,word ptr DGROUP:gradcolor
         dec   word ptr DGROUP:zz
         inc   word ptr DGROUP:zz //для проверки zz в конце цикла
	 jmp   short lab1n2
         }
      lab1n0:
      asm{
         mov   ax,gs                // fupan -> ax
         cmp   ax,word ptr fs:[esp] // (fypan < *ybuf) ?
         jge   short lab1n1        //jump if >=
	 mov   eax,ecx
	 shr   eax,16
         add   ax,bx
	 mov   al,byte ptr es:[eax]     //*(tram+(int)(Fzbe>>16)) -> al
         cmp   al,0                     //(colpix != 0) ?
         je    short lab1n1            //jump if ==
         mov   ah,0                     //обнуляем eax кроме al
         mov   al,byte ptr ds:[edi+eax] //gradcolor[colpix] -> al
	 mov   byte ptr ds:[bp],al  //al -> *rram
	 mov   word ptr fs:[esp],gs //*ybuf=fypan
         }
      lab1n1:
      asm{
	 add   bp,si                //rram+=partpenbmplenx
         add   ecx,edx              //Fzbe+=F1yp
	 add   sp,si;   add   sp,si //ybuf+=partpenbmplenx
         inc   word ptr DGROUP:zz   //++zz
         }
      lab1n2://конец z-цикла
      asm jle  short lab1n0        //jump if zz <= 0
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
      asm{
         mov   bp, word ptr DGROUP:rram //byte near *rram -> (ds):bp
         les   bx,dword ptr DGROUP:tram //byte far  *tram -> es:bx
         mov   esp,0
         lfs   sp,dword ptr DGROUP:ybuf //int  far  *ybuf -> fs:(e)sp
         mov   ecx,dword ptr DGROUP:Fzbe
         mov   si,word ptr DGROUP:partpenbmplenx
         mov   ax,word ptr DGROUP:fypan
         mov   gs,ax
         mov   edx,0
         lss   dx,dword ptr DGROUP:convpal //byte far *convpal -> ss:(e)dx
         mov   edi,0
         mov   di,word ptr DGROUP:gradcolor
         dec   word ptr DGROUP:zz
         inc   word ptr DGROUP:zz //для проверки zz в конце цикла
	 jmp   short lab2n2
         }
      lab2n0:
      asm{
         mov   ax,gs                // fupan -> ax
         cmp   ax,word ptr fs:[esp] // (fypan < *ybuf) ?
         jge   short lab2n1        //jump if >=
	 mov   eax,ecx
	 shr   eax,16
         add   ax,bx
	 mov   al,byte ptr es:[eax]     //*(tram+(int)(Fzbe>>16)) -> al
         mov   ah,0                     //обнуляем eax кроме al
         mov   al,byte ptr ss:[edx+eax] //(colpix=convpal[al]) -> al
         mov   al,byte ptr ds:[edi+eax] //gradcolor[colpix] -> al
	 mov   byte ptr ds:[bp],al  //al -> *rram
	 mov   word ptr fs:[esp],gs //*ybuf=fypan
         }
      lab2n1:
      asm{
	 add   bp,si                //rram+=partpenbmplenx
         add   ecx,dword ptr DGROUP:F1yp //Fzbe+=F1yp
	 add   sp,si;   add   sp,si //ybuf+=partpenbmplenx
         inc   word ptr DGROUP:zz   //++zz
         }
      lab2n2://конец z-цикла
      asm jle  short lab2n0        //jump if zz <= 0
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
      asm{//начало z-цикла
         //переменные помещаем в регистры
         mov   bp, word ptr DGROUP:rram //byte near *rram -> (ds):bp
         les   bx,dword ptr DGROUP:tram //byte far  *tram -> es:bx
         mov   esp,0
         lfs   sp,dword ptr DGROUP:ybuf //int  far  *ybuf -> fs:(e)sp
         mov   ecx,dword ptr DGROUP:Fzbe
         mov   si,word ptr DGROUP:partpenbmplenx
         mov   ax,word ptr DGROUP:fypan
         mov   gs,ax
         mov   edx,0
         lss   dx,dword ptr DGROUP:convpal //byte far *convpal -> ss:(e)dx
         mov   edi,0
         mov   di,word ptr DGROUP:gradcolor
         dec   word ptr DGROUP:zz
         inc   word ptr DGROUP:zz //для проверки zz в конце цикла
	 jmp   short lab3n2
         }
      lab3n0:
      asm{
         mov   ax,gs                // fupan -> ax
         cmp   ax,word ptr fs:[esp] // (fypan < *ybuf) ?
         jge   short lab3n1        //jump if >=
	 mov   eax,ecx
	 shr   eax,16 //здесь код с защитой от выхода смещения
         add   ax,bx  //es:[eax] за границу сегмента текстуры,
         //иначе в VirtualMode86 exeption 13 на следующей команде:
	 mov   al,byte ptr es:[eax]     //*(tram+(int)(Fzbe>>16)) -> al
         mov   ah,0                     //обнуляем eax кроме al
         mov   al,byte ptr ss:[edx+eax] //(colpix=convpal[al]) -> al
         cmp   al,0                     //(colpix != 0) ?
         je    short lab3n1            //jump if ==
         mov   al,byte ptr ds:[edi+eax] //gradcolor[colpix] -> al
	 mov   byte ptr ds:[bp],al  //al -> *rram
	 mov   word ptr fs:[esp],gs //*ybuf=fypan
         }
      lab3n1:
      asm{
	 add   bp,si                //rram+=partpenbmplenx
         add   ecx,dword ptr DGROUP:F1yp //Fzbe+=F1yp
	 add   sp,si;   add   sp,si //ybuf+=partpenbmplenx
         inc   word ptr DGROUP:zz   //++zz
         }
      lab3n2://конец z-цикла
      asm jle  short lab3n0        //jump if zz <= 0
      break;

    case 4://стекло !
/*    for (zz=-numz; zz<=0; ++zz)
        {
        if (fypan<*ybuf)
          {
          colpix=*(tram+(int)(Fzbe>>16));
          //если текстура=стекло, затемняем старый пиксель экрана:
          if (colpix==0) colpix=gradcolor[*rram];
          *rram=colpix;
          *ybuf=fypan;
          }
        rram+=partpenbmplenx;
        ybuf+=partpenbmplenx;
        Fzbe+=F1yp;
        }*/
      asm{
         mov   bp, word ptr DGROUP:rram //byte near *rram -> (ds):bp
         les   bx,dword ptr DGROUP:tram //byte far  *tram -> es:bx
         mov   esp,0
         lfs   sp,dword ptr DGROUP:ybuf //int  far  *ybuf -> fs:(e)sp
         mov   ecx,dword ptr DGROUP:Fzbe
         mov   si,word ptr DGROUP:partpenbmplenx
         mov   ax,word ptr DGROUP:fypan
         mov   gs,ax
         mov   edx,dword ptr DGROUP:F1yp
         mov   edi,0
         mov   di,word ptr DGROUP:gradcolor
         dec   word ptr DGROUP:zz
         inc   word ptr DGROUP:zz //для проверки zz в конце цикла
	 jmp   short lab4n2
         }
      lab4n0:
      asm{
         mov   ax,gs                // fupan -> ax
         cmp   ax,word ptr fs:[esp] // (fypan < *ybuf) ?
         jge   short lab4n1         //jump if >=
	 mov   eax,ecx
	 shr   eax,16
         add   ax,bx
	 mov   al,byte ptr es:[eax] //*(tram+(int)(Fzbe>>16)) -> al
         cmp   al,0                 //(colpix == 0) ?
         jne   short lab4n3         //jump if !=
         mov   al,byte ptr ds:[bp]  //*rram -> al
         mov   ah,0                     //обнуляем eax, кроме al
         mov   al,byte ptr ds:[edi+eax] //gradcolor[*rram] -> al
         }
      lab4n3:
      asm{
	 mov   byte ptr ds:[bp],al  //al -> *rram
	 mov   word ptr fs:[esp],gs //*ybuf=fypan
         }
      lab4n1:
      asm{
	 add   bp,si                //rram+=partpenbmplenx
         add   ecx,edx              //Fzbe+=F1yp
	 add   sp,si;   add   sp,si //ybuf+=partpenbmplenx
         inc   word ptr DGROUP:zz   //++zz
         }
      lab4n2://конец z-цикла
      asm jle  short lab4n0        //jump if zz <= 0
      break;

    }
  asm{//вытаскиваем регистры обратно
     mov   esp,dword ptr DGROUP:regesp
     mov   ebp,dword ptr DGROUP:regebp
     mov   esi,dword ptr DGROUP:regesi
     mov   edi,dword ptr DGROUP:regedi
     mov   ss,word ptr DGROUP:regss
     mov   es,word ptr DGROUP:reges
     sti   //разрешаем irq
     }

  }
}

int j0;//это здесь для ручного ASM ...
byte far *dram;
byte near *pram;
void RenderD(void)
//рендер дали: далёкие горы, облака и проч.
//горизонтальный масштаб учитывает текущее val_h !
{
int jsmin,i,k,hlen,vlen,js,je,jscol,dlen,dh,dshift;
byte far *daliram;
static dword dalia;
static int dhorshift;
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
  dalia+=dalidadt*cadrtime;
  angx=PEN.ear.angx+(angle)(dalia>>4);
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
    asm{
       mov  ax,0
       mov  cx,word ptr DGROUP:_j0
       les  bx,dword ptr DGROUP:_dram
       mov  dx,word ptr DGROUP:_dtranscol
       mov  si,word ptr DGROUP:_pram
       mov  di,word ptr DGROUP:_partpenbmplenx
       dec  cx
       inc  cx
       jmp  short lab1
       }
    lab2:
    asm{
       mov  al,byte ptr es:[bx]
       inc  bx
       cmp  ax,dx
       je   short lab3
       mov  byte ptr [si],al
       }
    lab3:
    asm{
       add  si,di
       inc  cx
       }
    lab1: asm jle  short lab2
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
    asm{
       mov  ax,0
       mov  cx,word ptr DGROUP:_j0
       les  bx,dword ptr DGROUP:_dram
       mov  si,word ptr DGROUP:_pram
       mov  di,word ptr DGROUP:_partpenbmplenx
       dec  cx
       inc  cx
       jmp  short lab10
       }
    lab20:
    asm{
       mov  al,byte ptr es:[bx]
       inc  bx
       mov  byte ptr [si],al
       add  si,di
       inc  cx
       }
    lab10: asm jle  short lab20
    }
  }
}

