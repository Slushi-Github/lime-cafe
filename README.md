# EXPERIMENTAL (FALIED) FORK FOR THE Wii U

This is a (failed) attempt to port Lime to the Wii U, based on my fork of Lime for the Nintendo Switch

It doesn’t change much, aside from including the [gx2gl (Fork)](https://github.com/Slushi-Github/gx2gl) library (the includes and the precompiled library) to try to use Lime’s current OpenGL renderer with GX2, which is the Wii U’s graphics API, but I don’t have enough knowledge or the motivation to get this working. I suppose most of the code is in [`project/src/backend/sdl/SDLWindow.cpp`](https://github.com/Slushi-Github/lime-cafe/blob/main/project/src/backend/sdl/SDLWindow.cpp), where I see that most graphics-related things are initialized, but I never managed to get it working, plus hxcpp-cafe is still very unstable and crashes quickly. At least I managed to get SDL to detect the Wii U Gamepad’s touchscreen, even though the hxcpp GC crashes afterward, which leads me to believe that Lime was able to successfully initialize that part of DevKitPro’s SDL2 for the Wii U

All that aside, the Wii U doesn't have OpenAL, so you'd have to create a specific backend for that platform. Other than that, it doesn't have audio, but that's actually the last thing I'd be interested in at first, haha...

I'd really appreciate any help with this. 

This is how I feel after giving up on this project (VIDEO WITH LOUD SOUND, maybe?):

https://github.com/user-attachments/assets/79171259-06c3-43e1-9e41-523a89a67cfe

## How to use

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

And and generate your Lime library:

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

