	processor 6502
	include vcs.h

	org $F000

Start
	lda #2     ; sync
	sta VSYNC 
	sta WSYNC
	sta WSYNC
	sta WSYNC

	lda #2      ;timer
	sta TIM64T

aha
	lda INTIM
	bne aha

	jmp Start

	org $FFFC
	.word Start
	.word Start
