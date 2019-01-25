;---------------------------------------------------------------------------------------------
;
; SmartLINK Snap loader for Retroleum's SmartCard by Paul Tankard - 2018
;
;
; pasmo --alocal smartcard_rom.asm smartcard.rom
;---------------------------------------------------------------------------------------------

rom_select_port		equ $fafb
sram_bank_port		equ $faf3
spi_control_port	equ sram_bank_port
spi_data_port		equ $faf7
card_cs_bit			equ 6

sram_loc			equ $2000
sram_stack			equ sram_loc+$1f40	;location chosen so that PC [7:0] < $xx72 (Re: IRQ prior to snapshot restart)

;---------------------------------------------------------------------------------------------

		org		$0
		jp		start

;---------------------------------------------------------------------------------------------
	
		org		$38
		ei							;to allow IRQ to occur prior to snapshot restarting
		reti

;---------------------------------------------------------------------------------------------
	
		org		$72
		retn
		retn
		retn

;--------- RESTORE REGISTERS AND RESTART 48K SNAPSHOT ----------------------------------------

		org 	$100				; ensures PC [7:0] < $72 until the switch point 

;---------------------------------------------------------------------------------------------
;
;
;
;
;
;---------------------------------------------------------------------------------------------
restart_snapshot:
		ld		bc,rom_select_port	; set the swap-ROM bit (waits for read from addr $xx72)
		in		a,(c)
		or		$40
		out		(c),a
	
		ei							; wait until just after a Spectrum frame IRQ before restarting snapshot  
		halt						; (to absorb any pending IRQ)
		di
	
		ld		a,(sna_header)		; I reg
		ld		i,a
		ld		sp,sna_header+1		; HL',DE',BC',AF'
		pop		hl
		pop		de
		pop		bc
		pop		af
		exx
		ex		af,af'
		pop		hl					; HL,DE,BC,IY,IX
		pop		de
		pop		bc
		pop		iy
		pop		ix
		ld		a,(sna_header+20)	; R reg
		ld		r,a
		ld		a,(sna_header+25)	; interrupt mode: 0, 1, or 2 (already in IM 1)
		or		a
		jr		nz,not_im0b
		im		0					; set IM 0
not_im0b
		cp		2
		jr		nz,not_im2b
		im		2					; set IM 2
not_im2b
		ld		a,(sna_header+26)			
		and		7
		out		(254),a				; set border colour

		ld		a,(sna_header+19)	; Interrupt (bit 2 contains IFF2, 1=EI/0=DI)
		bit		2,a					; Start without final EI if IFF2 bit clear
		jr		z,irq_offb
		ld		sp,sna_header+21	; AF reg
		pop		af
		ld		sp,(sna_header+23)	; SP reg		
		ei							; Enable interrupts before restart
		jp		$72					; restart program with RETN @ $72 (switches to Spectrum ROM)			

irq_offb	

		ld		sp,sna_header+21	; AF reg
		pop		af
		ld		sp,(sna_header+23)	; SP reg		
		jp		$72					; restart program with RETN @ $72 (switches to Spectrum ROM)	



;---------------------------------------------------------------------------------------------
;
;
;
;
;
;---------------------------------------------------------------------------------------------
start:		
		di
		im		1
    	ld		a,$3f
    	ld		i,a
		ld		sp, sram_stack
	
		ld		bc, sram_bank_port
  		ld		a,$80
		out		(c),a				; enable SRAM at $2000, use page 0

		ld		a,1
		out 	(254), a
		call 	show_logo
		ld 		a,6
		out 	(254), a

		call	init_scroll
		im		 1
		ei
_lop:	halt

		call	update_scroll
		call	_intr
		jr		_lop


;---------------------------------------------------------------------------------------------
;
;
;
;
;
;---------------------------------------------------------------------------------------------
_intr:
		push	hl
		push	de
		push	af
		push	bc

		ld		a,0
		out		(254),a
		call	spi_on

		call	request_work
_have_response:
		and		7
		out		(254),a
		call 	spi_read_write
		cp		$ff
		jr		nz, _ndone
		jp		_done
_ndone:
		cp		$fe
		jr		nz, _have_response






		ld		a,2
		out		(254),a

		call 	spi_read_write
		cp		$a0
		jr		nz, _not_reg_xfer
	
		; do reg xfer
		ld		a,4
		out		(254),a
		call	spi_read_write
		ld		l,a
		call	spi_read_write
		ld		h,a
		call	spi_read_write
		ld		e,a
		call	spi_read_write
		ld		d,a
		ld		hl, sna_header
		call	bulk_read_from_spi
		call	spi_read_write
		call	spi_read_write
		call	spi_read_write
		call	ack_work
		jr 		_done
_not_reg_xfer:

		cp		$aa
		jr		nz, _not_bulk_xfer
	
		; do bulk xfer
		ld		a,5
		out		(254),a
		call	spi_read_write
		ld		l,a
		call	spi_read_write
		ld		h,a
		call	spi_read_write
		ld		e,a
		call	spi_read_write
		ld		d,a
		call	bulk_read_from_spi
		call	spi_read_write
		call	spi_read_write
		call	spi_read_write
		call	ack_work
		jr		_done
_not_bulk_xfer:

		cp		$80
		jr		nz, _done	; not start game

		ld		a,6
		out		(254),a
		call	spi_read_write
		ld		l,a
		call	spi_read_write
		ld		h,a
		call	spi_read_write
		ld		e,a
		call	spi_read_write
		ld		d,a
		call	ack_work
		call	spi_read_write
		call	spi_read_write
		call	spi_read_write
		jp		restart_snapshot

_done:	
		ld		a,7
		out		(254),a
		call	spi_off
	
		pop		bc
		pop		af
		pop		de
		pop		hl
		ret

;---------------------------------------------------------------------------------------------
;
;
;
;
;
;---------------------------------------------------------------------------------------------	
spi_on:
		push    af
		push    bc
		ld      bc,spi_control_port
		in      a,(c)
		set     card_cs_bit,a			; set SD SPI_CS
		out     (c),a
		pop     bc
		pop     af	
		ret
	
spi_off:
		push    af
		push    bc
		ld      bc,spi_control_port
		in      a,(c)
		res     card_cs_bit,a			; reset SD SPI_CS
		out     (c),a
		pop     bc
		pop     af	
		ret
	
spi_read_write:
		push    bc
		ld      bc,spi_data_port
		out     (c),a
		nop
		nop
		nop
		in      a,(c)
		pop     bc
		ret

bulk_read_from_spi:
		push	af
		push	bc					
		ld		bc,spi_data_port	

		out		(c),a 				
		dec		de					
		inc		d					
		inc		e					
_loop	ini 				
		inc		b 	
		and     7				
		out		(c),a
		out     (254),a 				
		dec		e					
		jp		nz,_loop			

		dec		d					
		jp		nz,_loop			

		ini							
		pop		bc					
		pop		af
		ret

request_work:
		ld		hl, sd_request_work
		ld		b,14
		jr		work_loop
ack_work:
		ld		hl, sd_ack
		ld		b,14
work_loop:
		ld		a,(hl)
		inc		hl
		call 	spi_read_write
		djnz	work_loop
		; bytes have now been transmitted to the client (if it's there) so we need to flush the last ack packet,
		; the 0 response packet then the 0xfe packet should arrive, else all the packets will be 0xff and we will discard.
		call 	spi_read_write
		call 	spi_read_write		
		ret

show_logo:
		push 	af
		push	bc
		push	hl
		push	de
		ld		de, 0x4000
		ld		hl, slink_image
		call	zx7_decode
		pop		de
		pop		hl
		pop		bc
		pop		af
		ret

;---------------------------------------------------------------------------------------------
;
;
;
;
;
;---------------------------------------------------------------------------------------------
init_scroll:
		ld hl,message-1		; set to refresh char on first call
		ld (scroll_pos),hl
		ld hl,pix_count		; variable to check if new character needed
		ld (hl),1
		ret
	
update_scroll:
		ld hl,pix_count		; update pixel count
		dec (hl)
		jr nz,scroll
	
new_char:
		ld (hl),8			; reset pixel count
	
		ld hl,(scroll_pos)	; update current character
		inc hl
		ld (scroll_pos),hl
		ld a,(hl)			; check for loop token
		or a
		jr nz,get_glyph
	
loop_msg:
		ld hl,message		; loop if necessary
		ld (scroll_pos),hl
	
get_glyph:
		ld l,(hl)			; collect letter to print in ascii
		ld h,0				; convert to offset within font data
		add hl,hl
		add hl,hl
		add hl,hl
		ld de,font-(32*8)	; base address for font within ROM
		add hl,de			; hl=> this letter's font data
	
		ld de,tempchar		; move font data to tempchar space
		ld bc,8
		ldir
	
scroll:
		ld hl,050FFh		; top-right of print area
		ld de,tempchar
		ld c,8				; loop to scroll 8 rows
	
nextrow:
		ex de,hl			; scroll tempchar area left one pix, keep leftmost bit in carry
		rl (hl)
		ex de,hl
		push hl
	
		ld b,32
scrollrow:
		rl (hl)				; scroll each byte left, from right to left of screen.
		dec l				; NB rightmost byte scrolls tempchar carry data onto screen first time through
		djnz scrollrow		
	
		pop hl				; next raster line down
		inc h
		inc de				; find next tempchar data position
		dec c
		jr nz,nextrow

		ret


;---------------------------------------------------------------------------------------------
;
;
;
;
;
;---------------------------------------------------------------------------------------------
message:
    	db  	"SmartLINK Loader....   ",0
font:
		incbin	"aquaplane.fnt"

zx7_decode:
		include "dzx7_mega.asm"

slink_image:
		incbin	"smartlink.scr.zx7"



sd_request_work:
		db		$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$7f,$00,$00,$00,$00,$33		; first 8 bytes is data flush, 6 bytes after is actual payload
sd_ack:		
		db		$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$7f,$01,$00,$00,$00,$35		; first 8 bytes is data flush, 6 bytes after is actual payload


org sram_loc:
		org 	($+255) & $ff00
	
sna_header:
		ds		32,0			;this MUST be located at start of a page so that there's no Read @ $xx72
tempchar:
		ds		8,0

	
scroll_pos: 
		dw 		0,0
pix_count:  
		db 		0,0


		org 	$3fff
		db 		0				;pad to end of ROM