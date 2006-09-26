;
; Paddle test code for the Atari 2600 VCS
;
; By Joe Grand, jgrand@xxxxxxxxxxxxxx
;
; To be used in SCSIcide (http://www.mindspring.com/~jgrand/atari)
;
; All paddle specific code denoted by ** PADDLE **
;

        processor 6502
        include vcs.h

        SEG.U defines

YBOTTOM         = 20   ; Bottom-most line of playfield
YTOP            = 160  ; Top-most line of playfield



        SEG.U vars
        org $80         ; Allocate memory, PIA has 128 bytes of RAM ($80-$FF). Stack from $FF

verPosP0                ds      1       ; Vertical position of drivehead
pot0                    ds      1       ; ** PADDLE ** Player 0's paddle value

score                   ds      3       ; Player score (6 hex digits)
scorePtr0               ds      2       ; Pointer to score shapes
scorePtr1               ds      2
scorePtr2               ds      2
scorePtr3               ds      2
scorePtr4               ds      2
scorePtr5               ds      2

temp                    ds      1       ; Temporary variables for drawing the score
saveStack               ds      1
counter                 ds      1


        SEG code
        org $F000

Start
        SEI             ; Disable interrupts, if there are any.
        CLD             ; Clear BCD math bit.
        LDX  #$FF
        TXS             ; Set stack to beginning.

        ; Loop to clear memory and TIA registers
        LDA  #0
B1      STA  0,X
        DEX
        BNE  B1

        ; The above routine does not clear location 0, which is VSYNC

        JSR  GameInit           ;Initial variable and register setup
MainLoop
        JSR  VerticalBlank      ;Execute the vertical blank.
        JSR  CheckSwitches      ;Check console switches.
        JSR  GameCalc           ;Do calculations during Vblank
        JSR  DrawScreen         ;Draw the screen
        JSR  OverScan           ;Do more calculations during overscan
        JMP  MainLoop           ;Continue forever.


VerticalBlank  ;*********************** VERTICAL BLANK HANDLER
;
;Beginning of the frame - at the end of overscan.
;
        ; ** PADDLE **
        LDA  #$82       ; VB_DumpPots & VB_Enable (Thanks Eckhard!)
        STA  VBLANK     ; Discharge paddle potentiometer

        LDA  #2         ;VBLANK was set at the beginning of overscan.
        STA  WSYNC
        STA  WSYNC
        STA  WSYNC
        STA  VSYNC      ;Begin vertical sync
        STA  WSYNC      ;First line of VSYNC
        STA  WSYNC      ;Second line of VSYNC

        ;Set the timer to go off just before the end of vertical blank space
        ;37 scanlines * 76 cycles = 2888 cycles / 64 = approx. 44
        LDA  #44
        STA  TIM64T
;
; End the VSYNC period.
;
        LDA  #0
        STA  WSYNC      ; Third line of VSYNC.
        STA  VSYNC      ; (0)

        ; ** PADDLE **
        LDA  #$2
        STA  VBLANK     ; Start recharge of paddle capacitor

        RTS


CheckSwitches ;*************************** CONSOLE SWITCH HANDLER

        RTS

;
; Minimal game calculations, just to get the ball rolling.
;
GameCalc ;******************************* GAME CALCULATION ROUTINES

        LDA  #$30
        STA  CTRLPF             ;No Reflect,No Score,Low Priority,8 pixel-wide ball

        ; ** PADDLE **
        LDA  #0
        STA  pot0

        ;
        ; Set pointers to number data
        ; Score Score+1 Score+2
        ;
        LDX  #3-1
        LDY  #10
ScoreLoop
        LDA  score,X
        AND  #$0F               ; Mask it
        ASL                     ; Multiply by 8
        ASL
        ASL

        ADC  #<FontPage         ; Store the 2-byte pointer
        STA  scorePtr0,Y
        LDA  #0
        ADC  #>FontPage
        STA  scorePtr0+1,Y

        DEY
        DEY

        LDA  score,X
        AND  #$F0               ; Mask it
        LSR                     ; $00,$08,$10,$18,$20, ... ( / 8 = value)

        ADC  #<FontPage         ; Store the 2-byte pointer
        STA  scorePtr0,Y
        LDA  #0
        ADC  #>FontPage
        STA  scorePtr0+1,Y

        DEY
        DEY
        DEX
        BPL  ScoreLoop

        RTS


DrawScreen ;**************************** SCREEN DRAWING ROUTINES

        ;
        ; Initialize display variables.
        ;
        LDA  #$1E
        STA  COLUP0
        STA  COLUP1

        LDX  #3
        STX  NUSIZ0             ; Three copies of player, close together (for score)
        STX  NUSIZ1

WaitVBlank
        LDA  INTIM              ; Loop until timer is done - wait for the proper scanline
                                ; (i.e. somewhere in the last line of VBLANK)
        BNE  WaitVBlank         ; Whew! We're at the beginning of the frame
        LDA  #0
        STA  WSYNC              ; End the current scanline
        STA  HMOVE              ; Add horizontal motion to position sprites
        STA  VBLANK             ; End the VBLANK period with a zero, enable video
        STA  WSYNC              ; Wait 2 scanlines before drawing score
        STA  WSYNC

        LDX  #22
Pause1
        DEX
        BNE  Pause1
        STA  RESP0

        STA  WSYNC
        LDX  #22
Pause2
        DEX
        BNE  Pause2
        NOP
        STA  RESP1
        STA  HMCLR
        LDA  #%10100000         ; P1 position 2 less than P0
        STA  HMP1
        LDA  #%11000000
        STA  HMP0
        STA  WSYNC
        STA  HMOVE
        STA  WSYNC


ScanLoop ; ================================== SCANNING LOOP
;
; Display the scores - 6 digit, courtesy of Robin Harbron
;
        LDA  #8-1              ; Lines to display
        STA  counter

        LDX  #9
Pause3
        DEX
        BNE  Pause3
        NOP
        NOP
        NOP

        TSX
        STX  saveStack          ; Save the stack pointer (Andrew Davie)
DrawScore
        LDY  counter            ; 3
        LDA  (scorePtr0),Y      ; 5
        STA  GRP0               ; 3 = 11

        BIT  $00

        LDA  (scorePtr1),Y      ; 5
        STA  GRP1               ; 3
        LDA  (scorePtr5),Y      ; 5
        TAX                     ; 2
        TXS                     ; 2
        LDA  (scorePtr2),Y      ; 5
        STA  temp               ; 3
        LDA  (scorePtr3),Y      ; 5
        TAX                     ; 2
        LDA  (scorePtr4),Y      ; 5 = 37

        LDY  temp               ; 3
        STY  GRP0               ; 3
        STX  GRP1               ; 3
        STA  GRP0               ; 3
        TSX                     ; 2
        STX  GRP1               ; 3 = 17

        DEC  counter            ; 5
        BPL  DrawScore          ; 3 =  8

        LDX  saveStack
        TXS

        LDA  #0
        STA  GRP0
        STA  GRP1

        ;
        ; Skip down a few lines
        ; Do other calculations here if necessary, we have lots of time
        ;

        LDY  #6
DrawWait
        STA  WSYNC
        DEY
        BNE  DrawWait

        LDX  #0
        STX  NUSIZ0     ; One copy of player
        STX  NUSIZ1

        LDY  #178
Kernel
        STA  WSYNC

        ; ** PADDLE **
        LDA  INPT0      ; Read paddle 0
        BMI  Charged    ; Capacitor already charged, skip increment
        INC  pot0       ; Increment paddle position value
Charged

        ;Position drive head on proper Y axis
        CPY  verPosP0
        BNE  NotYet

        ; Draw sprite
        LDX  #8-1
Next
        LDA  Sprite,X   ;Get the shape
        STA  GRP0
        STA  WSYNC      ;Double height
        STA  WSYNC
        DEY
        DEY
        DEX
        BPL  Next

        INY
        INX
        STX  GRP0      ;Disable graphics

NotYet
        DEY
        BNE  Kernel

        LDA  #2
        STA  WSYNC      ; Finish this scanline.
        STA  VBLANK     ; Make TIA output invisible.

        RTS


OverScan   ;***************************** OVERSCAN CALCULATIONS
;
; For the Overscan routine, one might take the time to process such
; things as collisions.
;
        LDA  SWCHA              ; Has fire button been pressed?
        BMI  NoButton
        LDA  #$34               ; If yes, change color of background
        JMP  ColorDone
NoButton
        LDA  #$0
ColorDone
        STA  COLUBK             ; Background will be black.


        ; ** PADDLE **
        ;
        ; Set new vertical position of drivehead based on paddle value
        ;
        LDX  pot0
        STX  score+2            ; Show actual paddle value as score for debugging purposes

        ; ** PADDLE **
        ;
        ; Set bounds on paddle motion
        ;
        CPX  #YTOP              ; If pot0 > YTOP, set to YTOP
        BCC  PaddleCont
        LDX  #YTOP
        JMP  PaddleDone
PaddleCont
        CPX  #YBOTTOM           ; If pot0 < YBOTTOM, set to YBOTTOM
        BCS  PaddleDone
        LDX  #YBOTTOM
PaddleDone
        STX  verPosP0


        LDX  #30
KillLines
        STA  WSYNC
        DEX
        BNE  KillLines
        RTS


GameInit   ;***************************** INITIAL VARIABLE & REGISTER SETUP
        ;
        ; Clear data pointers
        ;
        LDX  #11
        LDA  #0
Clear2
        STA  scorePtr0,X
        DEX
        BPL  Clear2

        ;
        ; Initialize score value
        ;
        STA  score      ; A = 0
        STA  score+1
        STA  score+2

        RTS

;
; Must not cross page boundaries!!
; All shapes are upside down to allow decrementing by Y as both
; a counter and a shape index
;
        org $FE00 ; *********************** GRAPHICS DATA

;
; Numeric font
;
FontPage
Zero
                .byte   %00111100 ; |  XXXX  |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %00111100 ; |  XXXX  |

One
                .byte   %00111100 ; |  XXXX  |
                .byte   %00011000 ; |   XX   |
                .byte   %00011000 ; |   XX   |
                .byte   %00011000 ; |   XX   |
                .byte   %00011000 ; |   XX   |
                .byte   %00011000 ; |   XX   |
                .byte   %00111000 ; |  XXX   |
                .byte   %00011000 ; |   XX   |

Two
                .byte   %01111110 ; | XXXXXX |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %00111100 ; |  XXXX  |
                .byte   %00000110 ; |     XX |
                .byte   %00000110 ; |     XX |
                .byte   %01000110 ; | X   XX |
                .byte   %00111100 ; |  XXXX  |

Three
                .byte   %00111100 ; |  XXXX  |
                .byte   %01000110 ; | X   XX |
                .byte   %00000110 ; |     XX |
                .byte   %00001100 ; |    XX  |
                .byte   %00001100 ; |    XX  |
                .byte   %00000110 ; |     XX |
                .byte   %01000110 ; | X   XX |
                .byte   %00111100 ; |  XXXX  |

Four
                .byte   %00001100 ; |    XX  |
                .byte   %00001100 ; |    XX  |
                .byte   %00001100 ; |    XX  |
                .byte   %01111110 ; | XXXXXX |
                .byte   %01001100 ; | X  XX  |
                .byte   %00101100 ; |  X XX  |
                .byte   %00011100 ; |   XXX  |
                .byte   %00001100 ; |    XX  |

Five
                .byte   %01111100 ; | XXXXX  |
                .byte   %01000110 ; | X   XX |
                .byte   %00000110 ; |     XX |
                .byte   %00000110 ; |     XX |
                .byte   %01111100 ; | XXXXX  |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01111110 ; | XXXXXX |

Six
                .byte   %00111100 ; |  XXXX  |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01111100 ; | XXXXX  |
                .byte   %01100000 ; | XX     |
                .byte   %01100010 ; | XX   X |
                .byte   %00111100 ; |  XXXX  |

Seven
                .byte   %00011000 ; |   XX   |
                .byte   %00011000 ; |   XX   |
                .byte   %00011000 ; |   XX   |
                .byte   %00011000 ; |   XX   |
                .byte   %00001100 ; |    XX  |
                .byte   %00000110 ; |     XX |
                .byte   %01000010 ; | X    X |
                .byte   %01111110 ; | XXXXXX |

Eight
                .byte   %00111100 ; |  XXXX  |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %00111100 ; |  XXXX  |
                .byte   %00111100 ; |  XXXX  |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %00111100 ; |  XXXX  |

Nine
                .byte   %00111100 ; |  XXXX  |
                .byte   %01000110 ; | X   XX |
                .byte   %00000110 ; |     XX |
                .byte   %00111110 ; |  XXXXX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %00111100 ; |  XXXX  |

A
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01111110 ; | XXXXXX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %00111100 ; |  XXXX  |

B
                .byte   %01111100 ; | XXXXX  |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01111100 ; | XXXXX  |
                .byte   %01111100 ; | XXXXX  |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100110 ; | XX  XX |
                .byte   %01111100 ; | XXXXX  |

C
                .byte   %00111100 ; |  XXXX  |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01100110 ; | XX  XX |
                .byte   %00111100 ; |  XXXX  |

D
                .byte   %01111100 ; | XXXXX  |
                .byte   %01100110 ; | XX  XX |
                .byte   %01100010 ; | XX   X |
                .byte   %01100010 ; | XX   X |
                .byte   %01100010 ; | XX   X |
                .byte   %01100010 ; | XX   X |
                .byte   %01100110 ; | XX  XX |
                .byte   %01111100 ; | XXXXX  |

E
                .byte   %01111110 ; | XXXXXX |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01111110 ; | XXXXXX |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01111110 ; | XXXXXX |

F
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01111110 ; | XXXXXX |
                .byte   %01100000 ; | XX     |
                .byte   %01100000 ; | XX     |
                .byte   %01111110 ; | XXXXXX |

Sprite
                .byte  %01111110 ; | XXXXXX |
                .byte  %10000001 ; |X      X|
                .byte  %10111101 ; |X XXXX X|
                .byte  %10100101 ; |X X  X X|
                .byte  %10000001 ; |X      X|
                .byte  %10100101 ; |X X  X X|
                .byte  %01000010 ; | X    X |
                .byte  %00111100 ; |  XXXX  |


        .byte "J. Grand"


        org $FFFC

        .word  Start
        .word  Start

