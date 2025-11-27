# GTKAppmenu

GTKAppmenu is a GTKmm4 based Application Launcer Menu built for speed because [nwg-launcher](https://github.com/nwg-piotr/nwg-dock-hyprland) was too slow while searching hundreds of apps.

its built for and tested in hyprland but it should work for all gtk-layer-shell supporting wayland Window Managers.

<p>
  <img width="1920" height="1080" alt="Image" src="https://github.com/user-attachments/assets/8a78432b-0ff6-442f-b24f-6ecba415317e" />
</p>

## Dependencies:

Compiletime: `GTKmm4 gtk-layer-shell`

Runtime: `none`

## Building and Installing:

Build with make: `make clean && make -j $(nproc --ignore=2)`

Installation: `ln -s $(pwd)/GTKAppmenu $HOME/.local/bin`\
Or (if ~/.config/GTKAppmenu exists): `mv ./GTKAppmenu ~/.local/bin/`

## Usage:

`./GTKAppmenu`

There are two ways of using GTKAppmenu either leaving it in its Project Dir and linking to it\
or moving conf folder to ~/.config/GTKDock so one can use the executable anywhere

## Cutomization

You can edit the css in conf/style.css
If fundamental changes to the code are required feel free to edit and build your own version

You might want to use GTK_DEBUG=interactive to help with customization :)
