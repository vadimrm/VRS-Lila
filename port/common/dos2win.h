/*
  DOS2WIN.H - бэкенд Windows для порта VRS Lila.
  Заменяет: VGA 13h (A000/DAC-порты), int 8/9/0, int 33h (мышь),
  спикер (PIT канал 2 + порт 61h), dos.h-время, dir.h findfirst.
  API сознательно без типов из vrsNN.h (только char/int/unsigned long),
  чтобы dos2win.c не зависел от версии игры.
  Игровые модули включают dos2win.h ПОСЛЕ vrsNN.h (или standalone).
*/

#ifndef DOS2WIN_H
#define DOS2WIN_H

/*--- версия порта на Windows (для заголовка окна и диалога About) ---
  меняется при каждом коммите; конкретный продукт может переопределить
  WINPORT_VER до включения этого файла (если его версия отличается) ---*/
#ifndef WINPORT_VER
#define WINPORT_VER  "1.03"
#endif
#ifndef WINPORT_YEAR
#define WINPORT_YEAR "2026"
#endif

/*--- жизненный цикл ---*/
void WinInit(int argc,char **argv,const char *title);//окно, консоль, ввод, звук
void WinDone(void);
int  WinQuitRequested(void);//окно закрыто крестом/Alt-F4 -> завершить виток и EndProgram
void WinPump(void);//обработка сообщений окна (вызывать раз в виток)
void WinFrameEnd(void);//легкий ограничитель частоты кадров (Sleep(1))
unsigned long WinTimeMs(void);//монотонное время в мсек (вместо irq0 250Гц)
void WinWaitKeyBlocking(void);//блокировка до нажатия любой клавиши (вместо getch)

/*--- клавиатура: очередь событий scod-кодов (как из ISR int 9) ---*/
/*hit: 0=KEYOFF, 2=KEYON (значения из vrsNN.h скопированы здесь)*/
#define WINKEY_OFF 0
#define WINKEY_ON  2
int WinGetKey(int *scod,int *hit);//0=есть событие, -1=очередь пуста
int WinAnyCodFlag(void);//был ли ввод любой клавиши (anycodflag)

/*--- мышь: семантика int 33h ---*/
int  MousePresent(void);
void MouseResetSense(void);//fn 0x00 + 0x1A: сброс и стандартизация
void MouseGetDelta(int *dx,int *dy);//fn 0x0B: счетчики движения (потребляют)
int  MouseButtons(void);//fn 0x03: бит0=левая, бит1=правая
int  MousePressCount(int b);//fn 0x05: число нажатий с последнего опроса (b: 0/1)
int  MouseReleaseCount(int b);//fn 0x06: число отпусканий с последнего опроса

/*--- видео: 'VGA 13h' 320x200x256 в окне ---*/
#define WINVRAMLEN 64000
unsigned char *WinGetVRAM(void);//буфер 64000 байт (вместо A000:0000)
void VideoSetMode(int mode);//0x13 -> графмод (окно), иначе - текстмод-состояние
int  VideoGetMode(void);//текущий 'режим' (в DOS - int 10h AH=0Fh)
void VideoSetBorderColor(int c);//overscan - в окне не имеет смысла (заглушка)
void VideoSetPalette(const unsigned char *rgb,int startcol,int endcol);
void VideoGetPalette(unsigned char *rgb,int startcol,int endcol);//возврат последнего установленного
void VideoShow(void);//вывод WinGetVRAM() в окно (вместо копирования в A000)

/*--- звук: спикер через waveOut (квадрат-меандр как у PIT режим 3) ---*/
void SoundHWSetup(void);
void SoundToneHW(int pitdiv);//делитель PIT: частота=1193180/pitdiv, div<=2 -> тишина
void SoundGateHW(int on);//гейт порта 61h: 0=тишина всегда
void SoundHWDone(void);

/*--- dos.h время (_dos_gettime/_dos_getdate) ---*/
void GetDosTime(int *hour,int *minute,int *second,int *hsecond,
                int *year,int *month,int *day,int *dayow);

/*--- dir.h: findfirst/findnext с struct ffblk ---*/
#define ff_NAMELEN 256
struct ffblk
{
char ff_name[ff_NAMELEN];
unsigned long ff_fsize;
};
int findfirst(const char *spec,struct ffblk *ff,int attrib);//0=найдено
int findnext(struct ffblk *ff);//0=найдено

/*--- заполнение памяти: копии asm-функций из VRS54A.OBJ ---*/
void MoveDword(void *src,void *dst,int len4);// копия len4 двойных слов
void SetDword(void *dst,int numbyte4,int val);// заполнение numbyte4 двойных слов шаблоном val<<16|val
void Setmem(void *dst,int numbyte,int val);// заполнение numbyte байт

/*--- окно ошибок (вместо printf+getch в EndProgram) ---*/
void WinErrorBox(const char *text);

/*--- меню окна: Menu -> About / Help / Settings ---*/
/*информация об оригинале для диалога About (ASCII, многострочная)*/
void WinSetAbout(const char *originfo);

/*--- громкость звука и ограничение частоты кадров (окно Settings) ---*/
void WinSetVolume(int vol);    /*0..100, по умолчанию 50*/
int  WinGetVolume(void);
void WinSetTargetFPS(int fps); /*10..90, по умолчанию 50, 0=без ограничения*/
int  WinGetTargetFPS(void);
int  WinGetFPS(void);          /*реальная частота витков*/

/*--- звук: регистрация тика секвенсора 250 Гц (4 мс) ---
  callback вызывается из аудио-потока каждые 4 мс независимо от FPS;
  внутри callback игра делает то, что в оригинале делал ISR таймера*/
void WinSoundSetTick(void (*tickcb)(void));
int  WinSoundActive(void);     /*аудио-поток работает?*/

#endif//DOS2WIN_H
