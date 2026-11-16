/*
  VRSPORT.H - слой совместимости Borland C (DOS 16-bit) -> MinGW C (Win32)
  Включается в портируемые модули ДО включения vrsNN.h.
  Не определяет byte/word/dword/fix/angle - их определяет vrsNN.h!
*/

#ifndef VRSPORT_H
#define VRSPORT_H

#include <stdint.h>//uintptr_t

/*--- модели памяти DOS -> плоская память Win32 ---*/
#ifndef VRSPORT_INTERNAL_NO_MEMMODEL
#define far
#define near
#define huge
#undef _fastcall/*MinGW предопределяет _fastcall как __fastcall -
  а в оригинале это просто ключевое слово Turbo C: делаем пустым*/
#define _fastcall
#endif

/*--- cli/sti в однопоточном порте не нужны ---*/
#define disable()  ((void)0)
#define enable()   ((void)0)

/*--- FP_SEG/FP_OFF: на плоской памяти - части указателя (для сортировок
  и проверок выравнивания); MK_FP оставлен только для VGA-адресации
  A000:offset - все такие вызовы заменяются на WinGetVRAM()+offset ---*/
#define FP_OFF(p)  ((unsigned)((uintptr_t)(void*)(p)&0xFFFFu))
#define FP_SEG(p)  ((unsigned)(((uintptr_t)(void*)(p)>>16)&0xFFFFu))
#define MK_FP(seg,off) ((void *)(WinGetVRAM()+((long)(off))))

/*--- числовые пределы Turbo C ---*/
#ifndef MAXLONG
#define MAXLONG  2147483647L
#endif
#ifndef MAXINT
#define MAXINT   32767
#endif

/*--- argc/argv для '_argc/_argv' Borland ---*/
extern int VRS_argc;
extern char **VRS_argv;

/*--- глобальная мода файлов Borland CRT (msvcrt ее имеет) ---*/
extern int _fmode;

/*--- фикс-точка: вместо VRS54A.OBJ (asm 386) ---*/
/*fix = signed long 16.16 (см vrsNN.h); все операции с 64-бит
  промежуточным результатом, как в оригинальном asm.
  Отличие от DOS: fixdiv/lfixmuldiv при переполнении дают насыщение
  (в DOS ловился int 0 и делимое делось пополам до успеха).
  Прототипы через long, т.к. fix определяется позже в vrsNN.h*/
long fixmul(long x,long y);//        (x*y)>>16
long fixdiv(long x,long y);//        (x<<16)/y
long lfixmuldiv(long a,long x,long y);// (a*x)/y, 64-бит промежут.
long fixmulsumdiv(long a,long x,long b,long y,long z);// (a*x+b*y)/z

/*--- min/max как в Turbo C stdlib ---*/
#ifndef max
#define max(a,b) (((a)>(b))? (a):(b))
#endif
#ifndef min
#define min(a,b) (((a)<(b))? (a):(b))
#endif

/*--- бэкенд Windows (видео/ввод/таймер/звук) - см dos2win.h ---*/
#include "dos2win.h"

#endif//VRSPORT_H
