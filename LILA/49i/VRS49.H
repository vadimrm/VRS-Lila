/*
  H-файл определений, функций и глобальных переменных
*/

typedef unsigned char  byte;
typedef signed   char  sbyte;
typedef unsigned int   word;
typedef unsigned long  dword;
typedef signed   long  fix;//фиксация запятой 16.16 бит !
typedef word angle;//углы Lila в лидусах, 360 градусов=65536 лидусов!

typedef struct//структура данных системы 3D координат, 14 байт
{
//оригин и углы осей дочерней системы координат или панели в ней
fix x0,y0,z0;
//угол от оси материнской системы к этой же оси дочерней системы
angle angx;//,angy,angz;
} d3poz;
extern d3poz d3null;//нулевая позиция


int GetLockMEM2(int owner,int lenx,int lenz,fix fmulz);
void ComputSyspal(void);
void ComputGradcol(void);
void SyspalSetup(void);
void GradcolSetup(void);
void RenderSteklPanels(void);
void QuickSort(int maxind,long *srtkey,int *srtind);
void PaintGranBitmap(int bm,int glen,int mode,int col);
void pangran(int pan,int glen,int mode,int *gcol,int i0,int i1,int istep);
void pangran2(int pan,int len,int mode,int *col,int i0,int i1,int i2,int st4);
void RenderPanel(int srcpan);
void Fractal(int far *arr1025,int dclevel,int *harmonics,int seed);
void PaintZbitm(void);
void PaintDbitm(void);
void RenderZ(void);
void RenderD(void);
void RenderNebo(void);
void RenderStars(void);
int _fastcall powercol(int color);
void DeChoiceAP(void);
void SetPalette(byte far *rgb,int startcol,int endcol);
void SetPanelRes(int pan,int bmap,int conv);
int GetPAN(int owner);
void PENSetup(void);
void Setup000(void);
void CursorSetup(void);
void PencilSetup(void);
void CopyMEM(int dstmem,int srcmem);
void WarningWKP(char *str,int code);
void SetConvert1to1(int conv);
int let2i(int let);
void ComputConvsys(byte far *srcpal,byte far *dstram,int transcol);
void CursorOnPanel(int pan);
void Bestpoz(int dir,int mode,int pan);
int GetPANtxt(int owner,int glen,int tlenx,int tlenz,int panlenx,int consti);
void PaintFullBitmap(int bm,int col);
void RandColors(int *col,int len,int r,int g,int b,int dr,int dg,int db);
void RandPaintBitmap(int bm,int x0,int z0,int x1,int z1,int *col,int len);
void bmpixel(int bm,int x,int z,int col);
void bmline(int bm,int x0,int z0,int grad,int len,int col);
int pantext(int pan,int glen,int fgcol,int hor,int ver,char *format,...);
void panfill(int pan,int glen,int bgcol,int fglet,int letcol,int stepz);
int GetLockMEMconv(int owner);

void AllPANdataSetup(void);
void AllMEMdataSetup(void);
void ZeroPANhead(int n);
void ZeroMEMhead(int n);
int GetLockMEM(int owner,word len);
void LockMEM(int n);
void LockPAN(int n);
void UnLockPAN(int n);
void UnLockMEM(int n);
void CopyPAN(int dst,int src);
void SetMEMtoConstval(int mem,int val);
void USERandPENstartpoz(void);
void CompressMEM(void);

void SetRAM(void far *dst,word numval,int val,int lenval);
void CopyRAM(void far *dst,void far *src,word len);
void Srand(word seed);
void Srand2(word seed);
void Srandi(word seed);
void Srandu(word seed);
int Rand(void);
int Rand2(void);
int Randi(void);
int Randu(void);
int LoadSinTabFile(char *fname);
int LoadScod2kbTabFile(char *fname);
int LoadPalTabFile(char *fname);
int LoadYcolTabFile(char *fname);
int LoadFontFile(char *fname);
word Read(int handle,void far *buf,word len);
word Write(int handle,void far *buf,word len);
int OpenRead(char *fname,long offset,void far *buf,word len);
void WaitKeyPress(void);
void SetTimerFrequency(long firq);
void CopyRight(void);
void SetScreenMode(int mode);
int GetScreenMode(void);
void SetBorderColor(int bclr);
void NewInterruptHandlers(void);
void OldInterruptHandlers(void);
void TablesSetup(void);
void EndProgram(int cod);
void KeyboardBufferScan(void);
void SystemMessageFiltr(void);
void MessageDeleter(void);
void AllAPdataSetup(void);
void SetMSGBufferToEmpty(void);
void SetAllAPAddress(void);
int AddShortMSG(int srcaddr,int dstaddr,word message);
int AddIntMSG(int srcaddr,int dstaddr,word message,int intdata);
int Add2byteMSG(int srcaddr,int dstaddr,word message,int data0,int data1);
void OverShortMSG(int msgnum,int srcaddr,int dstaddr,word message);
int GetIntMSG(int poz);
int MSGBufNotFull(void);
dword VRSTime(void);
void MSGDel(int num);
void RAMAlloc(void);
int ConvertMEMmode(int mem,int rammode);
void VRSWorldSetup(int argnum,char *(argv[]));
int _fastcall Color(int r,int g,int b);
void SetPanelSize2Bmap(int pan);
angle grad2ang(int grad);
void USERearfixXYZgrad(fix x,fix y,fix z,int grad);
void USERearXYZgrad(int x,int y,int z,int grad);
void ConvertFontMode(void);
void PaintText(char far *tstr,int tlen,int bmap,int x,int z,
               int fgbg,int fgcol,int bgcol);
void PaintText88(char far *tstr,int tlen,int bmap,int x,int z,
                 int fgbg,byte far *fgcol,int bgcol);
d3poz XYZang2D3poz(int x,int y,int z,angle ang);
d3poz XYZgrad2D3poz(int x,int y,int z,int grad);
d3poz fixXYZang2D3poz(fix x,fix y,fix z,angle ang);
d3poz fixXYZgrad2D3poz(fix x,fix y,fix z,int grad);
fix iifixdiv(int x,int y);// x/y
fix fixmul(fix x,fix y);// x*y
fix fixdiv(fix x,fix y);// x/y
long lfixmuldiv(long a,fix x,fix y);// a*x/y
//fix fixmuldiv(fix  f,fix x,fix y);// f*x/y
//вместо fixmuldiv используем эквивалентную ей lfixmuldiv
#define fixmuldiv(f,x,y) lfixmuldiv(f,x,y)
fix   fixmulsumdiv(fix a,fix x,fix b,fix y,fix z);//(a*x+b*y)/z
#define WKP WaitKeyPress();
fix fcosin(angle ang);
fix fsinus(angle ang);
#define wang2grad(x) ((int)(((x)*360L)>>16))//преобр-е лидусов в градусы
void MoveDword(word sseg,word soff,word dseg,word doff,word len4);
void SetDword(void far *dst,word numbyte4,int val);
void Setmem(void far *dst,word numbyte,int val);
void MouseReset(void);
void MouseStatus(void);
void MouseSense(void);
void PaintBitmap(int bm,int x0,int z0,int x1,int z1,int col);
void LilaPaint(void);
void LilaPaintSetup(void);
int _fastcall color(int r16,int g16,int b16);
void PencilPoz(int numpoz);
void ComputPanelsPosition(void);
int PanelIsVisible(int n);
void SetYbuf(int ydist);
d3poz ConvD3pozFromAP2Earth(d3poz pn,d3poz ap);
d3poz ConvD3pozFromEarth2AP(d3poz pea,d3poz ap);
void ClipComput(void);
void ClipSwitch(void);
void ColorArraySetup(void);
int _fastcall hexcol(int rgb);
void ConvertPalette(byte far *spal,byte far *dpal,fix colorness,int startcol);

//это все AP по порядку адресов 0,1,2,3...
void AP_Null(void);
void AP_First(void);
void AP_Render(void);
void AP_OutVGAgra(void);

//мнемонические адресные константы AP (для удобства)
#define ADDR_APFirst  1
#define ADDR_APRend   2
#define ADDR_APOVGAg  3
#define ADDR_LAST_AP  3

//int-функции
void interrupt far KeyboardInterrupt(void);
void interrupt far NewTimerInterrupt(void);
void interrupt far New0Interrupt(void);//ошибка деления!

//указатели функций
extern void (interrupt far *OldTimerInterrupt)(void);
extern void (interrupt far *OldKeyboardInterrupt)(void);
extern void (interrupt far *Old0Interrupt)(void);
extern void (interrupt far *Mouse)(void);

//числовые константы
#define L65536   65536L
#define L32768   32768L
#define FIX1    L65536//fix 1
#define FIX05   L32768//fix 0.5
#define FIX6D5   78643L//fix 6/5
#define FIX5D6   54613L//fix 5/6

#define itofix(x) (((long)(x))<<16)//преобразование int  в fix
#define ltofix(x)         ((x)<<16)//преобразование long в fix
//преобразование fix в int
#define pfixtoi(x) ((int)((x)>>16))//для положительных fix используем сдвиг,
#define fixtoi(x) ((int)((x)/L65536))//для любых fix деление!

//да-нетные переключатели-индикаторы флагов
#define ON     1
#define YES    ON
#define OFF    0
#define NO     OFF
//возвращаемые значения функций
#define OK     0
#define ERR    1

extern long Seed,Seed2,Seed32;
extern word Seed16;

//формат кода клавиши кейборда, 6 байт на код
typedef struct
{
word t1,t0;//половинки dword vrs_time, времени появления кода в мсек
byte hit;//признак действия, см ниже
byte scod;//спецкод клавиши из файла scan3.h
} KeyboarD;
#define KEYOFF     0//физическое отжатие клавиши
#define KEYOFFREP  1//повторный (soft) сигнал об отжатии, пока нет!
#define KEYON      2//физическое нажатие клавиши
#define KEYONREP   3//повторный (soft/hard) сигнал о нажатии,
  //soft сигнала пока нет, а hard трактуется как физическое нажатие!

extern int kbbufvalid;//флаг разрешения буферизации кодов клавиш
//буфер нижнего уровня, только для накопления ввода между кадрами !
#define KBBUF  32
extern KeyboarD far kbbuf[KBBUF];//кольцевой буфер кодов клавиатуры
extern volatile int kbwrite;//позиция для новой записи
extern int kbread;//позиция для нового чтения этого буфера, т.о.
  //читать надо блок от kbread до kbwrite-1 включительно!

extern volatile int anycodflag;//флаг для проверки был ли ввод любой из клавиш;

extern byte far scod2kbcod[256];//таблица перекодировки scod->kbcod;

//формат мессага буфера сообщений, 14 байта на мессаг
typedef struct
{
dword time;//время генерации, мсек;
int count;//счетчик номера витка жизни мессага, начиная с 0
//эти адреса типа сетевых адресов программ в пакете IPX:
int srcaddr,dstaddr;
word cmd;//код команды
byte data[2];//2 байта данных, формат зависит от кода команды;
} MessagE;

#define MSGBUF 64//в msg-буфер должны помещаться _все_
  //сообщения kb/VRS/AP, появляющиеся за самый длинный виток!
extern MessagE far msg[MSGBUF];//кольцевой буфер сообщений VRS и AP
extern int msgwrite;//позиция для новой записи
extern int msgread;//позиция для нового чтения этого буфера, т.о.
  //читать надо блок от read до write-1 включительно!

//коды сообщений и форматы полей data[]
#define MSG_NULL          0//пустое сообщение, всегда нулевой код !!!
#define MSG_KBPC_DATA     1//данные стандартной клавиатуры PC:
  //data[0]=0/2 для нажатия/отпускания клавиши
  //data[1]=код клавиши из файла kbcod.h;
#define MSG_AP_SET_ADDR   2//принять индивидуальный адрес AP:
  //data[0]=младший байт адреса AP, data[1]=старший байт адреса AP;
//первым обращением системы к AP должно быть сообщение индивид-го адреса
  //AP подачей MSG_AP_SET_ADDR (с ADDR_ALL), иначе AP не будут доступны
  //ее личные данные из массива APdatA AP[] !
  //это сообщение надо передавать достаточно аккуратно, лучше в самом
  //начале работы системы вызвать цикл установки индив-х адресов AP !
  //после этой установки AP может удалить мессаг или нет - защититься!
#define MSG_AP_CSTART     4//начать работу AP 'с нуля'
#define MSG_AP_END        5//закончить работу AP
#define MSG_AP_STOP       6//приостановить работу AP
#define MSG_AP_WSTART     7//продолжить работу AP после приостановки
#define MSG_AP_INPUT_ON   8//подключение ввода (курсор и клава) к AP (выбор)
#define MSG_AP_INPUT_OFF  9//отключение ввода в AP (откл. выбора AP)

//константы адресов в сообщении
#define ADDR_NOBODY  -2//'ничей' адрес
#define ADDR_ALL     -1//VRS и все AP, 'общий адрес'
  //этот общий адрес присвоен AP до получения ими MSG_AP_SET_ADDR !
#define ADDR_VRS      0//VRS==AP с нулевым адресом !
//личные адреса AP не фиксированы, их устанавливает система в диапазоне
//1...32767 ('особые' AP могут иметь фикс. адреса в том же диапазоне ???).

//структура данных AP, 22 байта
typedef struct
{
int state;//состояние AP;
int taskflag;//флаг вызова кода AP в витке системы;
void (near *taskadress)(void);//указатель на начало кода задачи;
//оригин AP и углы осей системы AP, в материнской системе земли
d3poz ear;//например ear.angx это угол X_земли->X_AP;
int earshift;//флаг=YES при каждом изменении ear, что вызовет
  //сброс ...valid флагов всех панелей этой AP при рендере!
} APdatA;

#define MAXAP  16//макс-е число AP в системе=MAXAP-1;
extern APdatA far AP[MAXAP];//массив структур данных AP, индекс=адрес AP!
extern int numap;//адреса AP в текущей версии системы=1...numap;

//(рекомендуемые) коды состояний AP (если не указано иное, то реакция AP
  //только на сообщения по индивид-му адресу AP или ADDR_ALL):
#define AP_START    0//(начальное) нерабочее состояние:
  //AP в этом состоянии реагирует только на MSG_AP_CSTART, по которому
  //производятся [пере]запуск AP, состояние AP меняется на AP_WORK;
#define AP_STOP     1//приостановленное состояние AP:
  //реакция только на MSG_AP_WSTART, MSG_AP_CSTART, MSG_AP_END;
#define AP_WORK     2//рабочее состояние AP:
  //реакция AP на все сообщения, напр MSG_KBPC_DATA и др.

/* панель 'стоит' в плоскости XZ, ее оригин: левый нижний край
 верх Z ^*******
        |*******          ось X панели лежит в плоскости XY AP
        |*******          ось Y панели лежит в плоскости XY AP
 низ  O +--------> X      ось Z панели   параллельна оси  Z AP */
typedef struct//структура данных панели, 96 байт
{
int flag;//=YES, если наличие панели в мире разрешено
int oborot;//=YES, если панель видна с обратной стороны
int render;//=YES, если кусок панели будет рендериться
int owner;//адрес AP-собственника панели, поля owner остальных структур
  //имеют идентичный смысл: если owner==ADDR_ALL, то соответствующий
  //ресурс может использоваться всеми, иначе - только AP-собственником
  //ресурса; это полезно напр. для обобществления текстур битмапов, но
  //имеет оборотную сторону - любая AP может испортить общий ресурс...
int count;//счетчик ссылок на заголовок панели: 0=заголовок свободен,
// 1 и только 1 если заголовок используется !!!
//оригин панели и углы ее осей, в материнской системе координат AP owner-a
d3poz ap;//ap.angx: угол от оси X AP owner-a панели до оси X панели;
//--//-- преобразованные в систему координат земли
d3poz ear;//ear.angx: угол от оси X земли до оси X панели;
//--//-- в USER-систему координат
d3poz pen;//...
//размер панели по горизонтали/вертикали (x/z), сантиметров
fix flenx,flenz;
//xbe-координаты участка битмапа экрана наблюдения с проекцией панели
int xbelft,xbergt;
//действительны ли текущие ear/pen/xbe координаты
int earvalid,penvalid,xbevalid;
//у разных панелей могут быть разные или одинаковые memtex, разные или
//одинаковые memconv, в том числе с номером ISNOT (см его описание):
int memtex;//номер блока памяти с текстурой;
int memconv;//номер 'конверта' (блока папяти с таблицей преобразования
//цветов текстуры в syspal);
//битмап масштабируется по осям X, Z в панель с коэффициентами fmulx
fix fmulx,fmulz;//и fmulz умножения x и y битмапа;
//для масштабирования используется кусок битмапа (включительно концы)
// (xb0,zb0)...(xb1,zb1), разворачиваемого бесконечно с повторением
int xb0,zb0,xb1,zb1;
//пока что при выходе панели за z-края битмапа возникают искажения!
int consti;//уровень постоянной яркости панели:
//если >0, то яркость больше номинального уровня 0, если <0, то меньше;
int neproz;//непрозрачность стеклянной панели:
//панель стеклянная при neproz>0, а прозрачный цвет - всегда стекло;
//панель    обычная при neproz=0, а прозрачный цвет - дырка, если
//memory[memtex].transflag=YES или если у панели есть свой конверт !
int rand;//при rand>=0 случайные колебания пикселей по x и/или z !
} PANdatA;

#define MAXPAN 200//макс-е число панелей
extern PANdatA far panel[MAXPAN];//массив панелей

#define MAXMEMLEN 65520u//в блоке памяти байт не более (64к-16) байт !
typedef struct//структура данных блока памяти, 24 байт
{
int owner;//адрес собственника блока;
int count;//счетчик ссылок на блок;
word len;//количество рабочих байт в блоке, любой четности;
word fullen;//полное количество байт в блоке:
//сначала len рабочих, потом от 0 до 3 нерабочих, завершающих блок,
//fullen всегда кратно четырем ! fullen не более MAXMEMLEN байт !!!
byte far *rampntr;//far указатель на память блока в физической RAM;

//остальные поля имеют смысл только для двумерной памяти, когда
//вместо 'блок памяти' обычно применается термин 'битмап':
int rammode;//порядок следования байт (1 байт на точку):
//0=с левого нижнего  края вправо, потом вверх  на 1 точку   (как у penram)
//1=с левого верхнего края вправо, потом вниз   на 1 (граф.,текст, pp-файл)
//2=с левого нижнего  края вверх,  потом вправо на 1 (src-текстура рендера)
//3=с левого верхнего края вниз,   потом вправо на 1 (таких нет пока)   ...
int lenx,lenz;//длина битмапа по горизонтали и вертикали (x и z), точек;
int transflag;//флаг наличия прозрачного цвета syspal в текстуре, YES/NO;
//рекомендуемый множитель для панелей битмапа (круглость круга текстуры):
fix fmulz;
} MEMdatA;

#define MAXMEM  64//макс-е число блоков памяти
extern MEMdatA far memory[MAXMEM];//массив заголовков блоков памяти

#define ISNOT 0//если номер панели или блока памяти равен ISNOT
//(в проверках можно писать 'меньше или равен ISNOT') то этот
//номер выделен из-за нехватки соотв-го ресурса и во избежание
//зависания или прерывания программы вместо нормального ресурса
//будет использован резервный ресурс с этим нулевым номером=ISNOT !

extern dword ramfullfree;//текущий размер всей свободной нижней far-RAM
extern dword ramfree;//текущий размер непрерывного куска до конца RAM
// +- с far указателем меняют только его offset, здесь надо huge*
extern byte huge *ramfullstart;//начало RAM для всего набора блоков памяти
extern byte huge *ramstart;//начало непрерывного куска до конца RAM

extern volatile dword vrs_time;//счетчик VRS времени в мсек !
extern volatile word  vrstime1,vrstime0;//'половинки' vrs_time для irq1 !
extern dword oldtime;
extern int cadrtime;//время прорисовки кадра в мсек;

extern int startscrnmode;//номер видео-моды до и после программы
extern int actscrnmode;//текущий номер видео-моды;

#define NUMSIN   4097
extern fix far Fsin[NUMSIN];//Fsin[4096]=sin(90 градусов)!
//константы связи градусов с лидусами
#define ANGSTEP     4//шаг по углу Fsin
#define ANG1G     182//~лидусов в 1-м градусе
#define ANG90G  16384//=лидусов в 90 градусах
#define ANG180G 32768u

extern angle Alpha;//угол 'альфа' в формулах рендера, в лидусах!
extern fix fsin,fcos;//sin(Alpha),cos(Alpha);
//расстояние, ближе которого точки панели будут невидимы глазом
#define FIXYPMIN  FIX05//для 0.5 см макс увеличение 200/0.5=400...
//расстояние от глаза наблюдателя до экрана наблюдения:
extern int val_h,max_val_h;
extern fix X0,Y0,Z0;//координаты оригина панели
extern long Zpetop,Zpebtm;//диапазон предполагаемой Z-развертки PEN-pan
//long вместо int из-за возможного выхода за границу int !

#define YBG        32767//y-удаление фона, int !
#define DYFG  (250*FIX1)//расстояние увеличения разрешения по y;
#define YFGSHIFT       9//параметр увеличения разрешения по y;

#define ZGRADMAX   50//+-предел регулировки PEN.zgrad, градусов

#define PENRAMLEN  32000//размер буфера цвета
typedef struct//структура уникальной PEN-панели
{
//систему координат, в которой глаз юзера всегда находится в её оригине
//и смотрит в направлении возрастания координат точно вдоль оси y, будем
//называть USER-системой, или USER;
//оригин и углы осей USER-системы, в материнской системе земли
d3poz ear;//например ear.angx это угол от оси X земли до оси X USER;
int usershift;//флаг=YES при каждом перемещении USER-системы;

//оригин PEN-панели в USER-системе, углы ее осей всегда нулевые!
d3poz poz;
int pozshift;//флаг=YES при каждом перемещении PEN.poz в USER-системе,
//например при изменении 'скоса взгляда'

//PEN-memory поля
int rammode;//=0, мода развертки penram
byte near ram[PENRAMLEN+4];//near память для рендера, буфер цвета
//4 байта запас для выравнивания начала penram по границе dword !
word ramlen;//длина рабочего куска penram;

int tmpmem;//номер заголовка памяти для запоминания части penram
  //после первого шага рендера;

int ymem;//номер заголовка блока памяти для y-buffer (т.н. 'Z-буфер'),
  //расстояние от наблюдателя до точки видимой панели, возможно +- !

//остальные поля панели

//размеры панели по горизонтали/вертикали (x/z), в см
fix lenx,lenz;
int zgrad;//угол вертикального скоса взгляда, -179...+179 градусов;

} PenPan;
extern PenPan PEN;//'панель экрана наблюдения' или PEN-панель
extern byte near *penram;//PEN.ram после выравнивания по dword-ам;

//часто глобально-локально используемые дубли PEN-полей
extern int penpozx0,penpozz0,penpanlenz;//int а не fix !!!

//PEN-bitmap поля вне структуры PEN !
//битмап масштабируется в PEN с коэффициентом z-размеров bmp
extern fix penbmpfmulz;//=6/5
extern fix penbmpfmulz1;//=1/penbmpfmulz=5/6
extern int penbmplenx,penbmplenz;//полные размеры PEN-битмапа, в пикселях
extern int partpenbmplenx;//x-размер его для текущего шага рендера
//число шагов подготовки полного экрана (в рендере 1...2)
extern int numpart;
extern int actpart;//номер текущего шага, начиная с 0;

#define PPPALSHIFT 64000u//начало палитры в pp-файле
#define PAL768     768//байтовая длина палитры 256 цветов

extern byte far nnn[16][16][16];//таблица для расчетов цветов;
extern byte far syspal[PAL768];//VRS-палитра (модиф-я палитра 5.04):
//[rgb компоненты цвета=0...60 с шагом 4]
//получается из стандартной палитры версии 5.04 сдвигом её на 9
//цветов, т.о. основная рабочая часть syspal простирается
#define STARTCOL   9//от цвета номер 9
#define ENDCOL   255//до цвета номер 255,
//всего 247 цветов (как и в 5.04, но там цвета от 0 до 246);
//цвета 0...8 не входят в syspal и служат для:
#define COLTRANS  0//0 - самый удобный прозрачный цвет для рендера,
  //на экране обычно отсутствует, но например в ДОС-эмуляторе под
  //SUN этим цветом заполняется окно эмулятора, которое может быть
  //больше окна ДОСа, поэтому его лучше сделать черным...
#define COLEMPTY  0//цвет пустых участков дисплея, не занятых системным
  //выводом, обычно черный;
#define COLBORDR  0//цвет рамки дисплея, обычно черный;
  //вообще, три эти цвета если и менять, то одинаково !
#define COLPRINT  7//printf() и ДОС, печатают в графрежиме этим цветом
  //(цвет 7 на фоне цвета 0), который сделан для видности ярко-белым;

#define MAXGRADCOL  64//макс. число градаций цветов таблицы gradcol;
extern byte near gradcol[MAXGRADCOL][256];//таблица градаций [яркости] цветов;

//режимы добавления текста в PaintText/88
#define PAINTBG    0//фоном литер
#define PAINTFG    1//литерами
#define PAINTFGBG  2//литерами на фоне

typedef union//структура для текстового фонта 8x8
{
byte pattern[256][8];//[ASCII-код][номер строки]
byte buffer[2048];
} Font;
extern Font fnt;

#define ZAPASX 1//расстояние от принятого в вычислениях конца панели
//до идеального X-конца панели, в единицах самого младшего бита fix!
//если менее 1, то есть ошибки в виде грязи на правом конце панели !
#define ZAPASZ 1//то же для Z;

extern int clip;//флаг клиппинга;

extern int mousepresent;//флаг наличия мыши
extern int mousedx,mousedy;
extern int mouseb0,mouseb1;//клавиша b0/b1 мыши нажата=1, ненажата=0
//числа нажатий/отжатий b0/b1 за последний кадр
extern int mouseb0on,mouseb0off,mouseb1on,mouseb1off;

//параметры указки в любой моде:
extern int pointermode;//мода указки(pointer): 0=карандаш(pencil), 1=курсор(cursor),
extern int pnclpan,curpan;//номер панели указки-карандаша/указки-курсора
extern int pnclbm,curbm;//соответственно номера их битмапов;
//параметры указки в моде0=карандаш
extern int pnclmousemode;//0=мышь перемещает юзера, 1=карандаш
extern int pnclx,pnclz;//экранные x/z _пикселя острия_ _битмапа_ карандаша!
extern int pncldz;//смещение пикселя острия от оригина этого битмапа по z !
extern angle pnclangx;//угол наклона и
extern fix fpncly;//y-координата _панели_  карандаша!
extern int pnclmmm;//pencil moves mouse mode: 0=x/y, 1=x/z;
//параметры указки в моде1=курсор:
extern int curmousemode;//0=мышь перемещает юзера, 1=курсор
extern int curnumap;//номер выбранной AP (или VRS),
extern int curnumpan;//номер выбранной панели, ISNOT если нет
extern int curvisible;//видимость курсора;
extern fix fcurlenx,fcurlenz;//размеры курсора,
extern fix fcurpany;//высота курсора над панелью (если <0, то под панелью),
extern fix fcurstepx,fcurstepz;//шаги перемещения курсора;
//эти параметры целые, в единицах шага перемещения курсора:
//диапазон перемещения курсора в плоскости панели
extern int curminx,curmaxx,curminz,curmaxz;//обычно max=размеру панели/шаг курсора;
extern int curpanx,curpanz;//координаты оригина курсора, в плоскости панели,
extern int curpandx,curpandz;//перемещение за последний кадр, --//--;
extern int curk0,curk1;//состояние кнопки k0/k1 курсора (как b0/b1 мыши),
//число нажатий/отпусканий кнопки k0/k1 за последний кадр
extern int curk0on,curk0off,curk1on,curk1off;

#define PNCLMAXR   500L//радиус выбора панели (от острия карандаша), см
#define FPNCLMAXY (PNCLMAXR*FIX1)//макс удаление карандаша от PEN по Y

#define FMINYSTEPSW (FIX1*200)//ближе этого y шаг карандаша уменьшается
#define FMINSTEPY   (FIX1/32)//минимальный шаг при сдвиге мыши на 1

extern int autobestpoz;//флаг автовозврата к Bestpoz выбранной панели

#define PAN2USER  0//панель идет к Магомету
#define USER2PAN  1//Магомет идет к ней

//аргументы для Bestpoz():
#define MODEBMP  0//битмап с увеличением по x 1:1
#define MODEPAN  1//вся панель на полный экран

extern int bestpozmode;//MODEBMP/MODEPAN
extern int pnclvisible;//видимость карандаша
extern int pnclpozflag;//позиция его

#define RADMIN   4
#define RADMAX  14
#define RADLEN (RADMAX-RADMIN+1)
//набор цветов gray[RADMIN...RADMAX],gray1[MAX...MIN],red[MIN...MAX],
extern int raduga[14][RADLEN];//red1[MAX...MIN],... всего 14 полос радуги !
extern int gray[16],gray1[16],red[16],red1[16],grn[16],grn1[16],blu[16],
blu1[16],redgrn[16],redgrn1[16],redblu[16],redblu1[16],grnblu[16],grnblu1[16];

#define MAXCONSTI (MAXINT/2)//бесконечно высокая неизменная яркость!

extern int logovisible;//видимость лейбла
extern int locksyspan2user;//флаг прицепления системных панелей к юзеру

//состояние одиночных или сдвоенных обще-функциональных клавиш
extern int Shift,Control,Alter;// ON=1, OFF=0 !
extern int CapsLock;//а этот флаг триггеруется!

//минимальный/номинальный размеры памяти для mem-блоков при старте Лила:
#define MINRAMSIZE  170000ul//96к на рендер, 10к запас, 64к на 1 текстуру,
#define NOMRAMSIZE  370000ul//плюс 3*64к еще на 3 текстуры...

//текущие макс номера панели, битмапа, палитры и блока памяти
extern int maxpannum,maxmemnum;//но не ниже нуля!
//текущее количество соотв-х свободных ресурсов системы
extern int freepans,freemems;
extern int errpan,errmem;//номера ошибок >0 при операциях с panel, memory;

#define USERZ0 (180*FIX1)//начальное положение юзера по высоте: +180 см!

extern int printwarn;//флаг печати предупреждений при нефатальных ошибках

extern fix DYuser;//очередное линейное смещение юзера для плаванья по цветам земли;

extern int stepvalue;//флаг-шаг=(1+7*CapsLock)*(8+56*Shift)/(1+7*Alter)=1...512;

#define MAXHORSHIFT 1256//это horshift360 для полного экрана 320x200,
  //MAXHORSHIFT= (31416L*(max_val_h=200)) / 5000 ;
#define MAXVLEN   200//макс размер PEN-bmp (кадра) по вертикали
extern int horshift,horshift360;//см AP_Render

//уровень горизонта проходит по Z=0 USER-системы, если считать
//номер строки низа PEN-bmp =0, то горизонт будет на строке:
extern int jear;//и может быть вне экрана!
extern int jearuser;//строка близкой к юзеру земли на экране

extern int spaceflag;//флаг космоса: OFF=земля и небо, ON=везде небо!

#define STARVAR  5//число вариантов звезд
extern int starvar;//номер варианта, ему ~пропорционально количество звёзд!

#define LILAX 45
#define LILAZ 10
#define LILALEN (LILAX*LILAZ)
extern byte near lila[LILALEN];//битмап лейбла Lila, можно far @

extern int SPmin,SPmax;//max/min (int)_SP !

extern int neprozra;//добавка непрозрачности к стеклу всех панелей;

#define RANDUMUL 843314861L//это хорошие mul*seed+add для
#define RANDUADD 453816693L//случайных 32-битный датчиков !

#define FIXC   8716L//цветность системной палитры, 0.133->fix
#define FIXC1 18940L//=(FIX1-FIXC)/3;

//параметры раскраски цветов 1...8
#define BGVAR  10
#define BGPAR   8
extern int newbgpar;//флаг новых параметров bgpar
extern int bgvar;//текущий номер варианта цветов 1...8
//цвет цвета 1   множитель шага   шаг цвета    яркость цвета
//r,g,b=0...63,  dr,dg,db=0...1,  step=0...8,  power=0...64
//[][0,1,2]      [][3,4,5]        [][6]        [][7]
extern int bgpar[BGVAR][BGPAR];//параметры по вариантам

//наличие рамок вокруг битмапа панели:
//единичные биты 0,1,2,3 означают наличие рамки на краях
#define GRGT    1
#define GTOP    2
#define GLFT    4
#define GBTM    8
#define GALL   15//рамка по всем 4 краям

extern int randfluct;//при panel[].rand>=0: плюс к флуктуациям пикселей;
extern int fluctflag;//0-й бит: флаг флуктуаций по x, 1-й бит - по z !

//параметры изменения градаций (яркостей) цветов панели:
extern int gradcon;//общая постоянная компонента;
extern int gradvarflag;//флаг изменений градаций внутри панели:
extern int gradfx,gradfy,gradamp;//регуляторы частот по x/y и амплитуды;

extern byte zbitm[16];//псевдо-битмап земли
extern int dalibitm;//номер битмапа дали;

extern int nebocol;//абс. номер цвета неба;

//параметры дали:
extern int dalinali;//флаг рендера дали
extern int dalipaint;//битмап дали красить надо ли ?
extern int dali2d;//двумерность битмапа дали
#define DALIVAR   7
#define DALIPAR  54
extern int newdali;//флаг новых параметров дали
extern int dalivar;//номер варианта дали
extern int dnumpar;//номер параметра для регулировки
extern int dalipar[DALIVAR][DALIPAR];
//номер прозрачного цвета для дали (0...255, нет его при <0):
extern int dtranscol;//глобальная копия dalipar[dalivar][0] !!!
//отклонение нулевого уровня дали от уровня горизонта:
extern int dalidjear;//глобальная копия dalipar[dalivar][4] !!!

extern int dalidadt;//скорость вращения дали по углу - для облаков!

//строка периметра дали с параметрами столбцов
#define STRLEN  1024//этот размер фиксирован=2^10 !!!
//в трёх массивах 1+1+2=4 кб, можно near @
extern byte  far dalistrl[STRLEN];//длина столбца дали по вертикали, 0...255
extern sbyte far dalistrh[STRLEN];//вертикальный уровень начала столбца, -128...+127
extern int   far dalistrs[STRLEN+1];//смещение начала столбца в битмапе дали,
  //0...65535 (word !); началом столбца дали считается его нижний пиксель!

#define CURLENX   16
#define CURLENZ   10
#define CURLEN   (CURLENX*CURLENZ)
//можно far @
extern byte near cursor[CURLEN];//массив для загрузки текстуры курсора

