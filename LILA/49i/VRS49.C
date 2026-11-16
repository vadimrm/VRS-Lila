char VERSION[10]="0.49";
/*                                    _ _
                  Имя ...... Система ЛИЛА (ЛИИЛАА)
                 Дата ...... 08-10,12.02.96

                      Файлы: */
#include "kbcod.h"
#include "vrs49.h"
/*     main и функции ...... vrs49.c и vrs49a.c, vrs49c.c,
                             vrs49r.c, vrs49s.c + *.obj
      для инсталляции ...... vrs49i.c + *.exe
       для выполнения ...... sinus.lii, keyboard.lii, font.lii,
                             syspal.lii, gradcol.lii
         Документация ...... vrs49.doc, help.txt

  Новое:
  -изменение и введение многих хоткеев!
  -усовершенствование QuickSort() !!!
  -цвета земли теперь плавно затеняются !!!
  -часть кода инсталляции перенесена в саму Lila !
  -флуктуации пикселей по x и/или по z !
  -вместо градаций от угла периодические градации от расстояния
   до пикселя панели и/или от Xp1 пикселя панели!
  -вариант дали из двумерного битмапа !!!
  -вращение дали по углу - для облаков !
  -усовершенствовано выделение и загрузка битмапов !

  Задачи:
  -массив gradcol считается быстро, его можно не хранить на диске!
  -pp файлы с цветностью внутри !
  -имя файла к варианту дали ! облака/горы с диска !
  -AP_Render: переадресация совсем неиспольз-х хоткеев в VRS!
  -хоткеи AP1:
   -юзера->в bestpoz выбранной панели, в модах PAN/BMP по очереди!

  Важно:
  -7700 байт (минус 4096 стек) near RAM свободно!
*/

#include <dos.h>
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
#include <malloc.h>


/* @
#define PANLIST (MAXPAN+2)//max число панелей в panbox[]=(PANLIST-2)
//список номеров выделенных панелей для неких общих целей, напр. для
//удаления (со всеми ресурсами) системой по каждому хоткею Del-off...
//panlist[0]=размер массива panlist[],
//panlist[1]=текущее количество панелей в списке,
//panlist[2...(PANLIST-1)]=номера панелей, список растет вверх!
int panlist[PANLIST]={PANLIST,0};
*/

//============это в перспективе в h-файл

int LoadPPfile(int bm,char *fname,fix colness);

int Labirint(int owner,d3poz d3shift,int *bmaps,int *convs,int ind0, int ind1,
             int panx,int panz,int stepx,int stepy,byte *lab,int labx,int laby);

//============это непосредственно в h-файл:


//============это 'новьё' уже есть в h-файле:

int nebocol=100;//абс. номер цвета неба;

byte zbitm[16];//псевдо-битмап земли
int dalibitm=ISNOT;//номер битмапа дали;

//параметры раскраски цветов 1...8:
int newbgpar=YES;//флаг новых параметров bgpar
int bgvar=1;//текущий номер варианта цветов 1...8
//цвет цвета 1   множитель шага   шаг цвета    яркость цвета
//r,g,b=0...63,  dr,dg,db=0...1,  step=0...8,  power=0...64
//[][0,1,2]      [][3,4,5]        [][6]        [][7]
int bgpar[BGVAR][BGPAR]={//параметры по вариантам
{56,63,32, 1,0,0, 1, 32},
{63,63,63, 1,0,0, 1, 32},
{16,16, 0, 1,1,1, 6, 30},//подобрать !!! @
{63,63,63, 1,1,1, 1, 32},
{63,63,63, 1,1,1, 1, 32},
{63,63,63, 1,1,1, 1, 32},
{63,63,63, 1,1,1, 1, 32},
{63,63,63, 1,1,1, 1, 32},
{63,63,63, 1,1,1, 1, 32},
{63,63,63, 1,1,1, 1, 32}};

//параметры дали:
int dalinali=YES;//флаг рендера дали
int dalipaint=NO;//битмап дали [пере]красить надо ли ?
int dali2d=YES;//двумерность битмапа дали
int newdali=YES;//флаг новых параметров дали
int dalivar=1;//номер варианта дали
int dnumpar=4;//номер параметра для регулировки
int dalipar[DALIVAR][DALIPAR]={
//параметры дали по вариантам
//это для дали из 2D-битмапа с lenz=200 и прозрачным цветом !
{  0,  0,  0,  0,   20,256,  0,     0,  0,  0,     0,  0,  200,  0,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//  геометрия
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0},//  цветометрия
//следующий вариант
{ -1,  1,  0, 16,    0,256,-32,     0,  0, 32,     0,  0,    5,  0,
   0, 40,   -10, 30,   -10, 30,   -10, 20,   -10, 10,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,    -1,  1,//  геометрия
   0, 40,   -10, 30,   -10, 30,   -10, 20,   -10, 10,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,    -3,  4},//  цветометрия
//следующий вариант
{ -1,  0,  0,256,  -60,256,  0,     0,  0,  0,     0,  0,  260,  0,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//  геометрия
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,//фракто->
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0},//  цветометрия
//следующий вариант
//...
};
//номер прозрачного цвета для дали (0...255, нет его при <0):
int dtranscol;//глобальная копия dalipar[][0] !!!
//отклонение нулевого уровня дали от уровня горизонта:
int dalidjear;//глобальная копия dalipar[][4] !!!

int dalidadt;//скорость вращения дали по углу - для облаков!

//строка периметра дали с параметрами столбцов
//в трёх массивах 1+1+2=4 кб, можно near @
byte  far dalistrl[STRLEN];//длина столбца дали по вертикали, 0...255
sbyte far dalistrh[STRLEN];//вертикальный уровень начала столбца, -128...+127
int   far dalistrs[STRLEN+1];//смещение начала столбца в битмапе дали,
//0...65535 (word !); началом столбца дали считается его нижний пиксель!

int spaceflag;//флаг космоса: OFF=земля и небо, ON=везде небо!
int starvar;//номер варианта, ему ~пропорционально количество звёзд!

int jearuser;//строка близкой к юзеру земли на экране
//уровень горизонта проходит по Z=0 USER-системы, если считать
//номер строки низа PEN-bmp =0, то горизонт будет на строке:
int jear;//и может быть вне экрана!

int horshift,horshift360=1;//см AP_Render

int stepvalue;//флаг-шаг=(1+7*CapsLock)*(8+56*Shift)/(1+7*Alter)=1...512;

fix DYuser;//очередное линейное смещение юзера для плаванья по цветам земли;

//параметры изменения градаций (яркостей) цветов панели:
int gradcon;//общая постоянная компонента;
int gradvarflag;//=YES;//флаг изменений градаций внутри панели:
int gradfx=64,gradfy=32,gradamp=64;//регуляторы частот по x/y и амплитуды;

int randfluct=0;//при panel[].rand>=0: плюс к флуктуациям пикселей;
int fluctflag=3;//0-й бит: флаг флуктуаций по x, 1-й бит - по z !

int neprozra;//добавка непрозрачности к стеклу всех панелей;

int SPmin=MAXINT,SPmax=-MAXINT;//max/min (int)_SP !

//============это 'старьё':

//extern word _stklen=5000; //обычно 4 кб
//extern word _heaplen;//обычно 0

void (interrupt far *OldTimerInterrupt)(void);
void (interrupt far *OldKeyboardInterrupt)(void);
void (interrupt far *Old0Interrupt)(void);
void (interrupt far *Mouse)(void);

long Seed,Seed2,Seed32;
word Seed16;

int kbbufvalid=NO;//флаг разрешения буферизации кодов клавиш
volatile int anycodflag=0;//флаг для проверки был ли ввод любой из клавиш;
byte far scod2kbcod[256];//таблица перекодировки scod->kbcod;
KeyboarD far kbbuf[KBBUF];//кольцевой буфер кодов клавиатуры
volatile int kbwrite=0;//позиция для новой записи
int kbread=0;//позиция для нового чтения этого буфера, т.о.
  //читать надо блок от kbread до kbwrite-1 включительно!

MessagE far msg[MSGBUF];//кольцевой буфер сообщений VRS и AP
int msgwrite=0;//позиция для новой записи
int msgread=0;//позиция для нового чтения этого буфера, т.о.
  //читать надо блок от msgread до msgwrite-1 включительно!

APdatA far AP[MAXAP];//массив структур данных AP, индекс=адрес AP!
int numap=ADDR_LAST_AP;//адреса AP в текущей версии системы=1...numap;

PANdatA far panel[MAXPAN];//массив панелей
MEMdatA far memory[MAXMEM];//массив заголовков блоков памяти
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

byte near gradcol[MAXGRADCOL][256];//таблица градаций [яркости=0...63]

int mousepresent=NO;//флаг наличия мыши
int mousedx,mousedy;
int mouseb0,mouseb1;//клавиша b0/b1 мыши нажата=1, ненажата=0
//числа нажатий/отжатий b0/b1 за последний кадр
int mouseb0on,mouseb0off,mouseb1on,mouseb1off;

//параметры указки в любой моде:
int pointermode=0;//мода указки(pointer): 0=карандаш(pencil), 1=курсор(cursor),
int pnclpan,curpan;//номер панели указки-карандаша/указки-курсора
int pnclbm,curbm;//соответственно номера их битмапов;

//параметры указки в моде0=карандаш
int pnclmousemode=0;//0=мышь перемещает юзера, 1=карандаш
int pnclvisible=NO;//видимость карандаша
int pnclpozflag=1;//позиция его 0,1,2,3;
int pnclx,pnclz;//экранные x/z _пикселя острия_ _битмапа_ карандаша!
int pncldz;//смещение пикселя острия от оригина этого битмапа по z !
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

//можно far @
byte near cursor[CURLEN]={//массив для загрузки текстуры курсора
//(как временная память при старте): яркость литер от 0 до f !
"               2"
"              25"
"       2     2e4"
"      273456ced3"
"     279bdffedc3"
"    39bdfffeda53"
"   3bdffedcb2   "
"  4dffedcb982   "
" 4ffedcba98762  "
"246665554443332 "};

//можно near/far @
byte near lila[LILALEN]=//битмап лейбла Lila, яркость-=3 !
{
"         6i6i6      6i6        6i6i6         "
"        6ii6i6     6ii6       6ii6ii6        "
"       6iei6i6    6iei6      6iei6iei6       "
"      6iedi6i6   6iedi6     6iedi6idei6      "
"     6iedci6i6  6iedci6    6iedci6icdei6     "
"    6iedcbi6i6 6iedcbi6   6iedcbi6iiiiii6    "
"   6iedcbai6i66iedcbai6  6iedcbai6iabcdei6   "
"  6iedcba9i6i6iedcba9i6 6iedcba9i6i9abcdei6  "
" 6iedcba98i6iiedcba98i66iedcba98i6i89abcdei6 "
"6iedcba987i6iedcba987i6iedcba987i6i789abcdei6"
};

int printwarn=ON;//флаг печати предупреждений при нефатальных ошибках

//******* ниже локально-глобальные переменные и определения *******

#define MAXBMCON  16
int bmaps[MAXBMCON],convs[MAXBMCON];//стандартные битмапы и конверты

#define PPNUM       2
#define PPNAMELEN  20
char ppfile[PPNUM][PPNAMELEN]={//файлы битмапов
"st1.pp",//стенки
"st2.pp",
};

/* одна панель кодируется числом ABC, (если C=0 то используется 1/2)
                 B : 0 1 2 3 4 5 6 7 8 9 a b c d...
   угол панели=B%8 : 0 1 2 3 4 5 6 7 0 1 2 3 4 5...
   высота её=1+B/8 : 1 1 1 1 1 1 1 1 2 2 2 2 2 2...

12345678 12345678 12345678
|||||||| |||||||| ||||||||
3bjrzHPX 2aiqyGOW 19hpxFNV  следующие литеры преобразуются в числа:
       \    |    /          0123456789abc...xyzABC...XYZ ->  0...61
4cksAIQY<-- + --> 08gowEMU  значения вместо пробела (по умолчанию):
       /    |    \          A  0  /Bнет панели  /C 1
5dltBJRZ 6emuCKS_ 7fnvDLT_  битмап/угол и высота/длина */
byte labirint[768]={     //макс размер [16][16][3]
"06d                                          14f"
"                                                "
"                                                "
"                                                "
"                                                "
"                     1t11k1                     "
"                  1m1      1r1                  "
"                  1v1      1i1                  "
"                     1g11p1                     "
"                                                "
"                                                "
"                                                "
"                                                "
"106         070050         106070050         02d"
"                                                "
"                                                "
};




void main( int argc, char *(argv[]) )
{
int n;//адресный номер AP
dword dt;

CopyRight();
startscrnmode=actscrnmode=GetScreenMode();
Mouse=getvect(0x33);//адрес драйвера мыши
MouseReset();//проверяется наличие мыши !
if (mousepresent!=YES) printf("Mouse not found !\n");
else  MouseSense();//стандартизация чувствительности!
//переключение обработчиков прерываний и установка частоты irq таймера
NewInterruptHandlers();

RAMAlloc();//выделение far RAM для блоков памяти
//@
printf("Near RAM size=%li,  stackavail=%u\n",(dword)coreleft(),stackavail());
/*
printf("Far  RAM size=%li\n",ramfullfree);
printf("Far  RAM segment=%u  offset=%u\n",FP_SEG(ramstart),FP_OFF(ramstart));
printf("_stklen=%u  _heaplen=%u  press key!\n",_stklen,_heaplen);
WKP //@ здесь переполнение стека, если int 9 обрабатывается неправильно !
*/

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
VRSWorldSetup(argc,argv);

//для старта обязательных AP
AddShortMSG(ADDR_VRS,ADDR_APRend,MSG_AP_CSTART);
AddShortMSG(ADDR_VRS,ADDR_APOVGAg,MSG_AP_CSTART);
//сброс слепых перемещений мыши
if (mousepresent==YES) MouseStatus();

while (1)//основной цикл или виток системы
  {
  //системные функции в начале витка

  disable();//измеряем время прорисовки предыдущего кадра
  dt=vrs_time-oldtime;
  cadrtime= (oldtime==0)? 0:(int)dt;
  oldtime=vrs_time;
  enable();

  //просмотр буфера клавиатуры нижнего уровня и добавление в msg-буфер
    //сообщений о вводе данных
  KeyboardBufferScan();

  //системный фильтр сообщений
  SystemMessageFiltr();//выход из программы сидит здесь!

  //все AP (прикладные задачи) по порядку возрастания адресов
  for (n=1; n<=numap; ++n)
    if (AP[n].taskflag==ON)//если задача в витке
      AP[n].taskadress();

  //системные функции в конце витка

  MessageDeleter();//системный поглотитель сообщений

  //нет реакции на клавиатуру до момента конца первого витка!
  kbbufvalid=YES;
  }
}

void CopyRight(void)
//визуализация авторских прав
{
printf("\n\t\t\t_ _\n");
printf("Virtual Reality System LILA, ver. %s\n",VERSION);
printf("Copyright Vadim Madgazin (C) 1995,1996\n\n");
}

void AllAPdataSetup(void)
{
int n;//адресный номер AP

for (n=0; n<=numap; ++n)//0=адрес VRS (надо для преобр-й координат) !
  {
  AP[n].state=AP_START;
  AP[n].taskflag=ON;
  AP[n].taskadress=AP_Null;//пустая заглушка на случай пропуска адреса AP
  AP[n].ear=d3null;//ставим AP в [разные] места пространства
  AP[n].earshift=YES;
  }

AP[ADDR_APFirst].taskadress=AP_First;
AP[ADDR_APRend].taskadress=AP_Render;
AP[ADDR_APOVGAg].taskadress=AP_OutVGAgra;
}

int Labirint(int owner,d3poz d3shift,int *bmaps,int *convs,int ind0, int ind1,
             int panx,int panz,int stepx,int stepy,byte *lab,int labx,int laby)
//лабиринт, структура которого определена матрицей lab[laby][labx][3]
//троек литер строки текста, lab[0][0][]=левый верхний угол строки !
//лабиринт из панелей, номера их битмапов и конвертов (и те и другие
//могут быть ISNOT) берутся из массивов bmaps[] и convs[] соответ-но
//с разверткой индексов этих массивов от ind0 до ind1 включительно;
//тройке соседних литер соответствует шаг клетки поля stepx/stepy см;
//размер панелей panx/panz см;
//оригин (левый нижний угол) лабиринта сдвинут от овнера на d3shift!
//возвращает ERR, если не хватило панелей для всего лабиринта!
{
int pan,dint[4],a,b,c,x,y,bm,blx1,blz1,ind,iang,ilen,imulz;
fix flenz,flenx,flenx1;
byte *lab3;

flenz=itofix(panz);//высота панелей при imulz=1;
flenx=itofix(panx);//длина панелей  при ilen=1 для углов 0,90,180,270 гр
flenx1=fixmul(flenx,iifixdiv(14142,10000));//--//--  для 45,135...

//яркость для 4-х ориентаций панелей (%4) @@@
dint[0]=0;//  0, 4, 8 ...
dint[1]=-10;//1, 5...
dint[2]=-20;//2, 6
dint[3]=-10;//3, 7

//все панели лабиринта
for (y=0; y<laby; ++y)
  for (x=0; x<labx; ++x)
    {
    lab3=lab+3*((laby-1-y)*labx+x);
    a=lab3[0];//тройки литер abc дают параметры панели
    b=lab3[1];
    c=lab3[2];
    if (b==32) continue;//b=пробел, нет панели !
    //else есть панель
    b=let2i(b);
    iang=b%8;//номер ориентации панели по углу 0...7
    imulz=1+b/8;//множитель для высоты панели;
    ind=ind0+let2i(a);//a=пробел: умолчание для индекса битмапа ind=ind0
    if (ind>ind1) ind=ind1;
    if (c==32) ilen=1;//c=пробел: умолчание для длины ilen
    else ilen=let2i(c);//в том числе 0 !
    bm=bmaps[ind];
    blx1=memory[bm].lenx-1;//размеры с запасом 1 пиксель
    blz1=memory[bm].lenz-1;//для неперехлеста текстуры !
    pan=GetPAN(owner);
    if (pan<=ISNOT) return ERR;
    SetPanelRes(pan,bm,convs[ind]);
    LockPAN(pan);//т.о. ресурсы будут многократно залочиваться!
//    panel[pan].consti=dint[iang&3];//добавка яркости @@@
    //размеры панели
    panel[pan].flenz=imulz*flenz;
    //len x зависит от угла!
    panel[pan].flenx= ((1&iang)==0)? flenx:flenx1;
    if (ilen>0) panel[pan].flenx*=ilen;
    else//0-я длина будет половинной !
      panel[pan].flenx/=2;
    if (blx1>0) panel[pan].fmulx=flenx/blx1;//по x повторение, а
    if (blz1>0) panel[pan].fmulz=panel[pan].flenz/blz1;//по z растяжение!
    //положение панели внутри лабиринта
    panel[pan].ap=XYZgrad2D3poz(x*stepx,y*stepy,0,iang*45);
    //сдвиг панели с учетом сдвига лабиринта
    panel[pan].ap=ConvD3pozFromAP2Earth(panel[pan].ap,d3shift);
//    panel[pan].neproz=10;//@ для стеклянной архитектуры !
    }
return OK;
}

int let2i(int let)
//литера 0123456789abc...xyzABC...XYZ преобразуется в целое 0...61
{
int val;
val=0;
if (('0'<=let)&&(let<='9')) val=let-'0';
if (('a'<=let)&&(let<='z')) val=let-'a'+10;
if (('A'<=let)&&(let<='Z')) val=let-'A'+36;//10 цифр плюс 26 малых букв
return val;
}

void AP_First(void)
{
static int addr_AP=ADDR_ALL;
int msgrd,msgwr,kbcod,hit;
static int pan0=ISNOT;

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
      addr_AP=GetIntMSG(msgrd); break;

    case MSG_AP_CSTART://начать работу AP 'с нуля'
      AP[addr_AP].state=AP_WORK;
      //ставим систему AP в нужную позицию
      AP[addr_AP].ear=XYZgrad2D3poz(0,3500,0,0);
      AP[addr_AP].earshift=YES;
      if (pan0>ISNOT) break;//уже есть эта панель
      //делаем панель 0: текст-табличка с древком из оригна AP
      pan0=GetPANtxt(addr_AP,5,9,1,320,0);//ширина 320 см!
      panel[pan0].neproz=20;
      panel[pan0].rand=0;//будут флуктуации !
      panfill(pan0,0,0,0,0,0);
      pantext(pan0,5,blu[12],0,0,"Первая ПП");
      //надо функцию установки _центра_ панели в нужную точку ##
      panel[pan0].ap=fixXYZgrad2D3poz(
        -panel[pan0].flenx/2,-itofix(5),USERZ0+itofix(10),0);
      //сделать стандартную ф-ю древка ! (с табличкой?) ##
      {//древко
      int pan;
      pan=GetPAN(addr_AP);
      SetPanelRes(pan,bmaps[0],convs[0]);
      LockPAN(pan);
      //надо сервис по установке параметров панели ! ##
      panel[pan].consti=-40;
      panel[pan].flenx=itofix(30);
      panel[pan].flenz=USERZ0+panel[pan0].flenz+itofix(20);
      panel[pan].ap=fixXYZgrad2D3poz(-panel[pan].flenx/2,0,0,0);
      }
      //сама ПП пока сразу же отменяет выбор себя:
      DeChoiceAP();//##
      break;

    case MSG_AP_INPUT_OFF: break;

    case MSG_AP_INPUT_ON://выбор [панели] AP
      if (curnumpan>ISNOT)//выбор панели
        {
        CursorOnPanel(curnumpan);
        Bestpoz(USER2PAN,MODEPAN,curnumpan);
        //можно также установить автовозврат к панели:
        autobestpoz=ON; bestpozmode=MODEPAN;
        }
      //else выбор AP (без панели): см хоткеи F1...F8 !
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
    case MSG_AP_STOP://приостановить работу AP
      //если состояние AP не было рабочее, то пропуск сообщения
      if (AP[addr_AP].state!=AP_WORK) break;
      AP[addr_AP].state=AP_STOP;
      break;
    case MSG_KBPC_DATA:
      if (AP[addr_AP].state!=AP_WORK) break;
      hit=msg[msgrd].data[0];
      kbcod=msg[msgrd].data[1];
      if (hit==KEYON)
        switch(kbcod)
          {
          default: break;

          //перемещение курсора серыми стрелками
          case KeyCodRight:
          case KeyCodLeft: curpanx+= (kbcod==KeyCodRight)? 4:-4; break;
          case KeyCodUp:
          case KeyCodDown: curpanz+= (kbcod==KeyCodUp)? 4:-4; break;

          }
      else//KEYOFF
        switch(kbcod)
          {
          default: break;

          //функциональные хоткеи AP First
          case KeyCodF1:  case KeyCodF2:  case KeyCodF3:
          case KeyCodF4:  case KeyCodF5:  case KeyCodF6:
          case KeyCodF7:  case KeyCodF8:
            break;

            //переключаем флаг автовозврата к выбранной панели
          case KeyCodScrlLck: autobestpoz^=1;
            //если автовозврат и выбранная панель существует
            if ((autobestpoz==ON)&&(curnumpan>ISNOT))
              {
              //юзер -> в наиболее удобную позицию наблюдения панели
              Bestpoz(USER2PAN,bestpozmode,curnumpan);
              curmousemode=1;//мышь на курсоре!
              }
            break;

            //перемещение мышью юзера или курсора на панели
          case KeyCodBS: curmousemode^=1; break;

          case KeyCodHome: curpanx=curminx; curpanz=curminz; break;
          case KeyCodEnd:  curpanx=curmaxx; curpanz=curmaxz; break;

          }
      break;
    }

nextmsg:
  if (msg[msgrd].srcaddr==addr_AP) { if (msg[msgrd].count>0) msg[msgrd].cmd=0; }
  else if (msg[msgrd].dstaddr==addr_AP) msg[msgrd].cmd=0;
  if (++msgrd>=MSGBUF) msgrd=0;
  }

if (AP[addr_AP].state==AP_WORK)//если состояние AP 'рабочее'
  {
  if (pan0>ISNOT) panel[pan0].flag=YES;//видимость панели оригина AP !
  }
}

void VRSWorldSetup(int argnum,char *(argv[]))
//система загружает битмапы и наполняет мир несколькими панелями...
{
d3poz d3;
int n;

//заменяем pp-файлы на имена из строки вызова
for (n=1; n<argnum; ++n)//pp-файлы начинаются с аргумента [1] !
  {
  //  (n-1) это номер pp-файла!
  if ((n-1)>=PPNUM) break;
  strncpy(ppfile[n-1],argv[n],PPNAMELEN-1);
  }

//загружаем pp-файлы
for (n=0; n<PPNUM; ++n)
  {
  bmaps[n]=GetLockMEM2(ADDR_ALL,320,200,FIX6D5);
  LoadPPfile(bmaps[n],ppfile[n],2*FIX1);
//  convs[n]=GetLockMEMconv(ADDR_ALL);//по одному конверту на битмап!
//  SetConvert1to1(convs[n]);//сначала цвета без преобразования!
  }

dalibitm=bmaps[0];//@@@

//выделение панелей и расстановка их в пространстве

//лабиринт !!!

//ставим систему AP_First в нужную позицию
AP[ADDR_APFirst].ear=XYZgrad2D3poz(0,3500,0,0);
AP[ADDR_APFirst].earshift=YES;

d3=XYZgrad2D3poz(-2400,100,0,0);//сдвиг оригина лабиринта
Labirint(ADDR_APFirst,d3,bmaps,convs,0,1,320,240,320,320,labirint,16,16);

}

void SetConvert1to1(int conv)
//делает конверт с преобразованием цветов 1:1
{
int i;
if ((conv<=ISNOT)||(memory[conv].len<256)) return;//ошибка
for (i=255; i>=0; --i) memory[conv].rampntr[i]=i;//цвет i -> i !
//@ ~~'цветной негатив'
//for (i=9; i<256; ++i) memory[conv].rampntr[i]=255-i+9;
}

#define PPPALLEN (PAL768+10)
int LoadPPfile(int bm,char *fname,fix colness)
//из файла fname грузится текстура в блок памяти bm и преобразуется
//в syspal с цветностью colness и модой развертки 2, возврат OK/ERR
{
byte tmpram[PPPALLEN];
byte far *texram;
long len,lenx,lenz;
int transcol,x,z,lx,lz;
char num[10];

if (bm<=ISNOT) return ERR;

len=memory[bm].len;
lenx=memory[bm].lenx;
lenz=memory[bm].lenz;

if ((len!=(lenx*lenz))||(lenx<=0)||(lenz<=0)) return ERR;

//грузим текстуру с диска из pp файла
if (OpenRead(fname,0,memory[bm].rampntr,(word)len)!=OK) return ERR;

memory[bm].rammode=1;//входная  мода =1 текстуры pp-файла
ConvertMEMmode(bm,2);//выходная мода =2 для рендера

//грузим палитру и номер прозрачного цвета
OpenRead(fname,PPPALSHIFT,tmpram,PPPALLEN);
strncpy(num,(char*)tmpram+PAL768,10);
num[9]=0;
transcol=atoi(num);
//прозрачный цвет отмечаем в флаге
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
  for (z=lz-1; z>=0; --z)
    texram[z]=tmpram[texram[z]];
  }

return OK;
}

