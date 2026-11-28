;;;;;;;;;; Tiles

	.globl _level01_01_HSize
_level01_01_HSize:	dc.w	$1

	.globl _level01_01_VSize
_level01_01_VSize:	dc.w	$1

	.globl _level01_01
_level01_01:
; Uncompressed
	INCBIN "level01_01.bin"
