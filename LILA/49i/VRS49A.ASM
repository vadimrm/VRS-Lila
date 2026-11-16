	.386p
	ifndef	??version
publicdll macro	name
	public	name
	endm
	endif
_TEXT	segment byte public use16 'CODE'
_TEXT	ends
DGROUP	group	_DATA,_BSS
	assume	cs:_TEXT,ds:DGROUP
_DATA	segment word public use16 'DATA'
d@	label	byte
d@w	label	word
_DATA	ends
_BSS	segment word public use16 'BSS'
b@	label	byte
b@w	label	word
_BSS	ends
_TEXT	segment byte public use16 'CODE'
	assume	cs:_TEXT
_New0Interrupt	proc	far
	shrd	  eax,edx,1
	sar	   edx,1    
	iret	
_New0Interrupt	endp
	assume	cs:_TEXT
_fixmul	proc	near
	push	bp
	mov	bp,sp
	mov	   eax,dword ptr [bp+4]
	imul	  dword ptr [bp+8]
	shr	   eax,16     
	pop	bp
	ret	
_fixmul	endp
	assume	cs:_TEXT
_fixdiv	proc	near
	push	bp
	mov	bp,sp
	mov	   eax, dword ptr [bp+4]
	cdq	              
	shld	  edx,eax,16 
	shl	   eax,16     
	idiv	  dword ptr [bp+8]
	shld	  edx,eax,16 
	pop	bp
	ret	
_fixdiv	endp
	assume	cs:_TEXT
_lfixmuldiv	proc	near
	push	bp
	mov	bp,sp
	mov	   eax,dword ptr [bp+4]
	imul	  dword ptr [bp+8]
	idiv	  dword ptr [bp+12]
	shld	  edx,eax,16 
	pop	bp
	ret	
_lfixmuldiv	endp
	assume	cs:_TEXT
_fixmulsumdiv	proc	near
	push	bp
	mov	bp,sp
	mov	   eax,dword ptr [bp+4]
	imul	  dword ptr [bp+8]
	mov	   ebx,eax
	mov	   ecx,edx    
	mov	   eax,dword ptr [bp+12]
	imul	  dword ptr [bp+16]
	add	   eax,ebx
	adc	   edx,ecx    
	idiv	  dword ptr [bp+20]
	shld	  edx,eax,16 
	pop	bp
	ret	
_fixmulsumdiv	endp
	assume	cs:_TEXT
_MoveDword	proc	near
	push	bp
	mov	bp,sp
	push	si
	push	di
	cld	
	mov	   cx,[bp+12]
	mov	   di,[bp+10]
	mov	   es,[bp+8]
	mov	   si,[bp+6]
	push	  ds
	mov	   ds,[bp+4]
	rep   movsd	
	pop	   ds
	pop	di
	pop	si
	pop	bp
	ret	
_MoveDword	endp
	assume	cs:_TEXT
_SetDword	proc	near
	enter	4,0
	push	di
	mov	ax,word ptr [bp+6]
	mov	word ptr [bp-2],ax
	mov	ax,word ptr [bp+4]
	mov	word ptr [bp-4],ax
	mov	   es,[bp-2]
	mov	   edi,0
	mov	   di,[bp-4]
	mov	   ecx,0
	mov	   cx,[bp+8]
	mov	   ax,[bp+10]
	shl	   eax,16
	mov	   ax,[bp+10]
	cld	
	rep   stosd	
	pop	di
	leave	
	ret	
_SetDword	endp
	assume	cs:_TEXT
_Setmem	proc	near
	enter	4,0
	push	di
	mov	ax,word ptr [bp+6]
	mov	word ptr [bp-2],ax
	mov	ax,word ptr [bp+4]
	mov	word ptr [bp-4],ax
	mov	   es,[bp-2]
	mov	   di,[bp-4]
	mov	   cx,[bp+8]
	mov	   ax,[bp+10]
 	cld	
 	test	    di, 1
	je	short @8@338
	jcxz	short @8@450
 	stosb	
 	dec	 cx
@8@338:
 	shr	 cx, 1
 	rep stosw	
	jnc	short @8@450
 	stosb	
@8@450:
	pop	di
	leave	
	ret	
_Setmem	endp
_TEXT	ends
_DATA	segment word public use16 'DATA'
s@	label	byte
_DATA	ends
_TEXT	segment byte public use16 'CODE'
_TEXT	ends
	public	_New0Interrupt
	public	_Setmem
	public	_SetDword
	public	_MoveDword
	public	_fixmulsumdiv
	public	_lfixmuldiv
	public	_fixdiv
	public	_fixmul
_s@	equ	s@
	end
