# Custom backend for Wii U

This is because, apparently, certain features that the Wii U SDL is supposed to provide never seem to work properly with Lime for some reason, so I just prefer to use [WUT](https://github.com/devkitPro/wut) code to handle it myself.

## Input

The input in a Lime app is handled by the `WiiUInput` class, uses directly the WUT functions to get the input, touch and sensors, and dispatch events to the Lime events about it, so the code of the Haxe side doesn't need to be changed at all. The input are mapped to the Nintendo Switch Pro controller layout.