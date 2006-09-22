           processor 6502
           include "vcs.h"
           include "macro.h"

           SEG
           ORG $F000

Reset
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
	lda #%10101010
	sta GRP0
	lda #$FF
	sta COLUP0
               sta WSYNC
           
; begin scanlines

		SLEEP 21
		sta RESP0
		nop
		sta WSYNC

		SLEEP 22
		sta RESP0
		nop
		sta WSYNC

		SLEEP 23
		sta RESP0
		nop
		sta WSYNC

		SLEEP 24
		sta RESP0
		nop
		sta WSYNC

	REPEAT 95
		SLEEP 25
		sta RESP0
		nop
		sta WSYNC
	REPEND

	lda #$0
	sta GRP0

	REPEAT 92
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


           ORG $FFFA

           .word Reset          ; NMI
           .word Reset          ; RESET
           .word Reset          ; IRQ
    END
