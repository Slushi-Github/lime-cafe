package lime.utils;

import lime.utils.AssetManifest;
import lime.utils.AssetLibrary;
import lime.utils.AssetType;

import lime.media.AudioBuffer;
import lime.graphics.Image;
import lime.utils.Bytes;
import lime.text.Font;
import lime.app.Future;

import sys.FileSystem;
import sys.io.File;

import haxe.io.Path;

using StringTools;

/**
 * A utility that overrides the OpenFL/Lime Asset Library so that it uses `sys.io.File` and `sys.FileSystem` instead
 * 
 * This is because the orignal asset library for some reason doesn't work on the Wii U
 */
class WiiURootsLibrary extends AssetLibrary
{
    public final roots:Array<String>;

    public function new(roots:Array<String>)
    {
        this.roots = roots.filter(r -> r != null);
        super();

        final manifestPath:String = 'fs:/vol/content/manifest/default.json';
        if (FileSystem.exists(manifestPath))
        {
            final manifest = AssetManifest.fromFile(manifestPath, 'fs:/vol/content/');
            if (manifest != null)
            {
                __fromManifest(manifest);
                
                for (key in paths.keys())
                {
                    var p = paths.get(key);
                    if (p != null && p.startsWith("fs:/vol/") && !p.startsWith("fs:/vol/content/"))
                    {
                        var corrected = p.replace("fs:/vol/", "fs:/vol/content/");
                        paths.set(key, corrected);
                    }
                }
            }
        }

        this.loaded = true;
        trace("Custom Wii U asset library loaded!");
    }

    override public function load():Future<AssetLibrary>
    {
        return Future.withValue(cast (this, AssetLibrary));
    }

    override public function loadImage(id:String):Future<Image>
    {
        return Future.withValue(getImage(id));
    }

    override public function loadBytes(id:String):Future<Bytes>
    {
        return Future.withValue(getBytes(id));
    }

    override public function loadText(id:String):Future<String>
    {
        return Future.withValue(getText(id));
    }

    override public function loadAudioBuffer(id:String):Future<AudioBuffer>
    {
        return Future.withValue(getAudioBuffer(id));
    }

    override public function loadFont(id:String):Future<Font>
    {
        return Future.withValue(getFont(id));
    }

    override public function exists(id:String, type:String):Bool
    {
        if (getPath(id) != null)
        {
            if (type == null || cast(type, AssetType) == BINARY) return true;

            if (types.exists(id))
            {
                var requestedType:AssetType = cast type;
                var actualType:AssetType = types.get(id);

                if (actualType == requestedType) return true;
                if ((requestedType == SOUND || requestedType == MUSIC) && 
                    (actualType == SOUND || actualType == MUSIC))
                {
                    return true;
                }
                // trace("Asset type mismatch: " + id);
                return false;
            }

            return true;
        }

        // trace("Asset not found: " + id);
        return false;
    }

    override public function getPath(uPath:String):String
    {
        if (paths.exists(uPath))
        {
            trace("Mapping " + uPath + " to " + paths.get(uPath));
            uPath = paths.get(uPath);
        }

        if (uPath.startsWith("fs:/"))
        {
            if (FileSystem.exists(uPath))
            {
                // trace("Found direct path: " + uPath);
                return uPath;
            }
        }

        for (root in roots)
        {
            // trace("Trying root: " + root);
            final path:String = Path.join([root, uPath]);

            if (FileSystem.exists(path)) {
                // trace("Found path: " + path);
                return path;
            }
        }

        // trace("Asset not found: " + uPath + " in roots: " + roots + ", trying to resolve it...");
        return resolvePath(uPath);
    }

    override public function getBytes(id:String):Bytes
    {
        final path:String = getPath(id);
        return path == null ? null : File.getBytes(path);
    }

    override public function getText(id:String):String
    {
        final path:String = getPath(id);
        return path == null ? null : File.getContent(path);
    }

    @:dox(hide)
    public function resolve<T>(id:String, method:Bytes -> T):T
    {
        final bytes:Bytes = getBytes(id);
        return bytes == null ? null : method(bytes);
    }

    override public function getAudioBuffer(id:String):AudioBuffer
        return resolve(id, AudioBuffer.fromBytes);

    override public function getImage(id:String):Image
        return resolve(id, Image.fromBytes);

    override public function getFont(id:String):Font
        return resolve(id, Font.fromBytes);

    override public function getAsset(id:String, type:String):Dynamic
    {
        return switch (cast type)
        {
            case BINARY: getBytes(id);
            case TEXT: getText(id);
            case IMAGE: getImage(id);
            case SOUND, MUSIC: getAudioBuffer(id);
            case FONT: getFont(id);
            default: null;
        }
    }

    @:noCompletion private function resolvePath(uPath:String, ?keepRoot:Bool = true):String
    {
        final route:Array<String> = Path.normalize(uPath).split('/').filter(s -> s.length > 0);

        if (route.length == 0)
            return null;

        for (root in roots)
        {
            if (!FileSystem.exists(root))
                continue;

            var currentPath:String = root;
            var matchFound:Bool = true;

            for (index => dir in route)
            {
                if (!FileSystem.isDirectory(currentPath))
                {
                    matchFound = false;
                    break;
                }

                final targetSanitized:String = formatString(dir);
                var nextSegment:String = null;

                for (entry in FileSystem.readDirectory(currentPath))
                {
                    if (formatString(entry) == targetSanitized)
                    {
                        nextSegment = entry;
                        break;
                    }
                }

                if (nextSegment == null)
                {
                    matchFound = false;
                    break;
                }

                currentPath = Path.join([currentPath, nextSegment]);
            }

            if (matchFound && FileSystem.exists(currentPath)) {
                // trace("Resolved " + uPath + " to " + currentPath);
                return keepRoot ? currentPath : Path.join(route);
            }
        }

        // trace("Asset directly not found: " + uPath);
        return null;
    }

    @:noCompletion private function formatString(str:String):String
        return str.trim().toLowerCase().replace(' ', '-');
}