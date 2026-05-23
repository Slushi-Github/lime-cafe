package;

import lime.tools.HashlinkHelper;
import hxp.Haxelib;
import hxp.HXML;
import hxp.Path;
import hxp.Log;
import hxp.NDLL;
import hxp.System;
import lime.tools.Architecture;
import lime.tools.AssetHelper;
import lime.tools.AssetType;
import lime.tools.CPPHelper;
import lime.tools.DeploymentHelper;
import lime.tools.HXProject;
import lime.tools.JavaHelper;
import lime.tools.NekoHelper;
import lime.tools.NodeJSHelper;
import lime.tools.Orientation;
import lime.tools.Platform;
import lime.tools.PlatformTarget;
import lime.tools.ProjectHelper;
import lime.tools.hxcompileu.WiiULinker;
import lime.tools.Icon;
import lime.tools.IconHelper;
import lime.graphics.Image;
import lime.graphics.ImageFileFormat;
import sys.io.File;
import sys.io.Process;
import sys.FileSystem;
import haxe.Resource;
import sys.net.Host;
import sys.net.Address;
import sys.net.UdpSocket;
import haxe.io.Bytes;


using StringTools;

class WiiUPlatform extends PlatformTarget
{
    private var applicationDirectory:String;
    private var executablePath:String;
    private var is64:Bool;
    private var targetType:String;

    private static final WIIU_CORE_LIBS:Array<String> = [];

    public function new(command:String, _project:HXProject, targetFlags:Map<String, String>)
    {
        super(command, _project, targetFlags);

        var defaults = new HXProject();

        defaults.meta =
            {
                title: "MyApplication",
                description: "",
                packageName: "com.example.myapp",
                version: "1.0.0",
                company: "",
                companyUrl: "",
                buildNumber: null,
                companyId: ""
            };

        defaults.app =
            {
                main: "Main",
                file: "MyApplication",
                path: "export",
                preloader: "",
                swfVersion: 17,
                url: "",
                init: null
            };

        defaults.window =
            {
                width: 1280,
                height: 720,
                parameters: "{}",
                background: 0xFFFFFF,
                fps: 60,
                hardware: true,
                display: 0,
                resizable: true,
                borderless: false,
                orientation: Orientation.AUTO,
                vsync: false,
                fullscreen: false,
                allowHighDPI: true,
                alwaysOnTop: false,
                antialiasing: 0,
                allowShaders: true,
                requireShaders: false,
                depthBuffer: true,
                stencilBuffer: true,
                colorDepth: 32,
                maximized: false,
                minimized: false,
                hidden: false,
                title: ""
            };

        defaults.architectures = [PPC];
        defaults.window.allowHighDPI = false;

        for (i in 1...project.windows.length)
        {
            defaults.windows.push(defaults.window);
        }

        defaults.merge(project);
        project = defaults;

        for (excludeArchitecture in project.excludeArchitectures)
        {
            project.architectures.remove(excludeArchitecture);
        }

        is64 = false;
        targetType = "cpp";

        var defaultTargetDirectory = "wiiu";
        targetDirectory = Path.combine(project.app.path, project.config.getString("wiiu.output-directory", defaultTargetDirectory));
        targetDirectory = StringTools.replace(targetDirectory, "arch64", is64 ? "64" : "");
        applicationDirectory = targetDirectory + "/bin/";
        executablePath = Path.combine(applicationDirectory, project.app.file);
    }

    private function getLimePath():String
    {
        return Haxelib.getPath(new Haxelib("lime"));
    }

    public override function build():Void
    {
        var hxml = targetDirectory + "/haxe/" + buildType + ".hxml";

        System.mkdir(targetDirectory);

        if (targetType == "cpp")
        {
            var haxeArgs = [hxml];
            var flags = [];

            haxeArgs.push("-D"); haxeArgs.push("EPPC");
            haxeArgs.push("-D"); haxeArgs.push("cafe");
            haxeArgs.push("-D"); haxeArgs.push("HX_CAFE");
            haxeArgs.push("-D"); haxeArgs.push("static_link");
            haxeArgs.push("-D"); haxeArgs.push("HXCPP_BIG_ENDIAN");
            haxeArgs.push("-D"); haxeArgs.push("WORDS_BIGENDIAN");
            haxeArgs.push("-D"); haxeArgs.push("FLOAT_WORDS_BIGENDIAN");
            haxeArgs.push("-D"); haxeArgs.push("__BIG_ENDIAN__");
            haxeArgs.push("-D"); haxeArgs.push("HXCPP_PTHREADS");
            // haxeArgs.push("-D"); haxeArgs.push("HXCPP_SINGLE_THREADED_APP");

            flags.push("-DEPPC");
            flags.push("-Dcafe=1");
            flags.push("-DHX_CAFE=1");
            flags.push("-D__WIIU__");
            flags.push("-D__WUT__");

            flags.push("-DHXCPP_PTHREADS");
            // flags.push("-DHXCPP_SINGLE_THREADED_APP");

            var dkp = Sys.getEnv("DEVKITPRO");
            if (dkp == null || dkp == "") {
                Log.error("Environment variable DEVKITPRO is not defined.");
                return;
            }

            var hxcpp_xlinux64_cxx = project.defines.get("HXCPP_XLINUX64_CXX");
            if (hxcpp_xlinux64_cxx == null) hxcpp_xlinux64_cxx = '$dkp/devkitPPC/bin/powerpc-eabi-g++';

            var hxcpp_xlinux64_strip = project.defines.get("HXCPP_XLINUX64_STRIP");
            if (hxcpp_xlinux64_strip == null) hxcpp_xlinux64_strip = '$dkp/devkitPPC/bin/powerpc-eabi-strip';

            var hxcpp_xlinux64_ranlib = project.defines.get("HXCPP_XLINUX64_RANLIB");
            if (hxcpp_xlinux64_ranlib == null) hxcpp_xlinux64_ranlib = '$dkp/devkitPPC/bin/powerpc-eabi-ranlib';

            var hxcpp_xlinux64_ar = project.defines.get("HXCPP_XLINUX64_AR");
            if (hxcpp_xlinux64_ar == null) hxcpp_xlinux64_ar = '$dkp/devkitPPC/bin/powerpc-eabi-ar';

            flags.push('-DHXCPP_XLINUX64_CXX=$hxcpp_xlinux64_cxx');
            flags.push('-DHXCPP_XLINUX64_STRIP=$hxcpp_xlinux64_strip');
            flags.push('-DHXCPP_XLINUX64_RANLIB=$hxcpp_xlinux64_ranlib');
            flags.push('-DHXCPP_XLINUX64_AR=$hxcpp_xlinux64_ar');

            System.runCommand("", "haxe", haxeArgs);

            if (noOutput) return;

            CPPHelper.compile(project, targetDirectory + "/obj", flags);

            var libName = project.debug ? "libApplicationMain-debug.a" : "libApplicationMain.a";
            var staticLib = targetDirectory + "/obj/" + libName;
            Log.info("Static library created: " + staticLib);

            var limePath = getLimePath();
            var path = Path.combine(limePath, "templates/wiiu/MakeFileWUHB");

            if (!FileSystem.exists(path)) {
                Log.error("Could not find Makefile template at: " + path);
                return;
            }

            var makefileTemplate = File.getContent(path);

            // Librerías extra del proyecto (wiiu.libs en project.xml)
            var userLibs:Array<String> = [];
            var libsStr = project.config.getString("wiiu.libs");
            if (libsStr != null && libsStr != "")
                userLibs = libsStr.split(",").map(s -> s.trim());

            // Unir librerías core + las del usuario, sin duplicados
            var additionalLibs:Array<String> = [];
            for (lib in WIIU_CORE_LIBS)
            {
                if (!additionalLibs.contains(lib))
                    additionalLibs.push(lib);
            }
            for (lib in userLibs)
            {
                if (lib != "" && !additionalLibs.contains(lib))
                    additionalLibs.push(lib);
            }

            WiiULinker.finalBuild({
                switchExportPath: targetDirectory,
                projectName: project.app.file,
                projectTitle: project.meta.title,
                projectAuthor: project.meta.company,
                projectVersion: project.meta.version,
                mainLibPath: staticLib,
                romfsPath: sys.FileSystem.absolutePath(Path.combine(applicationDirectory, "WIIU_ASSETS/romfs")),
                outputDir: "bin",
                limePath: limePath,
                makeFileTemplate: makefileTemplate,
                maxJobs: 4,
                additionalLibs: additionalLibs
            });
        }
    }

    public override function clean():Void
    {
        if (FileSystem.exists(targetDirectory))
        {
            System.removeDirectory(targetDirectory);
        }
    }

    public override function deploy():Void
    {
        DeploymentHelper.deploy(project, targetFlags, targetDirectory, "Nintendo Wii U");
    }

    public override function display():Void
    {
        if (project.targetFlags.exists("output-file"))
        {
            Sys.println(executablePath);
        }
        else
        {
            Sys.println(getDisplayHXML().toString());
        }
    }

    private function generateContext():Dynamic
    {
        var context = project.templateContext;
        context.CPP_DIR = targetDirectory + "/obj/";
        context.BUILD_DIR = project.app.path + "/wiiu";
        return context;
    }

    private function getDisplayHXML():HXML
    {
        var path = targetDirectory + "/haxe/" + buildType + ".hxml";

        if (FileSystem.exists(path))
        {
            return File.getContent(path);
        }
        else
        {
            var context = project.templateContext;
            var hxml = HXML.fromString(context.HAXE_FLAGS);
            hxml.addClassName(context.APP_MAIN);
            hxml.cpp = "_";
            hxml.noOutput = true;
            return hxml;
        }
    }

    public override function rebuild():Void
    {
        var dkp = Sys.getEnv("DEVKITPRO");
        var commands = [];

        commands.push([
            "-Dcafe=1",
            "-DHX_CAFE=1",
            "-Dstatic",
            "-Dstatic_link",
            // "-DHXCPP_SINGLE_THREADED_APP",
            "-DBINDIR=WiiU",
            "-DEPPC",
            "-DHXCPP_BIG_ENDIAN",
            "-DWORDS_BIGENDIAN",
            "-D__BIG_ENDIAN__",
            "-DFLOAT_WORDS_BIGENDIAN",
            "-DHXCPP_PTHREADS",
            "-DDEVKITPRO=" + dkp,
            "-DCXX=" + dkp + "/devkitPPC/bin/powerpc-eabi-g++",
            "-DHXCPP_STRIP=" + dkp + "/devkitPPC/bin/powerpc-eabi-strip",
            "-DHXCPP_AR=" + dkp + "/devkitPPC/bin/powerpc-eabi-ar",
            "-DHXCPP_RANLIB=" + dkp + "/devkitPPC/bin/powerpc-eabi-ranlib"
        ]);

        CPPHelper.rebuild(project, commands);
    }

      public override function run():Void
    {
        var wuhbPath = Path.combine(applicationDirectory, project.app.file + ".wuhb");

        if (!FileSystem.exists(wuhbPath)) {
            Log.error("WUHB file not found at: " + wuhbPath);
            return;
        }

        var consoleIP = project.config.getString("wiiu.ip");
        if (targetFlags.exists("ip")) consoleIP = targetFlags.get("ip");

        if (consoleIP == null || consoleIP == "") {
            Log.error("Console IP not set. Add <config:wiiu ip=\"192.168.x.x\" /> to your project.xml or pass --ip=... on the command line.");
            return;
        }

        var dkp = Sys.getEnv("DEVKITPRO");
        if (dkp == null || dkp == "") {
            Log.error("DEVKITPRO environment variable not found.");
            return;
        }

        var wiiloadProgram = Path.combine(dkp, "tools/bin/wiiload");
        if (System.hostPlatform == WINDOWS) wiiloadProgram += ".exe";

        if (!FileSystem.exists(wiiloadProgram)) {
            Log.error("wiiload not found at: " + wiiloadProgram + " (install wii-tools via devkitPro pacman)");
            return;
        }

        if (Sys.getEnv("WIILOAD") == null) {
            Sys.putEnv("WIILOAD", "tcp:" + consoleIP);
        } else {
            Log.warn("WIILOAD env var already set, using existing value: " + Sys.getEnv("WIILOAD"));
        }

        var stat = FileSystem.stat(wuhbPath);
        var fileSizeMB:Float = Math.round((stat.size / 1024.0 / 1024.0) * 100) / 100;
        Log.info("Sending: [" + project.app.file + ".wuhb] (" + fileSizeMB + " MB) to " + consoleIP);

        var exitCode = System.runCommand("", wiiloadProgram, [wuhbPath]);
        if (exitCode != 0) {
            Log.error("wiiload: Transfer failed with code " + exitCode);
            return;
        }

        var udpPort = 4405;
        var udpPortStr = project.config.getString("wiiu.udp-port");
        if (udpPortStr != null && udpPortStr != "") {
            var parsed = Std.parseInt(udpPortStr);
            if (parsed != null) udpPort = parsed;
        }

        Log.info("Listening for Wii U logs on UDP port " + udpPort + " (Ctrl+C to stop)...");
        Log.info("--------------------------------------------------");

        var socket = new UdpSocket();
        try {
            var host = new Host("0.0.0.0");
            socket.bind(host, udpPort);
            socket.setTimeout(0); // bloqueante

            var buf = Bytes.alloc(4096);
            var addr = new Address();

            while (true) {
                var read = socket.readFrom(buf, 0, buf.length, addr);
                if (read > 0) {
                    var msg = buf.getString(0, read);
                    Sys.print(msg);
                }
            }
        } catch (e:Dynamic) {
            Log.info("--------------------------------------------------");
            Log.error("UDP listener stopped: " + e);
        }

        socket.close();
    }

    public override function update():Void
    {
        AssetHelper.processLibraries(project, targetDirectory);

        var context = generateContext();
        context.OUTPUT_DIR = targetDirectory;

        System.mkdir(targetDirectory);
        System.mkdir(targetDirectory + "/obj");
        System.mkdir(targetDirectory + "/haxe");
        System.mkdir(applicationDirectory);

        var limePath = getLimePath();

        var limeLibDest = Path.combine(targetDirectory, "obj/LIME_LIB/lib");
        System.mkdir(limeLibDest);

        var limeSource = Path.combine(limePath, "ndll/WiiU/liblime.a");
        if (FileSystem.exists(limeSource)) {
            File.copy(limeSource, Path.combine(limeLibDest, "liblime.a"));
        } else {
            Log.warn("liblime.a not found at: " + limeSource);
        }

        var gl33Source = Path.combine(limePath, "templates/wiiu/libgl33_gx2_core.a");
        if (FileSystem.exists(gl33Source)) {
            File.copy(gl33Source, Path.combine(limeLibDest, "libgl33_gx2_core.a"));
            Log.info("libgl33_gx2_core.a copied to: " + limeLibDest);
        } else {
            Log.warn("libgl33_gx2_core.a not found at: " + gl33Source);
        }

        var romfsDirectory = Path.combine(applicationDirectory, "WIIU_ASSETS/romfs");
        System.mkdir(romfsDirectory);

        var icons = project.icons;

        if (icons.length == 0) {
            var defaultIconPath = System.findTemplate(project.templatePaths, "default/icon.svg");
            if (defaultIconPath != null) {
                Log.info("Using default icon for Wii U: " + defaultIconPath);

                var switchAssetsDir = Path.combine(applicationDirectory, "WIIU_ASSETS");
                System.mkdir(switchAssetsDir);

                var iconPngPath = Path.combine(switchAssetsDir, "icon_temp.png");
                var iconJpgPath = Path.combine(switchAssetsDir, "icon.jpg");

                if (IconHelper.createIcon([new Icon(defaultIconPath)], 256, 256, iconPngPath)) {
                    try {
                        var image = Image.fromFile(iconPngPath);
                        if (image != null) {
                            if (image.width != 256 || image.height != 256)
                                image.resize(256, 256);

                            var jpegBytes = image.encode(ImageFileFormat.JPEG, 100);
                            if (jpegBytes != null) {
                                File.saveBytes(iconJpgPath, jpegBytes);
                                Log.info("Wii U icon created from default at: " + iconJpgPath);
                                context.HAS_ICON = true;
                            } else {
                                Log.warn("Could not encode default icon to JPEG");
                            }
                        } else {
                            Log.warn("Could not load default icon PNG");
                        }
                    } catch (e:Dynamic) {
                        Log.warn("Error converting default icon to JPG: " + e);
                    }

                    if (FileSystem.exists(iconPngPath))
                        FileSystem.deleteFile(iconPngPath);
                } else {
                    Log.warn("Could not create Wii U icon from default");
                }
            }
        }

        if (icons.length > 0) {
            var switchAssetsDir = Path.combine(applicationDirectory, "WIIU_ASSETS");
            System.mkdir(switchAssetsDir);

            var iconPngPath = Path.combine(switchAssetsDir, "icon_temp.png");
            var iconJpgPath = Path.combine(switchAssetsDir, "icon.jpg");

            if (IconHelper.createIcon(icons, 256, 256, iconPngPath)) {
                try {
                    var image = Image.fromFile(iconPngPath);
                    if (image != null) {
                        if (image.width != 256 || image.height != 256)
                            image.resize(256, 256);

                        var jpegBytes = image.encode(ImageFileFormat.JPEG, 100);
                        if (jpegBytes != null) {
                            File.saveBytes(iconJpgPath, jpegBytes);
                            Log.info("Wii U icon created at: " + iconJpgPath);
                            context.HAS_ICON = true;
                        } else {
                            Log.error("Could not encode image to JPEG");
                        }
                    } else {
                        Log.error("Could not load PNG for conversion");
                    }
                } catch (e:Dynamic) {
                    Log.error("Error converting icon to JPG: " + e);
                }

                if (FileSystem.exists(iconPngPath))
                    FileSystem.deleteFile(iconPngPath);
            } else {
                Log.error("Could not create Wii U icon");
            }
        }

        ProjectHelper.recursiveSmartCopyTemplate(project, "haxe", targetDirectory + "/haxe", context);
        ProjectHelper.recursiveSmartCopyTemplate(project, targetType + "/hxml", targetDirectory + "/haxe", context);

        if (targetType == "cpp")
        {
            ProjectHelper.recursiveSmartCopyTemplate(project, "cpp/static", targetDirectory + "/obj", context);
        }

        for (asset in project.assets)
        {
            var path = Path.combine(romfsDirectory, asset.targetPath);
            if (asset.embed != true)
            {
                System.mkdir(Path.directory(path));
                if (asset.type != AssetType.TEMPLATE)
                    AssetHelper.copyAssetIfNewer(asset, path);
                else
                    AssetHelper.copyAsset(asset, path, context);
            }
        }
    }

    public override function watch():Void
    {
        var hxml = getDisplayHXML();
        var dirs = hxml.getClassPaths(true);
        var command = ProjectHelper.getCurrentCommand();
        System.watch(command, dirs);
    }

    @ignore public override function install():Void {}
    @ignore public override function trace():Void {}
    @ignore public override function uninstall():Void {}
}