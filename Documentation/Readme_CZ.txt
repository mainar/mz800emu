

Pokud vam nevyhovuje spousteni programu pres MZ800.BAT (nelze jej umistit na
hlavni listu ve Win7), tak muzete nahrat obsah ./runtime/gtk3 a ./runtame/sdl2 do 
adresare ./ ve kterem se nachazi MZ800EMU.EXE - potom bude mozne spoustet emulator rovnou, 
bez nutnostni nastaveni paths z BAT

Styly
=====

V adresari ./ui_resource naleznete soubory .glade a mz800emu.css, pomoci ktereho je mozne 
zmenit kompletni styly vsech GTK+ menu a oken.

Glade3: https://glade.gnome.org/
CSS: https://developer.gnome.org/gtk3/stable/GtkCssProvider.html#GtkCssProvider.description


Rozlozeni klavesnice
====================

V zasade je stejne, jako v emulatoru Zdenka Adlera. Vetsina klaves je
mapovana tak, aby se nachazely v miste, kde je ocekavate i na Sharpu.

Sharp key		PC key
---------------------------------

GRAPH			CAPSLOCK
ALPHA			\
BLANK_KEY		~
ESC			ESC, nebo take END
INST			INSERT
DEL			BACKSPACE, nebo take DELETE

@			F6
\			F7
?			F8
LIBRA			F9
