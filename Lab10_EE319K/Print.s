; Print.s
; Student names: Zahid Hossain, Arjun Patel
; Runs on LM4F120 or TM4C123
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; ST7735_OutChar   outputs a single 8-bit ASCII character
; ST7735_OutString outputs a null-terminated string 

modResult EQU  0
outFixArray EQU 0
;5 char array = 5 bytes (4th byte = null)

    IMPORT   ST7735_OutChar
    IMPORT   ST7735_OutString
    EXPORT   LCD_OutDec
    EXPORT   LCD_OutFix

    AREA    |.text|, CODE, READONLY, ALIGN=2
    THUMB

  

;-----------------------LCD_OutDec-----------------------
; Output a 32-bit number in unsigned decimal format
; Input: R0 (call by value) 32-bit unsigned number
; Output: none
; Invariables: This function must not permanently modify registers R4 to R11
; Lab 7 requirement is for at least one local variable on the stack with symbolic binding
LCD_OutDec
     PUSH {R1-R3,LR}
     SUB  SP,#8

     CMP  R0,#10
     BHS continue
     ADD  R0,#0x30
     BL   ST7735_OutChar
     B return

continue
     MOV  R1,#10
     UDIV R2,R0,R1
     ;R2 contains result of integer division

     MLS R3,R2,R1,R0 ;R3=R0-R2*R1
     STR R3,[SP,#modResult]

     CMP R2,#0
     BEQ printChar
     MOV  R0,R2
     BL LCD_OutDec

printChar
     LDR  R0,[SP,#modResult]
     ADD  R0,#0x30
     BL   ST7735_OutChar

return
     ADD  SP,#8
     POP {R1-R3,PC}
;* * * * * * * * End of LCD_OutDec * * * * * * * *

; -----------------------LCD _OutFix----------------------
; Output characters to LCD display in fixed-point format
; unsigned decimal, resolution 0.01, range 0.00 to 9.99
; Inputs:  R0 is an unsigned 32-bit number
; Outputs: none
; E.g., R0=0,    then output "0.00 "
;       R0=3,    then output "0.003 "
;       R0=89,   then output "0.89 "
;       R0=123,  then output "1.23 "
;       R0=999,  then output "9.99 "
;       R0>999,  then output "*.** "
; Invariables: This function must not permanently modify registers R4 to R11
; Lab 7 requirement is for at least one local variable on the stack with symbolic binding
LCD_OutFix
     PUSH {R1-R3,LR}
     SUB SP,#8

     MOV  R1,#999
     CMP  R0,R1
     BHI  invalidFix

     MOV  R1,#0x2E ;period
     STRB  R1,[SP,#(outFixArray+1)]

     MOV  R1,#10

     UDIV R2,R0,R1
     MLS R3,R2,R1,R0
     ADD  R3,#0x30
     STRB R3,[SP,#(outFixArray+3)]
     MOV  R0,R2

     UDIV R2,R0,R1
     MLS R3,R2,R1,R0
     ADD  R3,#0x30
     STRB R3,[SP,#(outFixArray+2)]
     MOV  R0,R2

     UDIV R2,R0,R1
     MLS R3,R2,R1,R0
     ADD  R3,#0x30
     STRB R3,[SP,#(outFixArray)]

     MOV  R1,#0x00
     STRB  R1,[SP,#(outFixArray+4)]

     B outFixreturn

invalidFix
     MOV  R1,#0x2E ;period
     STRB  R1,[SP,#(outFixArray+1)]
     MOV  R1,#0x2A ;asterisk
     STRB  R1,[SP,#outFixArray]
     STRB  R1,[SP,#(outFixArray+2)]
     STRB  R1,[SP,#(outFixArray+3)]
     MOV  R1,#0x00
     STR  R1,[SP,#(outFixArray+4)]

outFixreturn
     ADD  R0,SP,#outFixArray
     BL   ST7735_OutString
     ADD SP,#8
     POP {R1-R3,LR}
     BX   LR
 
     ALIGN
;* * * * * * * * End of LCD_OutFix * * * * * * * *

     ALIGN          ; make sure the end of this section is aligned
     END            ; end of file
