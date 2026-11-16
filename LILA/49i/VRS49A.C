/*
   Lila ASM386-СИ код всего, кроме рендеров !

   Важно:
   -вручную выкидывать часть асм-кода New0Interrupt() !
   -без компиляторной оптимизации код короче и быстрее?
   -все 12 предупреждений компилятора игнорировать!
*/

#pragma  inline
#include "vrs49.h"
#include <dos.h>
#include <values.h>

void interrupt far New0Interrupt(void)
//адрес возврата из int0 для 80286 и старше равен адресу команды
//DIV/IDIV, вызвавшей это прерывание при переполнении результата!
{
//в асм-коде вручную выкинуть _все_ инструкции кроме 3-х ниже
//приведенных в СИ-коде (надо выкинуть push,pop,mov...) !!!!!

//уменьшаем делимое, пока прерывание не перестанет вызываться,
//делитель не должен равняться нулю, иначе зацикливание int0 !
asm{
   shrd  eax,edx,1//сдвиг вправо eax с добавлением мл. бита edx
   sar   edx,1    //сдвиг вправо edx с сохранением знака
   iret
   }
//этот код считает что деление было dword, если же размеры
//операндов word/byte, то хотя защита в конце-концов тоже
//сработает, но результат может иметь произвольную величину
//и даже неверный знак, т.к. на него повлияют нерабочие
//байты edx и eax...
}

fix fixmul(fix x,fix y)
{
asm{
   mov   eax,dword ptr x
   imul  dword ptr y//результат x*y в edx:eax
   shr   eax,16     //старшее слово eax в ax
   //fix возвращаем в dx:ax (это требует BC++) !
   }
}

fix fixdiv(fix x,fix y)
{
asm{
   mov   eax, dword ptr x
   cdq              //расширение знака eax в edx

   //сдвигаем операнд x=edx:eax влево на 16 бит
   shld  edx,eax,16 //старшее слово eax в dx
   shl   eax,16     //передвигаем 16 бит ax на старшее слово eax

   idiv  dword ptr y//edx:eax = x/y, частное в eax
   shld  edx,eax,16 //старшее слово eax->dx:
   //целая часть fix в dx, дробная часть fix в ax
   //fix возвращаем в dx:ax !
   }
}

long lfixmuldiv(long a,fix x,fix y)
{
asm{
   mov   eax,dword ptr a
   imul  dword ptr x//результат a*x в edx:eax
   //т.к. long*fix=fix, после запятой результата всего 16 бит!
   idiv  dword ptr y//edx:eax = (a*x)/y, частное в eax (остаток в edx)
   shld  edx,eax,16 //старшее слово eax в dx
   //_long int_ возвращаем в dx:ax !
   }
}

fix fixmulsumdiv(fix a,fix x,fix b,fix y,fix z)//(a*x+b*y)/z
{
asm{
   mov   eax,dword ptr a
   imul  dword ptr x//результат  a*x в edx:eax
   mov   ebx,eax
   mov   ecx,edx    //запоминаем его в ecx:ebx
   mov   eax,dword ptr b
   imul  dword ptr y//результат  b*y в edx:eax
   add   eax,ebx
   adc   edx,ecx    //сумма  a*x+b*y в edx:eax
   idiv  dword ptr z//edx:eax = сумма/z, частное в eax
   shld  edx,eax,16 //старшее слово eax->dx:
   //целая часть fix в dx, дробная часть fix в ax
   }
}

void MoveDword(word sseg,word soff,word dseg,word doff,word len4)
//вариант mowedata для 4 байтных dword;
//оба soff/doff должны быть выровнены по границе dword!
{
asm{
   cld
   mov   cx,len4
   mov   di,doff
   mov   es,dseg
   mov   si,soff
   push  ds
   mov   ds,sseg
   rep   movsd
   pop   ds
   }
}

void SetDword(void far *dst,word numbyte4,int val)
//вариант Setmem для заполнения numbyte4 dword парой байт val;
//dst должен быть выровнен по границе dword!
{
word dseg,doff;
dseg=FP_SEG(dst);
doff=FP_OFF(dst);
asm{
   mov   es,dseg
   mov   edi,0
   mov   di,doff
   mov   ecx,0
   mov   cx,numbyte4
   mov   ax,val
   shl   eax,16
   mov   ax,val
   cld
   rep   stosd
   }
}

void Setmem(void far *dst,word numbyte,int val)
//вариант setmem для void _far_ *dst, с 2-х байтным val !
{
word dseg,doff;
dseg=FP_SEG(dst);
doff=FP_OFF(dst);
asm{
   mov   es,dseg
   mov   di,doff
   mov   cx,numbyte
   mov   ax,val
   }
asm cld
asm test    di, 1
asm jz  isAligned
asm jcxz    done
asm stosb
asm dec cx
isAligned:
asm shr cx, 1
asm rep stosw
asm jnc noOdd
asm stosb
noOdd:
done: ;
}

