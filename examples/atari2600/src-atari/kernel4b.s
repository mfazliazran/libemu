;Reflected playfield goodness v2


            processor 6502
            include "vcs.h"
            include "macro.h"


COUNTER1        = $80
COLOROFFSET     = $81
LINECOUNTER     = $82
LINESCOMPLT     = $83
GRAPHICCOUNT    = $84

TIMETOCHANGE    = 2                    ; speed of "animation" 
DATALINES       = 14
NUMLINES        = 8

            SEG
            ORG $F000

Reset
; Clear RAM and all TIA registers

                ldx #0
                lda #0
Clear           sta 0,x
                inx
                bne Clear

        ;------------------------------------------------
        ; Once-only initialisation...

        lda #0
        ;lda #$45
        sta COLUPF              ; set the playfield colour
        lda #%00000001 
        sta CTRLPF              ; reflect playfield 
        lda #$00
        sta COLUBK
        sta LINECOUNTER
        sta LINESCOMPLT
        sta GRAPHICCOUNT

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
           
                ldx #0
VerticalBlank   sta WSYNC
                inx
                cpx #36 ; Now waiting 36 lines
                bne VerticalBlank

                ;do some color offset calcs

                ldy COUNTER1
                cpy #TIMETOCHANGE
                bne notyet

                inc COLOROFFSET
                ldy COLOROFFSET
                lda #0
                sta COUNTER1
                

notyet
                inc COUNTER1
                ldy COLOROFFSET
                
                sta WSYNC  ;  wait for last line of vblank

DrawField       
                ldx #0                  ; this counts our scanline number 

                lda #$FF
                sta PF0 
                sta PF1 
                sta PF2 

Top             sta WSYNC
                sty COLUPF
                
                lda #0
                inx
                iny
                cpx #8
                bne Top
                
                lda #$10
                sta PF0 
                lda #0 
                sta PF1 
                sta PF2 

TopSides        sta WSYNC               
                sty COLUPF
                inx
                iny
                cpx #36
                bne TopSides
                
Face            stx LINESCOMPLT
                
OUTER           ldx GRAPHICCOUNT
                lda Graphic,X
                ldx #8
                sta PF2
INNER           sta WSYNC
                sty COLUPF
                iny 
                inc LINESCOMPLT
                dex
                cpx #0
                bne INNER
                inc GRAPHICCOUNT
                lda GRAPHICCOUNT
                cmp #7
                bne OUTER
                
                ;no color change mid face
MIDSMILE        ldx GRAPHICCOUNT
                lda Graphic,X  
                sta PF2
                ldx #8
INNERM          sta WSYNC
                sty COLUPF
                inc LINESCOMPLT
                dex
                cpx #0
                bne INNERM
                
                inc GRAPHICCOUNT
                
                ;reverse color change 
OUTER2          ldx GRAPHICCOUNT
                lda Graphic,X
                ldx #8
                sta PF2
INNER2          sta WSYNC
                sty COLUPF
                dey 
                inc LINESCOMPLT
                dex
                cpx #0
                bne INNER2
                inc GRAPHICCOUNT
                lda GRAPHICCOUNT
                cmp #15
                bne OUTER2
                
                ldx LINESCOMPLT                
                
                
                lda #$10         
                sta PF0 
                lda #0 
                sta PF1 
                sta PF2


BottomSides     sta WSYNC               
                sty COLUPF
                inx
                dey
                cpx #184
                bne BottomSides

                lda #$FF
                sta PF0 
                sta PF1 
                sta PF2 

Bottom          sta WSYNC               

                sty COLUPF
                inx
                dey
                cpx #192
                bne Bottom      


            lda #%01000010
            sta VBLANK                      ; end of screen - enter blanking

                ldx #0
                stx COLUPF
Overscan        sta WSYNC
                inx
                cpx #29
                bne Overscan
                
                lda #0
                sta LINECOUNTER
                sta LINESCOMPLT
                sta GRAPHICCOUNT
                sta WSYNC
                
                jmp StartOfFrame


Graphic  
        .byte #%11100000
        .byte #%11110000
        .byte #%11111000
        .byte #%11111100
        .byte #%11101110
        .byte #%11101111
        .byte #%11111111
        .byte #%11111011
        .byte #%11111011
        .byte #%11110111
        .byte #%11101110
        .byte #%11011100
        .byte #%00111000
        .byte #%11110000
        .byte #%11100000
        
        
            ORG $FFFA

            .word Reset           ; NMI
            .word Reset           ; RESET
            .word Reset           ; IRQ

       END
