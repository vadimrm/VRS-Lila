/*                             _ _
                      Система ЛИЛА
                 Дата ...... 30.06-21.07.96
*/
char VERS[8]="0.58";
#include "vrsport.h"
#include    "vrs58.h"
#include    "kbcod.h"
/*   Файлы:   проекта ... vrs58.c,vrs58ap1.c,vrs58c.c,vrs58r.c,
                          vrs58s.c,vrs54a.obj
       для выполнения ... sinus.lii, keyboard.lii, font.lii,
         опциональные ... [syspal.lii] ll.lii, *.pp, *.ll
         документации ... help.doc, lila.txt, vrs58.txt
      для инсталляции ... [vrs49i.exe]

  Новое:
  -удаление и изменение хоткеев!
  -вместо карандаша-пера теперь нож!
  -клиппинг делаем с ножом !
  -полностью выброшен курсор, выбор панели и т.п. !!!
  -curnumap переименовано в APforinput !!!
  -позиция ножа у PEN чуть правее средней вертикали клиппинга!
  -системный генератор синусоиды!
  -вибрации ножа по y-расстоянию!
  -выбор ближайшей панели-цели!
  -бросание ножа в цель!
  -разрыв цели на осколки, центробежные + случайные линейные
   скорости разлета осколков!
  -полет по параболам, падение всех осколков на землю!
  -хелп сначала заполнен битмапом, а текст только по F1!
  -хоткей 0: космос<->земля, сила тяжести тоже !!!
  -новый ll.lii файл и начальная архитектура vrs/ap1 !!!

  Надо:
  -звук на бластер!

  Позже:
  -перепросмотр глобальных переменных и кода:
   все хоткеи перевести в VRS или AP1, причем лучше всего чтобы
   там не было совпадений, тогда все хоткеи будут доступны в VRS
   одновременно, с учетом перенаправления части их в AP1 !
   для этого надо вместо плавных регуляторов и многоклавишных
   переключателей сделать один хоткей с перебором нескольких
   фикс-х значений нужных параметров!
  -стереопара?
  -исправить перехлест текстуры по вертикали - новые формулы ???
  -простой хоткей-генератор случайных лабиринтов!

  Важно:
  -4500 байт (минус 4096 стек) near RAM свободно!
*/

#include <sys/stat.h>
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

fix sysfsinus;//системный генератор синусоиды
int Fsys=5;//частота ГСК = 2^(Fsys-6) Гц

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

int soundflag=ON;//флаг разрешения звука

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

int nebocoli=3;//индекс цвета неба;
//nebocol[nebocoli]=абсолютный номер цвета неба:
int nebocol[9]={16,25,65,69,82,84,96,185,208};

byte zbitm[16];//псевдо-битмап земли
int dalibitm;//номер битмапа дали;

//параметры раскраски цветов 1...8:
int newbgpar=YES;//флаг новых параметров bgpar
int bgvar=0;//текущий номер варианта цветов
//цвет цвета 1   множитель шага   шаг цвета    яркость цвета
//r,g,b=0...63,  dr,dg,db=0...1,  step=0...8,  power=0...64
//[][0,1,2]      [][3,4,5]        [][6]        [][7]
int bgpar[BGVAR][BGPAR]={//параметры по вариантам
{58,30, 0, 1,1,0, 1, 24},//0
{ 8,63,63, 2,0,0, 1, 26},//1
{27,31,63, 0,0,1, 1, 38},//2
{57,51,33, 0,0,1, 1, 32},//3
{44,63, 0, 1,0,0, 1, 32},//4
{63,63,63, 1,0,0, 1, 32},//5
{30, 0,63, 0,0,1, 1, 55},//6
{ 8,63,63, 2,0,0, 2, 53},//7
{63,32, 0, 0,1,0, 1, 56},//8
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
{ -1,  1,-11, 32,    0,256,-16,     0,  0, 32,     1,  1,    0, 50,
   0, 40,   -10, 30,   -10, 30,   -10, 20,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,    -1,  1,
   0, 40,   -10, 30,   -10, 30,   -10, 20,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,    -3,  4},
//4: 1D-даль       горы
{ -1,  1,1428, 32,   0,256,-16,     0,  0, 32,     0,  0,   40, 50,
   0,  0,     0,  0,     0,  0,   -10, 10,   -10, 10,
 -10, 10,     0,  0,     0,  0,     0,  0,     0,  0,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,
 -10, 10,   -10, 10,   -10, 10,   -10, 10,   -10, 10},
//5: 1D-даль       горы
{ -1,  1, 592, 32,   0,256,-16,     0,  0, 32,     0,  0,   50, 50,
   0,  0,     0,  0,     0,  0,     0,  0,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,
   0,  0,     0,  0,     0,  0,     0,  0,   -10, 10,
   0,  0,     0,  0,   -50, 50,   -50, 50,     0,  0},
//6: 1D-даль       лес
{ -1,  1,1480,-108,    0,256,-64,   0,  0, 64,     0,  0,   10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,    -3,  2,
   0,  0,     0,  0,     0,  0,     0,  0,    -1,  1,
   0,  0,     0,  0,     0,  0,     0,  0,    -5,  5,
   0,  0,     0,  0,     0,  0,     0,  0,    -1,  1},
//7: 1D-даль       облака
{ -1,  1, -12,  13,   70,256,-64,   0,  0, 32,     0,  0,  200, 50,
   0,  0,     0,  0,     0,  0,     0,  0,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0,
   0,  0,     0,  0,     0,  0,     0,  0,   -10, 10,
   0,  0,     0,  0,     0,  0,     0,  0,     0,  0},

//8: 1D-даль для тестирования цветов
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

//нож (старое название pencil):
int pnclpan;//номер панели ножа;
int pnclvisualmode=1;//вид ножа: 0=невидимый, 1=видимый без, 2=со стеклом;
int APforinput=ADDR_VRS;//адрес обладателя текущего ввода с клавиатуры!

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

VRS_argc=argc;
VRS_argv=argv;
WinInit(argc,argv,"Virtual Reality System LILA 0.58 (Windows " WINPORT_VER ")");
WinSetAbout("Virtual Reality System LILA, ver. 0.58\nCopyright Vadim Madgazin (C) 1995,1996");
CopyRight();
startscrnmode=actscrnmode=GetScreenMode();
MouseReset();//проверяется наличие мыши !
if (mousepresent!=YES) printf("Mouse not found !\n");
else  MouseSense();//стандартизация чувствительности!
//порт: вместо перехвата irq - сброс системного времени
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
Setup000();//сетап нулевых ресурсов, а также курсора и ножа!

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
  //ПОРТ: обработка сообщений окна и ввода
  WinPump();
  if (WinQuitRequested()) EndProgram(0);

  //перекачка событий клавиатуры из очереди бэкенда в kbbuf
  FillKbbufFromWin();

  //системные функции в начале витка
  FistVRStask();

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

void Setup000(void)
//сетап нулевых ресурсов panel[0], memory[0], memory[1]
//с их вечным закреплением!
//подготовка курсора и ножа только _после_ этого!
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

//выделяем спецблок mem=VGAMEM=1
mem=GetLockMEM(ADDR_VRS,4);
//меняем параметры блока VGAMEM для вывода в видео-RAM
memory[mem].rampntr=MK_FP(0xA000,0);
memory[mem].len=memory[mem].fullen=64000u;
//переставлением x/z размеров можно добиться читабельности
//выводимого в 'битмап' VGAMEM текста, но с поворотом на бок
memory[mem].lenx=200;
memory[mem].lenz=320;

//подготовка ножа только _после_ этого!
PencilSetup();
}

void PencilSetup(void)
//сетап ресурсов ножа
{
int pan,bm,x,z,ze,lenx,lenz,lenkl,lenru,col;

lenz=20;
lenx=80;
lenkl=60;//длина клинка ножа
lenru=lenx-lenkl;//длина ручки
bm=GetLockMEM2(ADDR_VRS,lenx,lenz,FIX1,YES);
//по краям битмапа будет пусто
PaintFullBitmap(bm,0);
//красим нож
ze=lenz/2;
for (z=1; z<ze; ++z)
  {
  //ручка
  col=red[z+2];
  bmline(bm,lenkl,z,       0,lenru,col);
  bmline(bm,lenkl,lenz-1-z,0,lenru,col);
  //клинок
  col=gray[z+4];
  bmline(bm,0,z,       0,lenkl,col);
  bmline(bm,0,lenz-1-z,0,lenkl,col);
  }
//делаем заострение на конце клинка
for (z=1; z<ze; ++z)
  {
  x=ze-z+2;
  bmline(bm,0,z,       0,x,0);
  bmline(bm,0,lenz-1-z,0,x,0);
  }
//ширину клинка чуть уменьшаем
PaintBitmap(bm,0,     0,lenkl-1,     2,0);
PaintBitmap(bm,0,lenz-3,lenkl-1,lenz-1,0);
//ширину части ручки чуть уменьшаем
PaintBitmap(bm,lenkl+2,     0,lenx-3,     4,0);
PaintBitmap(bm,lenkl+2,lenz-5,lenx-3,lenz-1,0);

//панель ножа
pan=pnclpan=GetPAN(ADDR_VRS);
LockPAN(pan);
SetPanelRes(pan,bm,ISNOT);
SetPanLenxz(pan,160,20);
FullPanelBmap(pan);
}

void FistVRStask(void)
//системные функции в начале витка
{
dword dt;
dword nowt=WinTimeMs();

//ПОРТ: системное время теперь от монотонных часов WinTimeMs();
//тик звука перенесен в аудио-поток - не зависит от FPS
vrs_time=nowt;
vrstime0=(word)nowt;
vrstime1=(word)(nowt>>16);

//измеряем время прорисовки предыдущего кадра
disable();
dt=vrs_time-oldtime;
cadrtime= (oldtime==0)? 0:(int)dt;
oldtime=vrs_time;
enable();

//системный генератор синусоиды
//на выходе диапазон fix= -1.0 ... +1.0 !
sysfsinus=fsinus(vrstime0<<Fsys);
//word vrstime0: это младшие 16 бит времени в миллисек!
//при Fsys=6 частота колебаний=(1 за 1024 мсек)=1 Герц!
}

void LastVRStask(void)
//системные функции в конце витка, 1 раз за виток
{
//вкл. реакции на клавиатуру в момент конца _первого_ витка
kbbufvalid=YES;

//меняем нужные параметры панелей
NewPanelsPars();
}

void Sound(int numsound)
//генерация звука с условным номером numsound
{
static int i;
switch(numsound)
  {
  default: break;
/*
  case SNDVRSIN    : SoundPars( 98, 7,3,32, 0); break;
  case SNDSTARTPOZ : SoundPars( 21, 7,2,64, 0); break;
  case SNDLILASTART: SoundPars(  0,16,4,32,-1); break;
*/
  case SNDLILAEXIT  : SoundPars( 20, 7,4,64,10); break;
  case SNDCLIPUDAR  : SoundPars(  0, 1,0,32,-1); break;
  case SNDCLIPTOLHOK: SoundPars(  0, 1,1,16,-1); break;
  case SNDFIRE      ://бросок ножа
    SoundPars(-200,3,2,20,10); break;
  case SNDVZRYV     ://взрыв цели
    SoundPars( ++i,7,3,32,20); break;
  }
}

void SoundPars(int seed,int num,int okt,int dur,int noise)
//подготовка параметров и старт нового звука:
//запись в кэш нот num случайных нот с зерном seed и длительностью
//dur мсек каждая, с опорной октавы okt вверх на ~3 октавы, ширина
//полосы шума= noise (в %-х долях от частоты звука);
//при noise<0: гамма вверх из num нот вместо случайных нот!
{
int n,r;
n=soundstate=0;//запрос на стоп звука
if (!WinSoundActive()) soundstate=1;//аудио-потока нет - сразу стоп
else
while (soundstate==0)
  if (++n==0) break;//ждём тика секвенсора из аудио-потока (до 4 мс)

if (soundflag==OFF) num=0;//не генерим ноты без звука
num=max(num,0);
num=min(num,MAXCASH);
numnotecash=num;
if (numnotecash<=0) return;
okt=max(okt,0);
okt=min(okt,NUMOKT-3);//-3 вместо -1: 2 октавы за счёт номера ноты!
durnote=dur;
noisediv= (noise<=0)? MAXINT:100/noise;
Srandu(seed);
for (n=0; n<num; ++n)//суём ноты в кэш
  {
  //гамма вверх или случайные ноты 0...15
  r= (noise<0)? n:Randu()%16;
  notecash[n]=12*okt+notelad[r];
  }
soundstate=2;//старт звука
}

void SoundOnOff(void)
//порт: запрет и разрешение звука - гейт бэкенда вместо порта 61h
{
SoundGateHW((soundflag==OFF)? 0:1);
}

