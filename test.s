; test.s
; Compile with:
;   xa test.s -v -bt 1536

;        ldx #50		; Use X index register as "counter"
; loop:  lda $a0,X		; Get a byte from Source + X
;        sta $b0,X		; Store it at Target + X
;        dex			; Count down one byte
;        ; bpl loop		; If not negative (-1, FFh), we're not done, repeat from label Loop

; ldx #32	; Load 32 into X
; inx		; Increment X
; stx $44	; Store X into address $44
; lda $44 	; Load the byte from address $ff into A

lda #32 ; Load 32 into A
sta $44 ; Store A into address $44
