package lime.utils.wiiu;

import lime.utils.AssetManifest;
import lime.utils.AssetLibrary;
import lime.utils.AssetType;
import lime.utils.wiiu.WiiUMacros;

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

    var caches(get, never):Array<Map<String, Dynamic>>;
    function get_caches():Array<Map<String, Dynamic>>
        return [cachedAudioBuffers, cachedBytes, cachedFonts, cachedImages, cachedText];

    public function new(roots:Array<String>)
    {
        this.roots = roots.filter(r -> r != null);
        super();

        try
        {
            final proxy = Assets.getLibrary('default');
            if (proxy != null)
            {
                classTypes = proxy.classTypes.copy();

                cachedAudioBuffers = proxy.cachedAudioBuffers.copy();
                cachedBytes = proxy.cachedBytes.copy();
                cachedFonts = proxy.cachedFonts.copy();
                cachedImages = proxy.cachedImages.copy();
                cachedText = proxy.cachedText.copy();
            }
        }
        catch (e:Dynamic) {}

        final baseRoot:String = WiiUMacros.getCustomRoot();
        final manifestPath:String = baseRoot + "manifest/default.json";
        
        if (FileSystem.exists(manifestPath))
        {
            final manifest = AssetManifest.fromFile(manifestPath, baseRoot);
            if (manifest != null)
            {
                __fromManifest(manifest);
                
                for (key in paths.keys())
                {
                    var p = paths.get(key);
                    if (p != null && p.startsWith("fs:/vol/") && !p.startsWith(baseRoot))
                    {
                        var corrected = p.replace("fs:/vol/content/", baseRoot);
                        paths.set(key, corrected);
                    }
                }
            }
        }

        this.loaded = true;
        trace("Custom Wii U roots asset library loaded with root: " + baseRoot);
    }

    override public function load():Future<AssetLibrary>
    {
        return Future.withValue(cast this);
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
        return getPath(id) != null || getEmbedPath(id) != null || getCachedPath(id) != null;
    }

    override public function getPath(uPath:String):String
    {
        if (paths.exists(uPath))
        {
            var mapped = paths.get(uPath);
            if (FileSystem.exists(mapped))
                return mapped;
        }

        if (uPath.startsWith("fs:/"))
        {
            if (FileSystem.exists(uPath))
                return uPath;
        }

        for (root in roots)
        {
            final path = Path.join([root, uPath]);
            if (FileSystem.exists(path))
                return path;
        }

        return resolvePath(uPath);
    }

    public function getEmbedPath(uPath:String):String
    {
        for (root in roots)
        {
            final path = Path.join([root, uPath]);

            if (classTypes.exists(path))
                return path;
        }

        return classTypes.exists(uPath) ? uPath : null;
    }

    public function getCachedPath(uPath:String, ?map:Map<String, Dynamic>):WiiUAssetsLibraryCache
    {
        for (root in roots)
        {
            final path = Path.join([root, uPath]);

            if (map == null)
            {
                for (cache in caches)
                {
                    if (cache.exists(path))
                        return { cache: cache, path: path };
                }
            } 
            else if (map.exists(path))
            {
                return { cache: map, path: path };
            }
        }

        if (map == null)
        {
            for (cache in caches)
            {
                if (cache.exists(uPath))
                    return { cache: cache, path: uPath };
            }
        }
        else if (map.exists(uPath))
        {
            return { cache: map, path: uPath };
        }

        return null;
    }

    public function getEmbed(id:String):Dynamic
    {
        final path:String = getEmbedPath(id);
        return path == null ? null : Type.createInstance(classTypes[path], []);
    }

    public function getCached(id:String, ?map:Map<String, Dynamic>):Dynamic
    {
        final path = getCachedPath(id, map);
        return path == null ? null : path.cache[path.path];
    }

    override public function getBytes(id:String):Bytes
    {
        final path = getPath(id);

        if (path != null && FileSystem.exists(path))
            return File.getBytes(path);
        
        return getEmbed(id) ?? getCached(id, cachedBytes);
    }

    override public function getText(id:String):String
    {
        final path = getPath(id);

        if (path != null && FileSystem.exists(path))
            return File.getContent(path);

        return getEmbed(id) ?? getCached(id, cachedText);
    }

    override public function getAudioBuffer(id:String):AudioBuffer
    {
        final bytes = getBytes(id);
        return bytes == null ? getCached(id, cachedAudioBuffers) : AudioBuffer.fromBytes(bytes);
    }

    override public function getImage(id:String):Image
    {
        final path = getPath(id);

        if (path != null && FileSystem.exists(path))
            return Image.fromBytes(File.getBytes(path));

        return getEmbed(id) ?? getCached(id, cachedImages);
    }

    override public function getFont(id:String):Font
    {
        final path = getPath(id);

        if (path != null && FileSystem.exists(path))
            return Font.fromBytes(File.getBytes(path));

        return getEmbed(id) ?? getCached(id, cachedFonts);
    }

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

            if (matchFound && FileSystem.exists(currentPath))
                return keepRoot ? currentPath : Path.join(route);
        }

        return null;
    }

    @:noCompletion private function formatString(str:String):String
        return str.trim().toLowerCase().replace(' ', '-');
}

typedef WiiUAssetsLibraryCache = {
    cache: Map<String, Dynamic>,
    path: String
}