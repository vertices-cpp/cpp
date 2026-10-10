MMM 0.3.4
by Kaneda


System requirement
------------------
A PC with Windows 98+ (It seems MMM got some problems under W98...I need to check it)
Bmp2Tile (see "Links")


Features
--------
- Create new map with Bmp2Tile's tle file
- Import existing MapMaker map
- Zoom
- Grid
- Make groups
- Flips
- Handle Field A, Field B and Sprites
- Export binary to include in your demos/games
- Export test rom of your map (w/o sprite)
- Export map preview
- Configure starting tile


To come
-------
- Export a test rom with sprites
- Integrated Bmp2Tile
- ... (contact me to ask me some features)


Known bugs
----------
- Some visual data corruption on W98 (to check)
- No cursor on resizing Window
- Unneeded cursor with Ctrl key on Sprite mode


How to use
----------
- Create a tle file with BMP2TILE
- Create a new map with these tiles
OR
- Import an existing map
- Make groups to easier edit your fields
- Edit Field A
- Edit Field B
- Export a test rom
- Play it on any emulator
- Add some sprites, 16 colors bitmap, 80x80 max
- Edit Sprites
- Export the map data (a binary file by field)
- Include it on your demo/games


Links
-----
Genny dev Home Page : http://www.consoledev.fr.st (MMM and Bmp2Tile home page)
Genny dev ring      : http://fvring.free.digitartstudio.com/index.php?main=cga&sub=lks&ssub=vew
Devega board        : http://www.emulationzone.org/projects/metalix/board/list.php?f=1
CMD Home Page       : http://cgfm2.emuviews.com/ (a MUST read)


Thanks
------
Deedo - to allow me to work on it ;)
TeT - for his gfxer point of view, remarks and tests
Fonzie - the main MMM user ;) Thanks for the useful reports

按*键选择整个区域
文件 -> 保存调色板 -> 在 ASM 中
文件 -> 保存 Tiles -> 在 ASM 中
文件 -> 导出 TLE -> 新建
打开 MMM，然后选择File -> New。将指标设置为 64×16，并导入BMP2Tile 导出的TLE 文件：
导出的TLE文件的第6与8个字节的FC都改为0