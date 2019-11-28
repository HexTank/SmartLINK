;---------------------------------------------------------------------------------------------
;
; SmartLINK Snap loader for Retroleum's SmartCard by Paul Tankard - 2018
;
;
; pasmo --alocal smartcard_rom.asm smartcard.rom
;---------------------------------------------------------------------------------------------

rom_select_port     equ $fafb
sram_bank_port      equ $faf3
spi_control_port    equ sram_bank_port
spi_data_port       equ $faf7
card_cs_bit         equ 6
sinclair_rom_bank   equ 14
switch_out_rom      equ 0
screen_mem          equ $4000

sram_loc            equ $2000
sram_stack          equ sram_loc+$1000
comm_stack          equ sram_loc+$1800

;---------------------------------------------------------------------------------------------

        org     $0
        jp      start

;---------------------------------------------------------------------------------------------
	
        org     $20
        jp      intr

;---------------------------------------------------------------------------------------------
	
        org     $38
        ei                          ; to allow IRQ to occur prior to snapshot restarting
        reti

;---------------------------------------------------------------------------------------------
	
        org     $72
        retn
        retn
        retn

;--------- RESTORE REGISTERS AND RESTART 48K SNAPSHOT ----------------------------------------

        org     $100                ; ensures PC [7:0] < $72 until the switch point 

;---------------------------------------------------------------------------------------------
;
;
;
;
;
;---------------------------------------------------------------------------------------------

restart_snapshot:
        if switch_out_rom
                ld      bc,rom_select_port  ; set the swap-ROM bit (waits for read from addr $xx72)
                in      a,(c)
                or      $40
                out     (c),a
        else        
                ; push the following opcode to the begining of video memory so when we execute we patch out our ROM.
                ld      hl,rst_begin
	            ld      de,screen_mem
	            ld      bc,rst_end-rst_begin
	            ldir
                ; store of the value needed to switch off sram
                ld      bc,sram_bank_port
                in      a,(c)
                and     $70
                ld      (rst_end),a
        endif
        ei                          ; wait until just after a Spectrum frame IRQ before restarting snapshot  
        halt                        ; (to absorb any pending IRQ)
        di
       
        ld      a,(sna_header)      ; I reg
        ld      i,a
        ld      sp,sna_header+1     ; HL',DE',BC',AF'
        pop     hl
        pop     de
        pop     bc
        pop     af
        exx
        ex      af,af'
        pop     hl                  ; HL,DE,BC,IY,IX
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ld      a,(sna_header+20)   ; R reg
        ld      r,a
        ld      a,(sna_header+25)   ; interrupt mode: 0, 1, or 2 (already in IM 1)
        or      a
        jr      nz,not_im0b
        im      0                   ; set IM 0
not_im0b
        cp      2
        jr      nz,not_im2b
        im      2                   ; set IM 2
not_im2b
        ld      a,(sna_header+26)			
        and     7
        out     (254),a             ; set border colour
        
        ld      a,(sna_header+19)   ; Interrupt (bit 2 contains IFF2, 1=EI/0=DI)
        bit     2,a                 ; Start without final EI if IFF2 bit clear
        jr      z,irq_offb
        ld      sp,sna_header+21    ; AF reg
        pop     af
        ld      sp,(sna_header+23)  ; SP reg		
        if switch_out_rom
                ei                          ; Enable interrupts before restart
                jp      $72                 ; restart program with RETN @ $72 (switches to Spectrum ROM)			
        else
                jr      rdy
        endif
irq_offb
        if !switch_out_rom
            ld      a,$f3
            ld      (rst_ei_val - rst_begin + screen_mem),a    ; di
        endif

        ld      sp,sna_header+21    ; AF reg
        pop     af
        ld      sp,(sna_header+23)  ; SP reg		
        if switch_out_rom
                jp      $72                 ; restart program with RETN @ $72 (switches to Spectrum ROM)			
        else
rdy:    
                ; swapping out to  real spectrum ROM mean we can no longer page in, so try and use a mirror of the spectrum ROM
                ; and copy some restoration code at the start of vmem to put the snapshot in the state it expects before execution.
                ld      ((rst_bc_val - rst_begin) + screen_mem), bc
                ld      ((rst_a_val - rst_begin) + screen_mem), a
                ld      bc,sram_bank_port
                ld      a,(rst_end)
                out     (c),a
                ld      bc,rom_select_port
                ld      a,sinclair_rom_bank
                jp      $4000
        endif
    
;---------------------------------------------------------------------------------------------
;
;
;
;
;
;---------------------------------------------------------------------------------------------
start:		
        di
        im      1
        ld      a,$3f
        ld      i,a
        ld      sp, sram_stack
        
        ld      bc, sram_bank_port
        ld      a,$80
        out     (c),a               ; enable SRAM at $2000, use page 0
        
        call    show_logo
        
        call    init_scroll
        im      1
        ei
_lop:   halt

        call    update_scroll
        call    intr
        jr      _lop


;---------------------------------------------------------------------------------------------
;
;
;
;
;
;---------------------------------------------------------------------------------------------

read_packet_pointers:
        call    spi_read_write
        ld      l,a
        call    spi_read_write
        ld      h,a
        call    spi_read_write
        ld      e,a
        call    spi_read_write
        ld      d,a
        ret



intr:
        push    af
        push    bc
        ld      bc,sram_bank_port
        in      a,(c)
        push    af                  ; save current sram settings so we can restore later
        ld      a,$80
        out 	(c),a               ; enable SRAM at $2000, use page 0
        ld      (saved_sp),sp
        ld      sp,comm_stack        
        
        call    spi_on
        ld      d,0
_redo:  push    de

        call    request_work
_have_response:
        call    spi_read_write
        cp      $ff
        jr      nz, _ndone
        pop     de
        jp      _done
_ndone:
        cp      $fe
        jr      nz, _have_response
        
        call    spi_read_write
        cp      $a0
        jr      nz, _not_reg_xfer
        ; do reg xfer
        call    read_packet_pointers
        ld      hl, sna_header
        call    bulk_read_from_spi
        call    spi_read_write
        call    spi_read_write
        call    spi_read_write
        call    ack_work
        pop     de
        ld      d,1
        jr      _done

_not_reg_xfer:
        cp      $aa
        jr      nz, _not_bulk_xfer
        ; do bulk xfer
        call    read_packet_pointers
        call    bulk_read_from_spi
        call    spi_read_write
        call    spi_read_write
        call    spi_read_write
        call    ack_work
        pop     de
        jr		_done

_not_bulk_xfer:
        cp      $80
        jr      nz, _not_start_game
        call    read_packet_pointers
        call    ack_work
        call    spi_read_write
        call    spi_read_write
        call    spi_read_write
        jp      restart_snapshot
_not_start_game:
        pop     de

_done:	
        ld      a,d
        or      a
        jp      nz,_redo
        
        call    spi_off

        ld      sp,(saved_sp)
        ld      bc, sram_bank_port
        pop     af                  ; restore sram settings
        out     (c),a               ; enable SRAM at $2000, use page 0
        pop     bc
        pop     af
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
        set     card_cs_bit,a           ; set SD SPI_CS
        out     (c),a
        pop     bc
        pop     af	
        ret
	
spi_off:
        push    af
        push    bc
        ld      bc,spi_control_port
        in      a,(c)
        res     card_cs_bit,a           ; reset SD SPI_CS
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
        push    af
        push    bc					
        ld      bc,spi_data_port	
        
        out     (c),a 				
        dec     de					
        inc     d					
        inc     e					
_loop   ini 				
        inc     b 	
        out     (c),a
        dec     e					
        jp      nz,_loop			
        
        dec     d					
        jp      nz,_loop			
        
        ini							
        pop     bc					
        pop     af
        ret

request_work:
        ld      hl, sd_req
        ld      b,14
        jr      work_loop
ack_work:
        ld      hl, sd_ack
        ld      b,14
work_loop:
        ld      a,(hl)
        inc     hl
        call    spi_read_write
        djnz    work_loop
        ; bytes have now been transmitted to the client (if it's there) so we need to flush the last ack packet,
        ; the 0 response packet then the 0xfe packet should arrive, else all the packets will be 0xff and we will discard.
        call    spi_read_write
        call    spi_read_write		
        ret

show_logo:
        push    af
        push    bc
        push    hl
        push    de
        ld      de,0x4000
        ld      hl,slink_image
        call    zx7_decode
        pop     de
        pop     hl
        pop     bc
        pop     af
        ret

;---------------------------------------------------------------------------------------------
;
;
;
;
;
;---------------------------------------------------------------------------------------------
init_scroll:
        ld      hl,message-1        ; set to refresh char on first call
        ld      (scroll_pos),hl
        ld      hl,pix_count        ; variable to check if new character needed
        ld      (hl),1
        ret
	
update_scroll:
        ld      hl,pix_count        ; update pixel count
        dec     (hl)
        jr      nz,scroll
	
new_char:
        ld      (hl),8              ; reset pixel count
        
        ld      hl,(scroll_pos)     ; update current character
        inc     hl
        ld      (scroll_pos),hl
        ld      a,(hl)              ; check for loop token
        or      a
        jr      nz,get_glyph
	
loop_msg:
        ld      hl,message          ; loop if necessary
        ld      (scroll_pos),hl
	
get_glyph:
        ld      l,(hl)              ; collect letter to print in ascii
        ld      h,0                 ; convert to offset within font data
        add     hl,hl
        add     hl,hl
        add     hl,hl
        ld      de,font-(32*8)      ; base address for font within ROM
        add     hl,de               ; hl=> this letter's font data
        
        ld      de,tempchar         ; move font data to tempchar space
        ld      bc,8
        ldir
	
scroll:
        ld      hl,050FFh           ; top-right of print area
        ld      de,tempchar
        ld      c,8                 ; loop to scroll 8 rows
	
nextrow:
        ex      de,hl               ; scroll tempchar area left one pix, keep leftmost bit in carry
        rl      (hl)
        ex      de,hl
        push    hl
        
        ld b,   32
scrollrow:
        rl      (hl)                ; scroll each byte left, from right to left of screen.
        dec     l                   ; NB rightmost byte scrolls tempchar carry data onto screen first time through
        djnz    scrollrow		
        
        pop     hl                  ; next raster line down
        inc     h
        inc     de                  ; find next tempchar data position
        dec     c
        jr      nz,nextrow
        
        ret

;---------------------------------------------------------------------------------------------
;
;---------------------------------------------------------------------------------------------
zx7_decode:
                include "dzx7_mega.asm"

;---------------------------------------------------------------------------------------------
; ROM data
;---------------------------------------------------------------------------------------------
message:        db      "SmartLINK Loader V0.3....   ",0
font:           incbin  "aquaplane.fnt"
slink_image:    incbin  "smartlink.scr.zx7"

; --------------------------------------------------------------------------------------------
; -- Code to be placed at the top of vmem for restoring snapshot registers before execution
; --------------------------------------------------------------------------------------------

rst_begin:      db      $ed, $79    ; out   (c),a
                db      $01         ; ld    bc
rst_bc_val:     db      $00, $00    ;         ,xxxx
                db      $3e         ; ld    a,
rst_a_val:      db      $00         ;         xx
rst_ei_val:     db      $fb         ; ei
                db      $ed, $45    ; retn
rst_end:

; --------------------------------------------------------------------------------------------

sd_req:         db      $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$7f,$00,$00,$00,$00,$33     ; first 8 bytes is data flush, 6 bytes after is actual payload
sd_ack:         db      $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$7f,$01,$00,$00,$00,$35     ; first 8 bytes is data flush, 6 bytes after is actual payload

; --------------------------------------------------------------------------------------------
; SRAM data
; --------------------------------------------------------------------------------------------

org sram_loc:
                org     ($+255) & $ff00

saved_sp:       dw      0
        	
sna_header:     ds      32,0        ;this MUST be located at start of a page so that there's no Read @ $xx72
tempchar:       ds      8,0
scroll_pos:     dw      0,0
pix_count:      db      0,0
sram_restore:   db      $00         ; intentionally after rst_end - used to store byte needed to disable sram without affecting flags later on.

; --------------------------------------------------------------------------------------------

                org     $3fff
                db      0           ;pad to end of ROM