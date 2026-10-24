# EXTREMELY EXPERIMENTAL FORK FOR THE NINTENDO Wii U

**This fork is based on the commit ``9778682`` (From  July 2, 2026) of the original [Lime](https://github.com/openfl/lime)**

This is a experimental fork to attempt to port Lime to the Wii U, based on [my fork of Lime for the Nintendo Switch.](https://github.com/Slushi-Github/lime-nx)

While I can start to consider it "functional", it's actually extremely unstable and difficult to maintain.
After many changes and a new version of [gx2gl](https://github.com/ExpansionPak/gx2gl) for this purpose, as well as experimenting with custom rendering methods, as opposed to the default ones in pure Lime (as in the Lime samples) or in OpenFL, I've managed to start rendering simple things through this whole experiment.

I don’t really consider this usable yet, and I don’t recommend trying it unless it’s for experimental purposes or to help with the project itself.

**A huge thanks** to [ExpansionPak](https://github.com/ExpansionPak) for creating [gx2gl](https://github.com/ExpansionPak/gx2gl), it’s literally the foundation of this entire project, allowing us to make the most of the existing OpenGL support in Lime, OpenFL, and, I hope, HaxeFlixel in the future, all while using the Wii U’s GX2 graphics API, without having to build an entire graphics backend from scratch. Without [gx2gl](https://github.com/ExpansionPak/gx2gl), this project wouldn’t be possible (and the same goes for the one that’s been on my mind for years, unlike [the one I managed to pull off with the Switch](https://github.com/Slushi-Github/lime-nx)).

## How to use

If you want try this experimental thing...

You need to install [Haxe](https://haxe.org/download) and [DevKitPro stuff](https://devkitpro.org/wiki/Getting_Started)

Once you have Haxe and DevKitPro with DevKitPPC installed, install the dependencies:

(If you are on Linux/macOS, you will most likely need to use `sudo dkp-pacman` instead of `pacman`)

```bash
pacman -S --needed 
ppc-bzip2 
wiiu-cmake 
wiiu-curl 
ppc-flac 
ppc-freetype 
ppc-glm 
ppc-harfbuzz 
ppc-libjpeg-turbo 
ppc-libmodplug 
ppc-libogg 
ppc-libopus 
ppc-libpng 
ppc-libvorbis 
ppc-libvorbisidec 
ppc-libwebp 
ppc-mpg123 
ppc-opusfile 
wiiu-pkg-config 
wiiu-sdl2 
wiiu-sdl2_gfx 
wiiu-sdl2_image 
wiiu-sdl2_mixer 
wiiu-sdl2_net 
wiiu-sdl2_ttf 
wut-tools 
ppc-zlib
```

Then just install this fork with:

```bash
haxelib git lime https://github.com/Slushi-Github/lime-cafe.git
```

install the dependencies for Lime:

```bash
haxelib install format
haxelib install hxp
```

And my fork of hxcpp:

```bash
haxelib git hxcpp https://github.com/Slushi-Github/hxcpp-cafe.git
```

And and generate your Lime library, this also will compile the gx2gl library:

```bash
haxelib run lime rebuild wiiu
```

For now, you must put this in your `project.xml`, otherwise your program will crash when you open it:

```xml
<haxedef name="lime-opengl" if="wiiu" />
<haxedef name="lime-cairo" value="false" if="wiiu" />
<set name="LIME_CAIRO" value="0" if="wiiu" />
<set name="LIME_OPENGL" value="1" if="wiiu" />
<undefine name="lime-openal" if="wiiu" />
```

and it is also advisable to include this:

```xml
<!--Wii U specific-->
<window if="wiiu" orientation="landscape" fullscreen="true" width="0" height="0" resizable="false" hardware="true" />
```

And now you can compile your project!:

```bash
haxelib run lime build wiiu
```

For use the run command, you need to add this to your `project.xml`:

```xml
<config:wiiu ip="192.168.x.x" if="wiiu"/>
```

or use:

```bash
haxelib run lime run wiiu --ip=192.168.x.x
```

For add more libs (which must be installed in DevKitPro) to the MakeFile (the one responsible for generating the final executable) you need to add this to your `project.xml`:

```xml
<config:wiiu libs="yourLib1, yourLib2" if="wiiu"/>
```

----

License
=======

Lime is free, open-source software under the [MIT license](LICENSE.md).