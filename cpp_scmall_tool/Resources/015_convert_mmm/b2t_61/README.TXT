                /////////////////////////
                //                     //
                //  BMP 2 TILE         //
                //                     //
                //          by Kaneda  //
                //                     //
                /////////////////////////

                version 6.0     21 August 2003


History
v2.0	first public release
v5.2	second public release, 32&16bits
	A lot of options and bugs fixes...in 3 years :)
v5.2b	Correct name for .BIN and .S
v6.0	Included packer from Charles MacDonald
	Save map
	Removed the first empty tile stuff


Some docs
---------
Well...BMP2TILE convert Bitmap to ASM/BIN sources to
include Tiles (and the pal) on Sega Genesis Demos.

Bitmap MUST be less than 320x200 and 16colors.

There are 2 kinds of tiles, MapTile which are coded horizontaly
and SpriteTile which are coded vertically.
You can select the sprite mode using the  -s  switch (or by menu).

The tiles are 8x8size ( 8 ulongs ).
The pal is a 16colors pal when the RGB is coded on 12bits.

If you want to load a tile as the first one, please don't forget Genesis
use the first tile to fill its 2 fields.

BMP2TILE use bitmap with 8multiple size...
If your bitmap isn't a multiple of 8, then pixel 0 are added at the end of the bitmap.
With the Right Alignment option, pixel 0 are added at the beggining of the bitmap.
You can know if you're in right alignment, see if you have a "R" at the end of the bitmap
size in the 'Image Info' dialog box.

Right Alignment can only be activated by command line (-r).

'Save Map' means 'Save Map def + Tiles'.
So 'Save Map Binary Optimized Compressed' means save a binary map without duplicates tiles and save its tiles compressed.

Please use SGCC3.0 (with maccer for INCBIN) else big bitmap will crash the linker.


Packer
------
Since version 6.0, Bmp2Tile included Charles MacDonald 's packer.
You can found the Genesis unpacker with the original source on his site.
(see below)


Steps
-----
01 - Load the bitmap
02 - BMP2TILE analyse the Bitmap to be sure it's a 16 colors bitmap.
03 - With the arrow you move the selector on the wanted zone
     You can 
     	increase the horizontal size of the selector by 8
     	decrease the horizontal size of the selector by 8
     	increase the vertical size of the selector by 8
     	decrease the vertical size of the selector by 8
     	change the color of the selector (index+1)
     	you change the color of the selector (index-1)
     	select all
04 - When your selector is sized, you can
        save the current tile (in asm/binary/binary compressed)
05 - You can resize the selector and made another stuff
06 - 'Esc' to quit BMP2TILE (or use the menu)
07 - Make a demo or a game
08 - Drop me a mail (URL of your dev included) to let me know if BMP2TILE was useful or/and what it lacks


Commands summary
----------------
'Esc'       Quit BMP2TILE
Arrow       Move the selector
'+'/'-'     Change the color of the selector
'r'         Increase HSize of the selector
't'         Decrease HSize of the selector
'f'         Increase VSize of the selector
'g'         Decrease VSize of the selector
'*'	    Max Size of the selector

See the menu for the others commands

When changing selector's size, if the size is more than 32 in Sprite mode,
multiple sprites are saved. (with each sprite in 32x32 max)


Known bugs
----------
Menu activated, EVEN when you cancel load
M accelerator key doesn't work on all systems  >:[

Thanks
------
Charles MacDonald
Charles Doty
Kevin Banks
Paul Lee
B. Eubanks
Eidolon
Scott Harner (for the BMP VSize bug report)


Links
-----
My little dev page :            http://www.consoledev.fr.st
My email :                      Kaneda@chez.com

Charles MacDonald :		http://cgfm2.emuviews.com
Eidolon's inn :			http://www.eidolons-inn.de
KBanks Dev page :               http://www.ro.com/~kbanks
Sega Dev Mailing list :      	http://groups.yahoo.com/group/segadev/