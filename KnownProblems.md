# Introduction #

Nothing is perfect. Cuberok has some issues that known as unfixable or WM/DE/OS-specific. Here is some workarounds for them.


# Details #

## Floating toolbars and menus ##
Usually floating toolbars and menus works fine.<br />
<a href='http://f.imagehost.org/view/0665/cuberok_floating_menu'><img src='http://f.imagehost.org/t/0665/cuberok_floating_menu.jpg' alt='cuberok_floating_menu.png (115 KB)' border='0' width='150' height='125' /></a><br />
But there is a problem on kde 3.5 (may be, there is same problem on kde 4, I am not sure):
when cuberok window lost focus, floating menus and toolbars disapear and apear again when focus returns to cuberok window. Workaround for kde5.3 is quite easy.
  1. Give a focus to floating toolbar
  1. press Alt+F3. This will show the kwin menu for toolbar window.
  1. select "advanced" -> "special window settings" and go to "workarounds" tab
  1. check "window type" checkbox, select "force" in toolbox and "normal window in next toolbox"
  1. push "Ok" button
<a href='http://f.imagehost.org/view/0037/cuberok_floating_menu_properties'><img src='http://f.imagehost.org/t/0037/cuberok_floating_menu_properties.jpg' alt='cuberok_floating_menu_properties.png (38 KB)' border='0' width='150' height='67' /></a><br />
Toolbar will not disapear anymore.

Repeat these steps for floating menu window. Also, you can set "always on top" and "all desktops" properties.