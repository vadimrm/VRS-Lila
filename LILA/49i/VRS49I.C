/*
                 Virtual Reality Shell Lila
                    Data Installation

                           Новое:

  -6 версия инсталлятора файлов данных, сокращен код

                           Надо:

*/

#include <dos.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "scod.h"
#include "kbcod.h"


typedef unsigned char  byte;
typedef signed   char  sbyte;
typedef unsigned int   word;
typedef unsigned long  dword;
typedef signed   long  fix;//фиксация запятой 16.16 бит !
typedef word angle;//углы Lila в лидусах, 360 градусов=65536 лидусов!

word Read(int handle,void far *buf,word len);
word Write(int handle,void far *buf,word len);
void SinTabSave(void);
void Scod2kbTabSave(void);


//да-нетные переключатели-индикаторы
#define OFF    0
#define ON     1
#define NO     0
#define YES    1
#define OK     0
#define ERR    1


byte scod2kbcod[256];//таблица перекодировки scod->kbcod

#define NUMSIN   4097
fix Fsin[NUMSIN];//Fsin[4096]=sin(90 градусов)!
//константы связи градусов с лидусами
#define ANGSTEP     4 //шаг по углу Fsin
#define ANG180G 32768u//=лидусов в 180 градусах



void main(void)
{
printf("\nVRS Lila Data Installation part\n");
printf(  "(C) Vadim Madgazin, 1995, 1996.\n\n");

Scod2kbTabSave();

SinTabSave();

printf("Normal end of data installation !\n");
}

word Write(int handle,void far *buf,word len)
{
static word n;
return _dos_write(handle,buf,len,&n);
}

word Read(int handle,void far *buf,word len)
{
word n;
_dos_read(handle,buf,len,&n);
return n;
}

void Scod2kbTabSave(void)
//создание и запись на диск таблицы scod2kbcod[256]
{
int hnd;
int kbcod,scod;

printf("Comput keyboard.lii ... ");

for (scod=0; scod<256; ++scod)
  {
  //преобразование scod->kbcod:
  switch(scod)
    {
    //код 255 для отсутствующих в scod.h спецкодов, напр. корейских...
    default: kbcod=255; break;

    case CodEsc     : kbcod=KeyCodEsc      ; break;
    case CodF1      : kbcod=KeyCodF1       ; break;
    case CodF2      : kbcod=KeyCodF2       ; break;
    case CodF3      : kbcod=KeyCodF3       ; break;
    case CodF4      : kbcod=KeyCodF4       ; break;
    case CodF5      : kbcod=KeyCodF5       ; break;
    case CodF6      : kbcod=KeyCodF6       ; break;
    case CodF7      : kbcod=KeyCodF7       ; break;
    case CodF8      : kbcod=KeyCodF8       ; break;
    case CodF9      : kbcod=KeyCodF9       ; break;
    case CodF10     : kbcod=KeyCodF10      ; break;
    case CodF11     : kbcod=KeyCodF11      ; break;
    case CodF12     : kbcod=KeyCodF12      ; break;
    case CodPrntScrn: kbcod=KeyCodPrntScrn ; break;
    case CodScrlLck : kbcod=KeyCodScrlLck  ; break;
    case CodPause   : kbcod=KeyCodPause    ; break;
    case CodTilda   : kbcod=KeyCodTilda    ; break;
    case Cod1       : kbcod=KeyCod1        ; break;
    case Cod2       : kbcod=KeyCod2        ; break;
    case Cod3       : kbcod=KeyCod3        ; break;
    case Cod4       : kbcod=KeyCod4        ; break;
    case Cod5       : kbcod=KeyCod5        ; break;
    case Cod6       : kbcod=KeyCod6        ; break;
    case Cod7       : kbcod=KeyCod7        ; break;
    case Cod8       : kbcod=KeyCod8        ; break;
    case Cod9       : kbcod=KeyCod9        ; break;
    case Cod0       : kbcod=KeyCod0        ; break;
    case CodMinus   : kbcod=KeyCodMinus    ; break;
    case CodPlus    : kbcod=KeyCodPlus     ; break;
    case CodBS      : kbcod=KeyCodBS       ; break;
    case CodIns     : kbcod=KeyCodIns      ; break;
    case CodHome    : kbcod=KeyCodHome     ; break;
    case CodPgUp    : kbcod=KeyCodPgUp     ; break;
    case CodNL      : kbcod=KeyCodNL       ; break;
    case CodAddDiv  : kbcod=KeyCodAddDiv   ; break;
    case CodAddMul  : kbcod=KeyCodAddMul   ; break;
    case CodAddMinus: kbcod=KeyCodAddMinus ; break;
    case CodTab     : kbcod=KeyCodTab      ; break;
    case CodQ       : kbcod=KeyCodQ        ; break;
    case CodW       : kbcod=KeyCodW        ; break;
    case CodE       : kbcod=KeyCodE        ; break;
    case CodR       : kbcod=KeyCodR        ; break;
    case CodT       : kbcod=KeyCodT        ; break;
    case CodY       : kbcod=KeyCodY        ; break;
    case CodU       : kbcod=KeyCodU        ; break;
    case CodI       : kbcod=KeyCodI        ; break;
    case CodO       : kbcod=KeyCodO        ; break;
    case CodP       : kbcod=KeyCodP        ; break;
    case CodLBracket: kbcod=KeyCodLBracket ; break;
    case CodRBracket: kbcod=KeyCodRBracket ; break;
    case CodEnter   : kbcod=KeyCodEnter    ; break;
    case CodDel     : kbcod=KeyCodDel      ; break;
    case CodEnd     : kbcod=KeyCodEnd      ; break;
    case CodPgDn    : kbcod=KeyCodPgDn     ; break;
    case CodAdd7    : kbcod=KeyCodAdd7     ; break;
    case CodAdd8    : kbcod=KeyCodAdd8     ; break;
    case CodAdd9    : kbcod=KeyCodAdd9     ; break;
    case CodAddPlus : kbcod=KeyCodAddPlus  ; break;
    case CodCapsLock: kbcod=KeyCodCapsLock ; break;
    case CodA       : kbcod=KeyCodA        ; break;
    case CodS       : kbcod=KeyCodS        ; break;
    case CodD       : kbcod=KeyCodD        ; break;
    case CodF       : kbcod=KeyCodF        ; break;
    case CodG       : kbcod=KeyCodG        ; break;
    case CodH       : kbcod=KeyCodH        ; break;
    case CodJ       : kbcod=KeyCodJ        ; break;
    case CodK       : kbcod=KeyCodK        ; break;
    case CodL       : kbcod=KeyCodL        ; break;
    case CodSemicolon:kbcod=KeyCodSemicolon; break;
    case CodInvCommas:kbcod=KeyCodInvCommas; break;
    case CodBackSlash:kbcod=KeyCodBackSlash; break;
    case CodAdd4    : kbcod=KeyCodAdd4     ; break;
    case CodAdd5    : kbcod=KeyCodAdd5     ; break;
    case CodAdd6    : kbcod=KeyCodAdd6     ; break;
    case CodLShift  : kbcod=KeyCodLShift   ; break;
    case CodKey102  : kbcod=KeyCodKey102   ; break;
    case CodZ       : kbcod=KeyCodZ        ; break;
    case CodX       : kbcod=KeyCodX        ; break;
    case CodC       : kbcod=KeyCodC        ; break;
    case CodV       : kbcod=KeyCodV        ; break;
    case CodB       : kbcod=KeyCodB        ; break;
    case CodN       : kbcod=KeyCodN        ; break;
    case CodM       : kbcod=KeyCodM        ; break;
    case CodComma   : kbcod=KeyCodComma    ; break;
    case CodPoint   : kbcod=KeyCodPoint    ; break;
    case CodDiv     : kbcod=KeyCodDiv      ; break;
    case CodRShift  : kbcod=KeyCodRShift   ; break;
    case CodUp      : kbcod=KeyCodUp       ; break;
    case CodAdd1    : kbcod=KeyCodAdd1     ; break;
    case CodAdd2    : kbcod=KeyCodAdd2     ; break;
    case CodAdd3    : kbcod=KeyCodAdd3     ; break;
    case CodAddEnter: kbcod=KeyCodAddEnter ; break;
    case CodLCtrl   : kbcod=KeyCodLCtrl    ; break;
    case CodLAlt    : kbcod=KeyCodLAlt     ; break;
    case CodSpace   : kbcod=KeyCodSpace    ; break;
    case CodRAlt    : kbcod=KeyCodRAlt     ; break;
    case CodRCtrl   : kbcod=KeyCodRCtrl    ; break;
    case CodLeft    : kbcod=KeyCodLeft     ; break;
    case CodDown    : kbcod=KeyCodDown     ; break;
    case CodRight   : kbcod=KeyCodRight    ; break;
    case CodAdd0    : kbcod=KeyCodAdd0     ; break;
    case CodAddDel  : kbcod=KeyCodAddDel   ; break;
    }

  scod2kbcod[scod]=kbcod;
  }

_fmode=O_BINARY;
//запись на диск (со стиранием старого файла)
if ((hnd=creat("keyboard.lii",S_IREAD|S_IWRITE))==-1)
  {
  printf("file create error !\n");
  exit(1);
  }

Write(hnd,&scod2kbcod[0],256);
close(hnd);

printf("file save !\n\n");
}

void SinTabSave(void)
//рассчет тригонометрических таблиц и запись на диск
{
float fcon,frad,floatsin;
int i,hnd;
angle ang;
fix f;

printf("Comput sinus.lii ... ");

fcon=M_PI/(float)ANG180G;
//рассчет таблиц синусов-косинусов
for (i=0; i<NUMSIN; ++i)
  {
  ang=i*ANGSTEP;
  frad=fcon*(float)ang;
  floatsin=sin(frad);
  //ограничение значащих цифр 17-ю младшими битами
  f=(fix)(floatsin*65536.);
  if (f> 65536L) f= 65536L;
  if (f<-65536L) f=-65536L;
  Fsin[i]=f;
  }

_fmode=O_BINARY;
//запись на диск (со стиранием старого файла)
if ((hnd=creat("sinus.lii",S_IREAD|S_IWRITE))==-1)
  {
  printf("file create error !\n");
  exit(2);
  }

Write(hnd,Fsin,sizeof(Fsin));
close(hnd);

printf("file save !\n\n");
}

