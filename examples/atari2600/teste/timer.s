           processor 6502
           include "vcs.h"
           include "macro.h"

           SEG
           ORG $F000

Reset
	
	ldx #$0
	lda #$4
	sta TIM64T
Proximo
	inx
	lda INTIM
	bne Proximo
	jmp Reset


           ORG $FFFA

           .word Reset          ; NMI
           .word Reset          ; RESET
           .word Reset          ; IRQ
    END
