/*                             _ _
                      Система ЛИЛА
                 Дата ...... 22-23.06.96
*/
char VERS[8]="0.57";
#include "vrsport.h"
#include    "vrs57.h"
#include    "kbcod.h"
/*   Файлы:   проекта ... vrs57.c,vrs57ap1.c,vrs57c.c,vrs57r.c,
                          vrs57s.c,vrs54a.obj
       для выполнения ... sinus.lii, keyboard.lii, font.lii,
         опциональные ... [syspal.lii] ll.lii, *.pp, *.ll
         документации ... help.doc, lila.txt, vrs57.txt
      для инсталляции ... [vrs49i.exe]

  Новое:
  -вторая компиляция (и первый прорыв вперед) в Москве !!!!
  -по Esc запрос на выход и мини-хелп выхода (F1/F10/Esc) !
  -если клиппинга нет - панель пера рендерится до стекла !!
  -острие пера в фикс. позиции к текущим размерам экрана !!
  -горизонтальная и вертикальная псевдостереопара по методу
   наложения предыдущего кадра на следующий !!!!!!!!!!!!!!!
  -рендер: не меняем y-буфер под пикселями стекла: исчезает
   клиппинг, но не исчезают сами панели из стекла !!!!!!!!!
  -быстрее движения юзера, медленнее слом панелей ПП1 !
  -удаление и изменение хоткеев!

  Надо:
  -стереопара!
  -перо для стерео!

  Позже:
  -перепросмотр глобальных переменных и кода!
  -исправить перехлест текстуры по вертикали - новые формулы ???
  -простой хоткей-генератор случайных лабиринтов!
  -другие фракталы дали, еще цвета земли...

  Важно:
  -4500 байт (минус 4096 стек) near RAM свободно!
*/

#include <sys\stat.h>
#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <malloc.h>

//******* локально-глобальные переменные и определения

//extern word _stklen; //обычно 4 кб
//extern word _heaplen;//обычно 0

//******* это в h-файл:


//******* это уже есть в h-файле:

int penbmpsize=5;//переключатель размеров экрана

//флаг палитры из pp-файла: 0=используется syspal, 1=pppal->syspal,
//2=загрузка pp-файла без палитры, 3=восстановление syspal:
int palfromppfile;

//нештатный рендер прозрачного цвета для нестеклянных панелей:
int rendtransflag=0;//0=штатный, 1=без, 2=с прозрачным цветом;

int soundseed;//зерно генерации случайных нот;

int notediv[MAXNOTE+1]={//делители частоты таймера для нот нижней октавы
18243,17219,16252,15340,14479,13667,12899,12175,11492,10847,10238,9664,};

//состояние проигрывания звука: 0=запрос на стоп, 1=стоп,
int soundstate;//2=старт, 3=текущее проигрывание звука;
int notecash[MAXCASH];//кэш нот для текущего проигрывания
int numnotecash;//число нот в кэше --//--
int durnote;//длительность одной ноты в миллисекундах;
int noisediv=MAXINT;//ширина шума = (частота звука)/noisediv;

//номера нот лада: это ~3 октавы пентатоники !
int notelad[16]={0,2,5,7,9, 12,14,17,19,21, 24,26,29,31,33, 36};

int soundflag=OFF;//флаг разрешения звука

d3poz ap1startpoz={0,2000*FIX1,0,0};//позиция AP_First после ColdStart

char llfile[16];//имя ll-файла
long llfilesize;//и размер его

//параметры всех групп панелей текущего лабиринта
LABdata far labdata[LLBMS];
//размеры панели и сдвиг лабиринта внутри кода его вызова!

int sysbms[SYSBMS],syscvs[SYSBMS];//системный набор битмапов и конвертов

dword daliang=4*L65536;//((угол дали)<<4)=90 градусов

int simsimflag=NO;//флаг симметричного рендера панели по x

//pp-файлы дали:          облака и горы
char ppfile[2][12]={"PP\\far0.pp","PP\\far1.pp"};

//статические номера битмапов с текстурами стены и стекла
int wallbitm,glassbitm;

int nebocoli=4;//индекс цвета неба;
//nebocol[nebocoli]=абсолютный номер цвета неба:
int nebocol[10]={236,32,25,65,69,82,84,96,185,208};

byte zbitm[16];//псевдо-битмап земли
int dalibitm;//номер битмапа дали;

//параметры раскраски цветов 1...8:
int newbgpar=YES;//флаг новых параметров bgpar
int bgvar=4;//текущий номер варианта цветов
//цвет цвета 1   множитель шага   шаг цвета    яркость цвета
//r,g,b=0...63,  dr,dg,db=0...1,  step=0...8,  power=0...64
//[][0,1,2]      [][3,4,5]        [][6]        [][7]
int bgpar[BGVAR][BGPAR]={//параметры по вариантам
{ 8,63,63, 2,0,0, 2, 53},//0
{58,30, 0, 1,1,0, 1, 24},//1
{ 8,63,63, 2,0,0, 1, 26},//2
{27,31,63, 0,0,1, 1, 38},//3
{57,51,33, 0,0,1, 1, 32},//4
{44,63, 0, 1,0,0, 1, 32},//5
{63,63,63, 1,0,0, 1, 32},//6
{30, 0,63, 0,0,1, 1, 55},//7
{27,42,18, 0,0,2, 1, 64},//8
{63,32, 0, 0,1,0, 1, 56},//9
};

//параметры дали:
int dalinali=YES;//флаг рендера дали
int dalipaint;//битмап дали [пере]красить надо ли ?
int dali2d;//двумерность битмапа дали
int newdali;//флаг новых параметров дали
int dalivar;//номер варианта дали
int dnumpar=2;//номер параметра для регулировки
int dalipar[DALIVAR][DALIPAR]={
//параметры дали по вариантам
//0: даль из 2D-битмапа (облаков) с прозрачным цветом
{  0,  0,  0,  0,    0,256,  0,     0,  0,  0,     0,  0,  200,  0,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//  геометрия
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0},// цветометрия
//1:       2D горы с откусанными снизу 40 строками         160=200-40
{  0,  0,  0,  0,    0,256,  0,    64, 40,  0,     0,  0,  160,  0,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//  геометрия
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0},// цветометрия

//2: 1D-даль       горы
{ -1,  1, 618, 34,   0,257,-21,    64, 69, 64,     7, -1,   35, 77,
   0, 21,   -84, 19,     0,  0,     0,  0,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,
 -25, 25,   -34,  0,     0,  0,     0,  0,   -10, 10,
   0,  0,     0,  0,   -50, 50,   -39, 15,     0,  0},
//3: 1D-даль       горы
{ -1,  1,242, 32,    0,256,-16,     0,  0, 32,     0,  0,   20, 50,
   0, 40,   -10, 30,   -10, 30,   -10, 20,   -10, 10,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,    -1,  1,//  геометрия
   0, 40,   -10, 30,   -10, 30,   -10, 20,   -10, 10,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,    -3,  4},//  цветометрия
//4: 1D-даль       горы
{ -1,  1,-11, 32,    0,256,-16,     0,  0, 32,     1,  1,    0, 50,
   0, 40,   -10, 30,   -10, 30,   -10, 20,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,    -1,  1,
   0, 40,   -10, 30,   -10, 30,   -10, 20,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,    -3,  4},
//5: 1D-даль       горы
{ -1,  1,1428, 32,   0,256,-16,     0,  0, 32,     0,  0,   40, 50,
   0,  0,     0,  0,     0,  0,   -10, 10,   -10, 10,
 -10, 10,     0,  0,     0,  0,     0,  0,     0,  0,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,
 -10, 10,   -10, 10,   -10, 10,   -10, 10,   -10, 10},
//6: 1D-даль       горы
{ -1,  1, 592, 32,   0,256,-16,     0,  0, 32,     0,  0,   50, 50,
   0,  0,     0,  0,     0,  0,     0,  0,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,
   0,  0,     0,  0,     0,  0,     0,  0,   -10, 10,
   0,  0,     0,  0,   -50, 50,   -50, 50,     0,  0},
//7: 1D-даль       лес
{ -1,  1,1480,-108,    0,256,-64,   0,  0, 64,     0,  0,   10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,    -3,  2,
   0,  0,     0,  0,     0,  0,     0,  0,    -1,  1,
   0,  0,     0,  0,     0,  0,     0,  0,    -5,  5,
   0,  0,     0,  0,     0,  0,     0,  0,    -1,  1},
//8: 1D-даль       облака
{ -1,  1, -12,  13,   70,256,-64,   0,  0, 32,     0,  0,  200, 50,
   0,  0,     0,  0,     0,  0,     0,  0,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,
   0,  0,     0,  0,     0,  0,     0,  0,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0},

//9: 1D-даль для тестирования цветов
{ -1,  0,  0,256,  -60,256,  0,     0,  0,  0,     0,  0,  260,  0,}
};

//номер прозрачного цвета для дали (0...255, нет его при <0):
int dtranscol;//глобальная копия dalipar[][0] !!!
//отклонение нулевого уровня дали от уровня горизонта:
int dalidjear;//глобальная копия dalipar[][4] !!!

int dalidadt;//скорость вращения дали по углу - для облаков!

//строка периметра дали с параметрами столбцов
//в трёх массивах 1+1+2=4 кб, можно near/far
byte  far dalistrl[STRLEN];//длина столбца дали по вертикали, 0...255
sbyte far dalistrh[STRLEN];//вертикальный уровень начала столбца, -128...+127
int   far dalistrs[STRLEN+1];//смещение начала столбца в битмапе дали,
//0...65535 (word !); началом столбца дали считается его нижний пиксель!

int spaceflag;//флаг космоса: OFF=земля и небо, ON=везде небо!
int starvar=2;//номер варианта, ему ~пропорционально количество звёзд!

int jearuser;//строка близкой к юзеру земли на экране
//уровень горизонта проходит по Z=0 USER-системы, если считать
//номер строки низа PEN-bmp =0, то горизонт будет на строке:
int jear;//и может быть вне экрана!

int horshift,horshift360=1;//см AP_Render

int stepvalue;//флаг-шаг=(1+7*CapsLock)*(8+56*Shift)/(1+7*Alter)=1...512;

fix DYuser;//очередное линейное смещение юзера для плаванья по цветам земли;

int allscrollx;//при panel[].scrollx!=0: плюс к параметру скроллинга;
int scrollflag=ON;//общий флаг разрешения скроллинга;

int randfluct;//при panel[].rfluct>0: плюс к флуктуациям пикселей;
int fluctflag=3;//0-й бит: флаг флуктуаций по x, 1-й бит - по z !

//параметры изменения градаций панели:
int gradconst;//общая постоянная компонента;
//бит0: флаг постоянных градаций, бит1: флаг градаций по x,
int gradvarflag=3;//бит2: флаг градаций по y панели;
int gradampy=20;//регулятор амплитуды изменений градаций яркости по y;
int gradleny=1024;//длина волны по y, в см !
int graddinflag=1;//флаг (~сила) динамической не-яркости;

int neprozra;//общая постоянная компонента не-прозрачности стекла;

//эти параметры влияют на панели только при установки лабиринта:
int labglass,labgradcon,labgradampx,labrfluct;
int labdscrollx;//delta-скролл лабиринта!

int SPmin=MAXINT,SPmax=-MAXINT;//(порт: контроль стека не имеет смысла)


long Seed,Seed2,Seed32;
word Seed16;

int kbbufvalid=NO;//флаг разрешения буферизации кодов клавиш
volatile int anycodflag=0;//флаг для проверки был ли ввод любой из клавиш;
byte scod2kbcod[256];//таблица перекодировки scod->kbcod;
KeyboarD kbbuf[KBBUF];//кольцевой буфер кодов клавиатуры
volatile int kbwrite=0;//позиция для новой записи
int kbread=0;//позиция для нового чтения этого буфера, т.о.
  //читать надо блок от kbread до kbwrite-1 включительно!

MessagE msg[MSGBUF];//кольцевой буфер сообщений VRS и AP
int msgwrite=0;//позиция для новой записи
int msgread=0;//позиция для нового чтения этого буфера, т.о.
  //читать надо блок от msgread до msgwrite-1 включительно!

APdata AP[MAXAP];//массив структур данных AP, индекс=адрес AP!
int numap=ADDR_LAST_AP;//адреса AP в текущей версии системы=1...numap;

PANdata far panel[MAXPAN];//массив панелей
MEMdata memory[MAXMEM];//массив заголовков блоков памяти
dword ramfullfree;//текущий размер всей свободной нижней far-RAM
dword ramfree;//текущий размер непрерывного куска до конца RAM
// +- с far указателем меняют только его offset, здесь надо huge*
byte huge *ramfullstart;//начало RAM для всего набора блоков памяти
byte huge *ramstart;//начало непрерывного куска до конца RAM

volatile dword vrs_time;//счетчик VRS времени в мсек !
volatile word  vrstime1,vrstime0;//'половинки' vrs_time для irq1 !
dword oldtime=0;
int cadrtime;//время прорисовки кадра в мсек;

int startscrnmode;//номер видео-моды до и после программы
int actscrnmode;//текущий номер видео-моды;

Font fnt;//текстовый фонт 8x8

d3poz d3null;//нулевая D3 позиция

fix far Fsin[NUMSIN];//Fsin[4096]=sin(90 градусов)!
angle Alpha;//угол 'альфа' в формулах рендера, в лидусах!
fix fsin,fcos;//sin(Alpha),cos(Alpha);

//расстояние от глаза наблюдателя до экрана наблюдения:
int val_h=200;//500 соответствует реальности для экрана 320x200,
//но для запаса по зашкалу и увеличения обзора лучше брать
//int max_val_h=200;//val_h<=200 !
fix X0,Y0,Z0;//координаты оригина панели
long Zpetop,Zpebtm;//диапазон предполагаемой Z-развертки PEN-pan
//long вместо int из-за возможного выхода за границу int !

int clip=YES;//флаг клиппинга;

PenPan PEN;//структура 'панель экрана наблюдения' или PEN-панель
byte near *penram;//PEN.ram после выравнивания по dword-ам;
//часто глобально-локально используемые дубли PEN-полей
int penpozx0,penpozz0,penpanlenz;//int а не fix !!!
//PEN-битмап поля вне структуры PenPan !
//битмап масштабируется в PEN с коэффициентом z-размеров bmp
fix penbmpfmulz;//=6/5
fix penbmpfmulz1;//=1/penbmpfmulz=5/6
int penbmplenx=320,penbmplenz=200;//полные размеры PEN-битмапа, в пикселях
int partpenbmplenx=160;//x-размер его для текущего шага рендера
//число шагов подготовки полного экрана (в рендере 1...2)
int numpart=2;
int actpart;//номер текущего шага, начиная с 0;

byte far nnn[16][16][16];//таблица для расчетов цветов;
byte far syspal[PAL768];//VRS-палитра (модиф-я палитра 5.04):
//[rgb компоненты цвета=0...60 с шагом 4]
//#define STARTCOL   9//от цвета номер 9
//#define ENDCOL   255//до цвета номер 255,
//всего 247 цветов (как и в 5.04, но там цвета от 0 до 246);
//цвета 0...8 не входят в syspal и служат для специальных применений:
//цвет 0=цвет рамки и пустых мест экрана, всегда чёрный
//цвета 1...8=цвета плавной земли, всегда не чёрные
//цветом 7 на фоне 0 печатает досовская printf() !!

byte near gradcol[MAXGRADCOL][256];//таблица градаций яркости цветов syspal;

int mousepresent=NO;//флаг наличия мыши
int mousedx,mousedy;
int mouseb0,mouseb1;//клавиша b0/b1 мыши нажата=1, ненажата=0
//числа нажатий/отжатий b0/b1 за последний кадр
int mouseb0on,mouseb0off,mouseb1on,mouseb1off;

//параметры указки в любой моде:
int pointermode=0;//мода указки(pointer):
  //0=карандаш(pencil, он похож скорее на перо), 1=курсор(cursor),
int pnclpan,curpan;//номер панели указки-карандаша/указки-курсора
int pnclbm,curbm;//соответственно номера их битмапов;

//параметры указки в моде0=карандаш
int pnclmousemode=0;//0=мышь перемещает юзера, 1=карандаш
int pnclvisible=NO;//видимость карандаша
int pnclpozflag=1;//позиция его 0,1,2,3;
fix fpnclx,fpnclz;//x/z пикселя острия пера относительно размеров экрана,
int pncldz;//и смещение пикселя острия пера от оригина его битмапа по z !
angle pnclangx;//угол наклона и
fix fpncly;//y-координата _панели_  карандаша!
int pnclmmm;//pencil moves mouse mode: 0=x/y, 1=x/z;

//параметры указки в моде1=курсор:
int curmousemode;//0=мышь перемещает юзера, 1=курсор
int curnumap=ADDR_VRS;//адрес 'выбранной' AP или адрес системы:
  //это обладатель текущего ввода с клавиатуры!
int curnumpan=ISNOT;//номер 'выбранной' панели, ISNOT если нет:
  //это панель 'выбранной' AP (не VRS), на которую юзер положил курсор!
int curvisible;//видимость курсора;
fix fcurlenx,fcurlenz;//размеры курсора,
fix fcurpany;//высота курсора над панелью (если <0, то под панелью),
fix fcurstepx,fcurstepz;//шаги перемещения курсора;
//эти параметры целые, в единицах шага перемещения курсора:
//диапазон перемещения курсора в плоскости панели
int curminx,curmaxx,curminz,curmaxz;//обычно max=размеру панели/шаг курсора;
int curpanx,curpanz;//координаты оригина курсора, в плоскости панели,
int curpandx,curpandz;//перемещение за последний кадр, --//--;
int curk0,curk1;//состояние кнопки k0/k1 курсора (как b0/b1 мыши),
//число нажатий/отпусканий кнопки k0/k1 за последний кадр
int curk0on,curk0off,curk1on,curk1off;

int autobestpoz;//флаг автовозврата к Bestpoz выбранной панели
int bestpozmode;//MODEBMP/MODEPAN при autobestpoz=ON

//набор цветов gray[RADMIN...RADMAX],gray1[MAX...MIN],red[MIN...MAX],
int raduga[14][RADLEN];//red1[MAX...MIN],... всего 14 полос радуги !

//это коды цветов до инсталляции !
//массив 16 градаций точного серого
int gray[16]= {0x000,0x111,0x222,0x333,0x444,0x555,0x666,0x777,
               0x888,0x999,0xaaa,0xbbb,0xccc,0xddd,0xeee,0xfff};
//наименее отличные цвета от цветов предыдущего массива
int gray1[16]={0x001,0x110,0x221,0x332,0x443,0x554,0x665,0x776,
               0x887,0x998,0xaa9,0xbba,0xccb,0xddc,0xeed,0xffe};
//градации ~наиболее красного
int red[16]=  {0x000,0x100,0x211,0x322,0x422,0x533,0x644,0x755,
               0x866,0x977,0xa88,0xb99,0xcbb,0xdcc,0xedd,0xfee};
//наименее отличные цвета от цветов предыдущего массива
int red1[16]= {0x001,0x101,0x212,0x323,0x423,0x534,0x645,0x756,
               0x867,0x978,0xa89,0xb9a,0xcbc,0xdcd,0xede,0xfef};
int redgrn[16]= {0x000,0x110,0x221,0x332,0x443,0x554,0x664,0x775,
                 0x886,0x997,0xaa8,0xbb8,0xcca,0xddb,0xeed,0xffe};
int redgrn1[16]={0x001,0x111,0x222,0x333,0x444,0x555,0x665,0x776,
                 0x887,0x998,0xaa9,0xbb9,0xccb,0xddc,0xeee,0xfff};
int grn[16]=   {0x000,0x010,0x121,0x232,0x242,0x353,0x464,0x475,
                0x586,0x797,0x8a8,0x9b9,0xacb,0xbdc,0xded,0xefe};
int grn1[16]=  {0x001,0x011,0x122,0x233,0x243,0x354,0x465,0x575,
                0x686,0x798,0x8a9,0x9ba,0xbcb,0xcdc,0xdee,0xeff};
int grnblu[16]= {0x000,0x011,0x122,0x233,0x344,0x455,0x466,0x577,
                 0x688,0x799,0x8aa,0x8bb,0xacc,0xbdd,0xdee,0xeff};
int grnblu1[16]={0x100,0x111,0x222,0x333,0x444,0x555,0x566,0x677,
                 0x788,0x899,0x9aa,0x9bb,0xbcc,0xcdd,0xeee,0xfff};
int blu[16]=   {0x000,0x001,0x112,0x223,0x224,0x335,0x446,0x557,
                0x668,0x779,0x88a,0x99b,0xbbc,0xccd,0xdde,0xeef};
int blu1[16]=  {0x100,0x101,0x212,0x323,0x324,0x435,0x546,0x657,
                0x768,0x879,0x98a,0xa9b,0xcbc,0xdcd,0xede,0xfef};
int redblu[16]= {0x000,0x101,0x212,0x323,0x434,0x545,0x646,0x757,
                 0x868,0x979,0xa8a,0xb8b,0xcac,0xdbd,0xede,0xfef};
int redblu1[16]={0x010,0x111,0x222,0x333,0x444,0x555,0x656,0x767,
                 0x878,0x989,0xa9a,0xb9b,0xcbc,0xdcd,0xeee,0xfff};

int logovisible=YES;//видимость лейбла

int locksyspan2user=ON;//флаг прицепления системных панелей к юзеру:
//это панель мини-хелпа, запроса на выход, печати координат...

//состояние общефункциональных псевдо клавиш, это не kb-коды:
int Shift,Control,Alter;//не KEYON/KEYOFF а ON=1/OFF=0 !
int CapsLock=ON;//это флаг скорости, он триггеруется !

//текущие макс номера панели, битмапа, палитры и блока памяти
int maxpannum,maxmemnum;//но не ниже нуля!
//текущее количество соотв-х свободных ресурсов системы
int freepans=MAXPAN,freemems=MAXMEM;
int errpan,errmem;//номера ошибок >0 при операциях с panel, memory;

//можно near/far
byte far cursor[CURLEN]={//массив для загрузки текстуры курсора
//(как временная память при старте): яркость литер от 0 до F !
"               2"
"              25"
"       2     2E4"
"      273456CED3"
"     279BDFFEDC3"
"    39BDFFFEDA53"
"   3BDFFEDCB2   "
"  4DFFEDCB982   "
" 4FFEDCBA98762  "
"246665554443332 "
};

//можно near/far
byte far lila[LILALEN]=//битмап лейбла Lila
{
"         3F3F3      3F3        3F3F3         "
"        3FF3F3     3FF3       3FF3FF3        "
"       3FBF3F3    3FBF3      3FBF3FBF3       "
"      3FBAF3F3   3FBAF3     3FBAF3FABF3      "
"     3FBA9F3F3  3FBA9F3    3FBA9F3F9ABF3     "
"    3FBA98F3F3 3FBA98F3   3FBA98F3FFFFFF3    "
"   3FBA987F3F33FBA987F3  3FBA987F3F789ABF3   "
"  3FBA9876F3F3FBA9876F3 3FBA9876F3F6789ABF3  "
" 3FBA98765F3FFBA98765F33FBA98765F3F56789ABF3 "
"3FBA987654F3FBA987654F3FBA987654F3F456789ABF3"
};

int printwarn=ON;//флаг печати предупреждений при нефатальных ошибках




int VRS_argc=0;
char **VRS_argv=NULL;

int main(int argc,char *argv[])
{
int n;//адресный номер AP
dword dt;

VRS_argc=argc;
VRS_argv=argv;
WinInit(argc,argv,"Virtual Reality Shell LILA 0.57 (Windows " WINPORT_VER ")");
WinSetAbout("Virtual Reality Shell LILA, ver. 0.57\nCopyright Vadim Madgazin (C) 1995,1996");
CopyRight();
startscrnmode=actscrnmode=GetScreenMode();
MouseReset();//проверяется наличие мыши !
if (mousepresent!=YES) printf("Mouse not found !\n");
else  MouseSense();//стандартизация чувствительности!
//переключение обработчиков прерываний и установка частоты irq таймера
NewInterruptHandlers();

RAMAlloc();//выделение far RAM для блоков памяти
/*
printf("Near RAM size=%li,  stackavail=%u\n",(dword)coreleft(),stackavail());
printf("Far  RAM size=%li\n",ramfullfree);
printf("Far  RAM segment=%u  offset=%u\n",FP_SEG(ramstart),FP_OFF(ramstart));
printf("_stklen=%u  _heaplen=%u\n",_stklen,_heaplen);
WKP();//@ здесь переполнение стека, если int 9 обрабатывается неправильно !
*/

SoundSetup();//сетап звука
TablesSetup();//загрузка табличных данных с диска и/или расчет
ColorArraySetup();//рассчет массивов цветов
//начальные установки в различных структурах данных системы
AllMEMdataSetup();
AllPANdataSetup();
AllAPdataSetup();
SetAllAPAddress();//установка внутренних адресов всех AP
SetMSGBufferToEmpty();//стирание msg-буфера
PENSetup();//сетап PEN
LilaPaintSetup();//подготовка лейбла
Setup000();//сетап нулевых ресурсов, а также курсора и карандаша!

//Lila считает или загружает битмапы и строит системный мир
VRSWorldSetup();

//для старта обязательных AP
AddShortMSG(ADDR_VRS,ADDR_APFirst,MSG_AP_CSTART);
AddShortMSG(ADDR_VRS,ADDR_APRend,MSG_AP_CSTART);
AddShortMSG(ADDR_VRS,ADDR_APOVGAg,MSG_AP_CSTART);
//сброс слепых перемещений мыши
if (mousepresent==YES) MouseStatus();

//начальная позиция наблюдателя и PEN
USERandPENstartpoz();
while (1)//основной цикл или виток системы
  {
  //системные функции в начале витка

  //ПОРТ: обработка сообщений окна, ввода и завершения
  WinPump();
  if (WinQuitRequested()) EndProgram(0);

  //перекачка событий клавиатуры из очереди бэкенда в kbbuf
  FillKbbufFromWin();

  {
  //ПОРТ: время от монотонных часов; тик звука - в аудио-потоке (не зависит от FPS)
  dword nowt=WinTimeMs();
  vrs_time=nowt;
  vrstime0=(word)nowt;
  vrstime1=(word)(nowt>>16);

  //измеряем время прорисовки предыдущего кадра
  disable();
  dt=vrs_time-oldtime;
  cadrtime= (oldtime==0)? 0:(int)dt;
  oldtime=vrs_time;
  enable();
  }

  //просмотр буфера клавиатуры нижнего уровня и добавление в msg-буфер
    //сообщений о вводе данных
  KeyboardBufferScan();

  //системный фильтр сообщений и отработчик
  SystemMessageFiltr();//выход из программы сидит здесь!

  //все AP (прикладные задачи) по порядку возрастания адресов
  for (n=1; n<=numap; ++n)
    if (AP[n].taskflag==ON)//если задача в витке
      AP[n].taskadress();

  //ПОРТ: вывод кадра 'VGA' в окно
  if (actscrnmode==0x13) VideoShow();

  //системные функции в конце витка
  LastVRStask();

  MessageDeleter();//системный поглотитель сообщений

  //ПОРТ: легкое ограничение частоты витков (бережем CPU)
  WinFrameEnd();
  }
}

void CopyRight(void)
//визуализация авторских прав
{
printf("\n\t\t\t_ _\n");
printf("Virtual Reality System LILA, ver. %s\n",VERS);
printf("Copyright Vadim Madgazin (C) 1995,1996\n\n");
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
      Sound(SNDAPRIN); break;

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

          case KeyCod0: case KeyCod3: case KeyCod6:
          case KeyCod1: case KeyCod4: case KeyCod7:
          case KeyCod2: case KeyCod5: case KeyCod8:
          case KeyCod9://номер варианта земли, неба и дали
            var=kbcod-KeyCod0;
            if (Alter==ON)        Dalivar(var);//вариант дали
            else if (Control==ON) nebocoli=var;//вариант цвета неба
            else   { bgvar=var; newbgpar=YES; }//вариант цветов земли bgpar
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
      //пропускаем панель пера чтобы не биться об неё при клиппинге!
      if ((clip==YES)&&(pan==pnclpan)) continue;
      RenderPanel(pan);
      }
    //рендер всех стеклянных панелей
    RenderSteklPanels();
    if (clip==YES)//будет клиппинг
      {
      //определяем параметры клиппинга, без пера-карандаша !
      ClipComput();
      pan=pnclpan;//теперь рендер панели пера-карандаша
      if ((panel[pan].flag==YES)&&
          (panel[pan].render==YES)) RenderPanel(pan);
      }

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

