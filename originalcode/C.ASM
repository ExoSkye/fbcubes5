;CUBES5 by Ken Silverman (http://advsys.net/ken)
.386P

EXTRN _hind : near
EXTRN _ui : near
EXTRN _vi : near
EXTRN _pinc : near

CODE SEGMENT PUBLIC USE32 'CODE'
ASSUME cs:CODE

ALIGN 16
PUBLIC setupdline_
setupdline_:
	mov dword ptr ds:[dmach1+2], eax         ;modify bufplc
	mov dword ptr ds:[dmach2+2], ebx         ;modify palookup
	mov dword ptr ds:[dmach4+2], eax         ;modify bufplc
	mov dword ptr ds:[dmach5+2], ebx         ;modify palookup
	mov dword ptr ds:[dmask1+2], ecx
	mov dword ptr ds:[dmask2+2], ecx
	ret

	;   DLINE OPERATION:
	;for(;sx<end;sx++)
	;{
	;   drawpixel(p,dapal[gbuf[((u>>16)&0x3f00)+((v>>24)&0x3f)]]);
	;   i = hind[sx]; u += ui[i]; v += vi[i]; p += pinc[i];
	;}
ALIGN 16
PUBLIC dline_
dline_:
	push ebp

		;Move shade from dl to dh and clear rest of edx
	mov ebp, 000000ffh
	sub esi, eax            ;(This line belongs to section below)
	and edx, ebp
	lea eax, _hind[eax]+1   ;(This line belongs to section below)
	mov dh, dl

		;modify hind / inc-jnz
	mov dword ptr ds:[dmach3+2], eax
	mov al, byte ptr [esi+eax-1]

		;setup for add-adc, add-adc
	rol ebx, 8
	and eax, ebp            ;(This line belongs to section above)
	rol ecx, 8
	mov ebp, ebx
	mov bh, bl
	mov bl, cl
dmask1: and ebx, 00003f3fh

	jmp dbeginloop

ALIGN 16
dbeginloop:
dmach1: mov dl, byte ptr [ebx+88888888h]   ;bufplc
	add ebp, dword ptr _ui[eax]
	adc bh, byte ptr _ui[eax+4]
	add ecx, dword ptr _vi[eax]
	adc bl, byte ptr _vi[eax+4]
dmach2: mov cl, byte ptr [edx+88888888h]   ;palookup
dmask2: and ebx, 00003f3fh
	mov byte ptr [edi], cl
	add edi, dword ptr _pinc[eax]
dmach3: mov al, byte ptr [esi+88888888h]   ;hind-(end-esi)
	inc esi
	jnz dbeginloop

dmach4: mov dl, byte ptr [ebx+88888888h]   ;bufplc
dmach5: mov cl, byte ptr [edx+88888888h]   ;palookup
	mov byte ptr [edi], cl

	pop ebp
	ret

ALIGN 16
PUBLIC setupdline24_
setupdline24_:
	mov dword ptr ds:[dmach124+2], eax         ;modify bufplc
	mov dword ptr ds:[dmach624+2], ebx         ;modify colookup
	mov dword ptr ds:[dmach424+2], eax         ;modify bufplc
	mov dword ptr ds:[dmask124+2], ecx
	mov dword ptr ds:[dmask224+2], ecx
	ret

ALIGN 16
PUBLIC dline24_
dline24_:
	push ebp

		;Move shade from dl to dh and clear rest of edx
	mov ebp, 000000ffh
	sub esi, eax            ;(This line belongs to section below)
	and edx, ebp
	lea eax, _hind[eax]+1   ;(This line belongs to section below)
	shl edx, 10
dmach624: add edx, 88888888h          ;colookup
	mov dword ptr ds:[dmach224+3], edx
	mov dword ptr ds:[dmach524+3], edx

		;modify hind / inc-jnz
	mov dword ptr ds:[dmach324+2], eax
	mov al, byte ptr [esi+eax-1]

		;setup for add-adc, add-adc
	rol ebx, 8
	and eax, ebp            ;(This line belongs to section above)
	rol ecx, 8
	mov ebp, ebx
	mov bh, bl
	mov bl, cl
dmask124: and ebx, 00003f3fh

	jmp dbeginloop24

ALIGN 16
dbeginloop24:
	xor edx, edx
dmach124: mov dl, byte ptr [ebx+88888888h]   ;bufplc
	add ebp, dword ptr _ui[eax]
	adc bh, byte ptr _ui[eax+4]
	add ecx, dword ptr _vi[eax]
	adc bl, byte ptr _vi[eax+4]
dmach224: mov edx, dword ptr [edx*4+88888888h] ;colookup
dmask224: and ebx, 00003f3fh
	mov dword ptr [edi], edx
	add edi, dword ptr _pinc[eax]
dmach324: mov al, byte ptr [esi+88888888h]   ;hind-(end-esi)
	inc esi
	jnz dbeginloop24

	xor edx, edx
dmach424: mov dl, byte ptr [ebx+88888888h]   ;bufplc
dmach524: mov edx, dword ptr [edx*4+88888888h]   ;colookup
	mov dword ptr [edi], edx

	pop ebp
	ret

CODE ENDS
END
