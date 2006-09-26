           processor 6502
           include "vcs.h"
           include "macro.h"

	SEG.U Variables
	ORG $80

	 .word
GRHEIGHT .byte
TEMPVAR  .byte
GRTABLE  .word 

	SEG Code
	ORG $F000, 0

Reset

; Load object table
	lda #6
	sta GRHEIGHT
	
	lda #<Um
	sta GRTABLE
	lda #>Um
	sta GRTABLE+1
	

StartOfFrame

  ; Start of vertical blank processing

           lda #0
           sta VBLANK

           lda #2
           sta VSYNC
           
              ; 3 scanlines of VSYNCH signal...

               sta WSYNC
               sta WSYNC
               sta WSYNC

           lda #0
           sta VSYNC           

              ; 37 scanlines of vertical blank...

               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
          
; begin scanlines

;                     Cycles  Pixel    GRP0   GRP0A   GRP1   GRP1A

loop2
 ldy  GRHEIGHT        ;+3  63  189
 lda  (GRTABLE),y     ;+5  68  204
 sta  GRP0            ;+3  71  213      D1     --      --     --
 sta  WSYNC           ;go
 lda  (GRTABLE+$2),y  ;+5   5   15
 sta  GRP1            ;+3   8   24      D1     D1      D2     --
 lda  (GRTABLE+$4),y  ;+5  13   39
 sta  GRP0            ;+3  16   48      D3     D1      D2     D2
 lda  (GRTABLE+$6),y  ;+5  21   63
 sta  TEMPVAR         ;+3  24   72
 lda  (GRTABLE+$8),y  ;+5  29   87
 tax                  ;+2  31   93
 lda  (GRTABLE+$A),y  ;+5  36  108
 tay                  ;+2  38  114
 lda  TEMPVAR         ;+3  41  123              !
 sta  GRP1            ;+3  44  132      D3     D3      D4     D2!
 stx  GRP0            ;+3  47  141      D5     D3!     D4     D4
 sty  GRP1            ;+3  50  150      D5     D5      D6     D4!
 sta  GRP0            ;+3  53  159      D4*    D5!     D6     D6
 dec  GRHEIGHT        ;+5  58  174                             !
 bpl  loop2           ;+2  60  180


	REPEAT 192
		sta WSYNC
	REPEND

              ; 192 scanlines of picture...

;               ldx #0
;               REPEAT 192; scanlines

;                   inx
;                   stx COLUBK
;                   sta WSYNC

;               REPEND


           lda #%01000010
           sta VBLANK                     ; end of screen - enter blanking

              ; 30 scanlines of overscan...

               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
              sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
              sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC
               sta WSYNC

           jmp StartOfFrame

Um:
	.byte %00010000
	.byte %00010000
	.byte %00010000
	.byte %00010000
	.byte %00010000
Dois:
	.byte %00111000
	.byte %00001000
	.byte %00111000
	.byte %00100000
	.byte %00111000
Tres:
	.byte %00111000
	.byte %00001000
	.byte %00111000
	.byte %00001000
	.byte %00111000
Quatro:
	.byte %00101000
	.byte %00101000
	.byte %00111000
	.byte %00001000
	.byte %00001000
Cinco:
	.byte %00111000
	.byte %00100000
	.byte %00111000
	.byte %00001000
	.byte %00111000
Seis:
	.byte %00111000
	.byte %00100000
	.byte %00111000
	.byte %00101000
FIM	.byte %00111000

           ORG $FFFA

           .word Reset          ; NMI
           .word Reset          ; RESET
           .word Reset          ; IRQ
    END
