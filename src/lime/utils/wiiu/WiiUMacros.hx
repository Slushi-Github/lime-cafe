package lime.utils.wiiu;

#if macro
import haxe.macro.Context;
import haxe.macro.Expr;
#end

using StringTools;

/**
 * A Simple utility for get a custom path for the assets of the Lime program on the Wii U 
 */
class WiiUMacros
{
    macro public static function getCustomRoot():ExprOf<String>
    {
        #if macro
        var customPath = Context.definedValue("WIIU_ROOT_PATH");

        if (customPath != null && customPath.length > 0)
        {
            if (!customPath.endsWith("/")) customPath += "/";
            return macro $v{customPath};
        }
        
        return macro $v{"fs:/vol/content/"};
        #end
    }
}