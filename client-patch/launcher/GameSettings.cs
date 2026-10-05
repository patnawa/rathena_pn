// PN launcher preferences and local settings snapshots. GPL-3.0-or-later.
using System;
using System.IO;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;

class LauncherPreferences {
    public bool autoRefresh=true;
    public bool minimizeOnPlay=true;
    internal static LauncherPreferences Load(string path) {
        try { Engine.NoLinks(path);if(File.Exists(path))return new JavaScriptSerializer().Deserialize<LauncherPreferences>(File.ReadAllText(path))??new LauncherPreferences(); }
        catch { /* A damaged preference file must not block the launcher. */ }
        return new LauncherPreferences();
    }
}

static class GameSettings {
    internal static string Tool(string root) {
        foreach(string name in new[]{"PNOpenSetup.exe","Setup.exe"}) {
            string path=Path.Combine(root,name);Engine.NoLinks(path);if(File.Exists(path))return name;
        }
        throw new IOException("Install the PN settings tools to open game settings.");
    }
    internal static string Summary(string root) {
        try {
            string path=Path.Combine(root,"savedata","OptionInfo.lua");Engine.NoLinks(path);
            if(new FileInfo(path).Length>1024*1024)return "Open game settings to choose your display.";
            string lua=File.ReadAllText(path);int width=Number(lua,"WIDTH"),height=Number(lua,"HEIGHT"),mode=Number(lua,"ISFULLSCREENMODE");
            if(width>0&&height>0)return width+" × "+height+"  ·  "+(mode==0?"Windowed":"Fullscreen")+"  ·  "+(Number(lua,"RENDERSYSTEM")==2?"DirectX 9":"Game renderer");
        }catch { }
        return "Open game settings to choose your display.";
    }
    static int Number(string lua,string key) {
        var match=Regex.Match(lua,"OptionInfoList\\[\""+Regex.Escape(key)+"\"\\]\\s*=\\s*(\\d+)");int value;
        return match.Success&&Int32.TryParse(match.Groups[1].Value,out value)?value:-1;
    }
    internal static string Backup(Engine engine) {
        string folder=Path.Combine(engine.Work,"settings-backups",DateTime.Now.ToString("yyyyMMdd-HHmmss")+"-"+Guid.NewGuid().ToString("N").Substring(0,6));
        Engine.NoLinks(folder);Directory.CreateDirectory(folder);int copied=0;
        foreach(string name in new[]{"savedata/OptionInfo.lua","savedata/UserKeys.lua","savedata/ChatWndInfo.lua","savedata/UIInfo.lua","savedata/MiniPartyInfo.lua","PN-Turbo.ini"}) {
            string source=Path.Combine(engine.Root,name.Replace('/',Path.DirectorySeparatorChar));Engine.NoLinks(source);
            if(!File.Exists(source))continue;
            if(new FileInfo(source).Length>8*1024*1024)throw new IOException("Settings file is unexpectedly large: "+name);
            string target=Path.Combine(folder,name.Replace('/',Path.DirectorySeparatorChar));Directory.CreateDirectory(Path.GetDirectoryName(target));File.Copy(source,target);copied++;
        }
        if(copied==0)throw new IOException("No saved game settings yet. Open game settings first.");
        return folder;
    }
}
