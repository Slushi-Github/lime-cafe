package lime.tools.hxcompileu;

import hxp.System;
import hxp.Path;
import hxp.Log;
import sys.FileSystem;
import sys.io.File;

using StringTools;

/**
 * Args for the Wii U MakeFile
 */
typedef WiiUMakeFileArgs = {
    var switchExportPath:String;
    var projectName:String;
    var projectTitle:String;
    var projectAuthor:String;
    var projectVersion:String;
    var mainLibPath:String;
    var romfsPath:String;
    var outputDir:String;
    var limePath:String;
    var makeFileTemplate:String;
    var maxJobs:Int;
    @:optional var additionalLibs:Array<String>;
}

/**
 * Linker for the Wii U, for generating the executables for the console
 *
 * Based on HaxeNXCompiler:
 * https://github.com/Slushi-Github/HaxeNXCompiler/blob/main/source/compilers/nx/NXLinker.hx
 *
 * @author Slushi
 */
class WiiULinker {

    /**
     * Generates the MakeFile and executes it
     * @param args The arguments for the MakeFile
     */
    public static function finalBuild(args:WiiUMakeFileArgs):Void {
        var basePath = Sys.getCwd().replace("\\", "/");

        var exportPath = args.switchExportPath;
        if (Path.isRelative(exportPath)) {
            exportPath = Path.combine(basePath, exportPath);
        }

        var objDir = Path.combine(exportPath, "obj");
        var binDir = Path.combine(exportPath, "bin");

        if (!FileSystem.exists(objDir)) FileSystem.createDirectory(objDir);
        if (!FileSystem.exists(binDir)) FileSystem.createDirectory(binDir);

        args.switchExportPath = exportPath;

        createMakefile(args);
        compileMakefile(args);
    }

    /**
     * Creates the MakeFile
     * @param args The arguments for the MakeFile
     */
    private static function createMakefile(args:WiiUMakeFileArgs):Void {
        var makeFile = args.makeFileTemplate;
        var exportPath = args.switchExportPath;

        makeFile = makeFile.replace("[LIME_PROJECT_FILENAME]", args.projectName);
        makeFile = makeFile.replace("[LIME_PROJECT_TITLE]",    args.projectTitle);
        makeFile = makeFile.replace("[LIME_PROJECT_AUTHOR]",   args.projectAuthor);
        makeFile = makeFile.replace("[LIME_PROJECT_VERSION]",  args.projectVersion);
        makeFile = makeFile.replace("[LIME_MAIN_SRC_DIR]",     "../obj");

        var libFileName = Path.withoutDirectory(args.mainLibPath);
        makeFile = makeFile.replace("[HAXE_MAIN_LIB]", Path.combine(exportPath, "obj/" + libFileName));

        makeFile = makeFile.replace("[LIME_MAIN_DIR]", Path.combine(exportPath, "obj/LIME_LIB"));

        var absoluteOutDir = Path.combine(exportPath, "bin");
        makeFile = makeFile.replace("[OUT_DIR]", absoluteOutDir);

        makeFile = makeFile.replace("[LIME_APPLICATION_DIR]", args.romfsPath);

        var additionalLibsStr = "";
        if (args.additionalLibs != null && args.additionalLibs.length > 0) {
            var validLibs = args.additionalLibs.filter(lib -> lib != null && lib != "");
            if (validLibs.length > 0) {
                additionalLibsStr = " " + validLibs.map(lib -> "-l" + lib).join(" ");
            }
        }
        makeFile = makeFile.replace("[ADDITIONAL_LIBS]", additionalLibsStr);

        var makefilePath = Path.combine(exportPath, "obj/Makefile");
        File.saveContent(makefilePath, makeFile);

        Log.info("Makefile created at: " + makefilePath);
        if (args.additionalLibs != null && args.additionalLibs.length > 0) {
            var validLibs = args.additionalLibs.filter(lib -> lib != null && lib != "");
            if (validLibs.length > 0) {
                Log.info("Additional libraries: " + validLibs.join(", "));
            }
        }
    }

    /**
     * Compiles the MakeFile
     * @param args The arguments for the MakeFile
     */
    private static function compileMakefile(args:WiiUMakeFileArgs):Void {
        var objDir = Path.combine(args.switchExportPath, "obj");
        var originalDir = Sys.getCwd();

        Sys.setCwd(objDir);
        var compileResult = System.runCommand("", "make", ["-j" + args.maxJobs]);
        Sys.setCwd(originalDir);

        if (compileResult != 0) {
            Log.error("Nintendo Wii U compilation failed!");
        }
    }
}