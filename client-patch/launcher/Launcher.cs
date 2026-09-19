// PN LAN launcher. GPL-3.0-or-later. .NET Framework 4.x; no external runtime.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Web.Script.Serialization;
using System.Windows.Forms;

class Entry { public string path; public long bytes; public string sha256; public bool preserve; }
class Manifest { public int schema; public long sequence; public string release; public Entry[] files; }
class Envelope { public string payload; public string signature; }
class Change { public string path; public bool existed; public string sha256; }
class Transaction { public string phase; public Change[] changes; public bool previousManifest; public string backupFolder="recovery"; public bool keepRollback; }

class Engine {
    public const string Feed="http://192.168.10.18:8082/";
    internal readonly string Root,Work;
    internal readonly JavaScriptSerializer Json=new JavaScriptSerializer { MaxJsonLength=8000000 };
    readonly Action<string,int> progress;
    static string PublicKey { get { using(var s=typeof(Engine).Assembly.GetManifestResourceStream("trusted-public-key.xml"))using(var r=new StreamReader(s))return r.ReadToEnd(); } }
    public Engine(string root,Action<string,int> report) {
        Root=Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar);Work=Path.Combine(Root,".pn-updater");progress=report;
        NoLinks(Root);Directory.CreateDirectory(Work);NoLinks(Work);
    }
    internal static string Hash(string path) { using(var s=File.OpenRead(path))using(var h=SHA256.Create())return BitConverter.ToString(h.ComputeHash(s)).Replace("-","").ToLowerInvariant(); }
    internal static void NoLinks(string path) {
        for(string p=path;!String.IsNullOrEmpty(p);p=Path.GetDirectoryName(p))
            if((File.Exists(p)||Directory.Exists(p)) && (File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked folders are not supported: "+p);
    }
    internal static void ValidName(string name) {
        if(String.IsNullOrWhiteSpace(name)||name.Length>220||name.IndexOf('\\')>=0||name.StartsWith("/")||name.IndexOf(':')>=0||name.Any(c=>c<32||"<>\"|?*".Contains(c)))throw new IOException("Invalid release path.");
        foreach(string part in name.Split('/'))
            if(part.Length==0||part=="."||part==".."||part.EndsWith(".")||part.EndsWith(" ")||Regex.IsMatch(part,@"^(CON|PRN|AUX|NUL|COM[0-9]|LPT[0-9])(\.|$)",RegexOptions.IgnoreCase))throw new IOException("Invalid release path.");
        string top=name.Split('/')[0];
        if(new[]{".pn-updater","savedata","ScreenShot","Replay","memo","PNLauncher.exe"}.Contains(top,StringComparer.OrdinalIgnoreCase))throw new IOException("Release attempts to overwrite personal or updater files.");
    }
    internal string SafePath(string basePath,string name) {
        ValidName(name);string p=Path.GetFullPath(Path.Combine(basePath,name.Replace('/',Path.DirectorySeparatorChar)));
        if(!p.StartsWith(basePath+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new IOException("Path escapes installation.");NoLinks(p);return p;
    }
    internal void Write(string path,string content) {
        NoLinks(path);string temp=path+".tmp";NoLinks(temp);
        using(var f=new FileStream(temp,FileMode.Create,FileAccess.Write,FileShare.None)){byte[] data=Encoding.UTF8.GetBytes(content);f.Write(data,0,data.Length);f.Flush(true);}
        if(File.Exists(path))File.Replace(temp,path,null);else File.Move(temp,path);
    }
    internal Manifest Verify(string wrapper) {
        var envelope=Json.Deserialize<Envelope>(wrapper);if(envelope==null)throw new IOException("Invalid release envelope.");
        byte[] payload=Convert.FromBase64String(envelope.payload),signature=Convert.FromBase64String(envelope.signature);
        using(var rsa=new RSACryptoServiceProvider(new CspParameters(24))){rsa.PersistKeyInCsp=false;rsa.FromXmlString(PublicKey);if(!rsa.VerifyData(payload,CryptoConfig.MapNameToOID("SHA256"),signature))throw new IOException("Release signature is invalid.");}
        var m=Json.Deserialize<Manifest>(Encoding.UTF8.GetString(payload));
        if(m==null||m.schema!=1||m.sequence<1||String.IsNullOrWhiteSpace(m.release)||m.files==null||m.files.Length==0||m.files.Length>20000)throw new IOException("Unsupported release manifest.");
        var names=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach(var f in m.files){ValidName(f.path);if(!names.Add(f.path)||f.bytes<0||f.bytes>8L*1024*1024*1024||!Regex.IsMatch(f.sha256??"","^[0-9a-f]{64}$"))throw new IOException("Invalid release file.");}
        if(!names.Contains("Ragexe.exe")||!names.Contains("RELEASE.json"))throw new IOException("Incomplete client manifest.");return m;
    }
    internal static HttpWebRequest Request(string url) {
        var r=(HttpWebRequest)WebRequest.Create(url);r.Proxy=null;r.AllowAutoRedirect=false;r.Timeout=15000;r.ReadWriteTimeout=30000;r.UserAgent="PNLauncher/1";return r;
    }
    internal static string Text(string url) {
        using(var r=(HttpWebResponse)Request(url).GetResponse()){
            if(r.StatusCode!=HttpStatusCode.OK||r.ContentLength>8000000)throw new IOException("Invalid server response.");
            using(var s=r.GetResponseStream())using(var m=new MemoryStream()){byte[] b=new byte[65536];int n;while((n=s.Read(b,0,b.Length))>0){m.Write(b,0,n);if(m.Length>8000000)throw new IOException("Server response too large.");}return Encoding.UTF8.GetString(m.ToArray());}
        }
    }
    internal void CheckStopped() { if(Process.GetProcessesByName("Ragexe").Length>0)throw new IOException("Close Ragnarok before updating or rolling back."); }
    internal void ClearDirectory(string p) {
        if(!p.StartsWith(Work+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new IOException("Invalid updater directory.");
        if(!Directory.Exists(p))return;NoLinks(p);
        foreach(string child in Directory.GetFileSystemEntries(p)){NoLinks(child);if(Directory.Exists(child))ClearDirectory(child);else File.Delete(child);}Directory.Delete(p);
    }
    internal void Recover() {
        string journal=Path.Combine(Work,"transaction.json");if(!File.Exists(journal))return;CheckStopped();NoLinks(journal);
        var t=Json.Deserialize<Transaction>(File.ReadAllText(journal));
        if(t==null||t.changes==null||!(t.phase=="applying"||t.phase=="complete")||!(t.backupFolder=="recovery"||t.backupFolder=="previous"))throw new IOException("Invalid update recovery journal.");
        if(t.phase=="complete"){Finish(t);return;}
        foreach(var c in t.changes.Reverse()) {
            string target=SafePath(Root,c.path),previous=SafePath(Path.Combine(Work,t.backupFolder),c.path);
            if(File.Exists(previous)){Directory.CreateDirectory(Path.GetDirectoryName(target));if(File.Exists(target))File.Delete(target);File.Move(previous,target);}
            else if(!c.existed && File.Exists(target)) {
                if(Hash(target)!=c.sha256)throw new IOException("A new file changed during recovery: "+c.path);File.Delete(target);
            }
        }
        string old=Path.Combine(Work,t.backupFolder+"-manifest.json"),current=Path.Combine(Work,"installed.json");
        NoLinks(old);NoLinks(current);
        if(t.previousManifest){if(!File.Exists(old))throw new IOException("Previous manifest is missing.");Write(current,File.ReadAllText(old));}
        else if(File.Exists(current))File.Delete(current);
        File.Delete(journal);ClearDirectory(Path.Combine(Work,"stage"));ClearDirectory(Path.Combine(Work,t.backupFolder));
        if(t.backupFolder=="previous"){string rollback=Path.Combine(Work,"rollback.json");NoLinks(rollback);if(File.Exists(rollback))File.Delete(rollback);}
    }
    internal void Finish(Transaction t) {
        string recovery=Path.Combine(Work,"recovery"),previous=Path.Combine(Work,"previous");
        if(t.keepRollback){
            if(Directory.Exists(recovery)){ClearDirectory(previous);Directory.Move(recovery,previous);}
            if(t.previousManifest)Write(Path.Combine(Work,"previous-manifest.json"),File.ReadAllText(Path.Combine(Work,"recovery-manifest.json")));
            t.backupFolder="previous";Write(Path.Combine(Work,"rollback.json"),Json.Serialize(t));
        }else ClearDirectory(recovery);
        ClearDirectory(Path.Combine(Work,"stage"));File.Delete(Path.Combine(Work,"transaction.json"));
    }
    internal void Download(Entry f,string stage) {
        string path=SafePath(stage,f.path);Directory.CreateDirectory(Path.GetDirectoryName(path));
        using(var response=(HttpWebResponse)Request(Feed+"objects/"+f.sha256).GetResponse()) {
            if(response.StatusCode!=HttpStatusCode.OK||response.ContentLength!=f.bytes)throw new IOException("Download length is wrong: "+f.path);
            using(var input=response.GetResponseStream())using(var output=new FileStream(path,FileMode.CreateNew,FileAccess.Write,FileShare.None)){
                byte[] buffer=new byte[1024*1024];long total=0;int n;
                while((n=input.Read(buffer,0,buffer.Length))>0){total+=n;if(total>f.bytes)throw new IOException("Download exceeds expected size.");output.Write(buffer,0,n);if(f.bytes>100000000)progress("Downloading "+f.path+"  "+(total/1048576)+" / "+(f.bytes/1048576)+" MB",-1);}
                output.Flush(true);if(total!=f.bytes)throw new IOException("Incomplete download: "+f.path);
            }
        }
        if(Hash(path)!=f.sha256)throw new IOException("Downloaded file failed verification: "+f.path);
    }
    public string Update() {
        CheckStopped();Recover();progress("Checking signed release...",0);
        string wrapper=Text(Feed+"release.json");Manifest manifest=Verify(wrapper);
        string highPath=Path.Combine(Work,"highest-sequence.txt");NoLinks(highPath);long highest=File.Exists(highPath)?Int64.Parse(File.ReadAllText(highPath)):0;
        if(manifest.sequence<highest)throw new IOException("An older release was rejected. Use Rollback for the saved local version.");
        var changes=new List<Entry>();int checkedCount=0;
        foreach(var f in manifest.files){string path=SafePath(Root,f.path);bool exists=File.Exists(path);
            if(!(f.preserve&&exists) && !(exists&&new FileInfo(path).Length==f.bytes&&Hash(path)==f.sha256))changes.Add(f);
            checkedCount++;if(checkedCount%20==0)progress("Checking client files: "+checkedCount+" / "+manifest.files.Length,checkedCount*45/manifest.files.Length);
        }
        if(changes.Count==0){Write(Path.Combine(Work,"installed.json"),wrapper);Write(highPath,Math.Max(highest,manifest.sequence).ToString());return manifest.release+" is verified and up to date.";}
        long bytes=changes.Sum(f=>f.bytes);var disk=new DriveInfo(Path.GetPathRoot(Root));
        if(disk.AvailableFreeSpace<bytes+256L*1024*1024)throw new IOException("Not enough free disk space for this update.");
        string stage=Path.Combine(Work,"stage"),previous=Path.Combine(Work,"recovery"),journal=Path.Combine(Work,"transaction.json");
        ClearDirectory(stage);Directory.CreateDirectory(stage);int count=0;
        foreach(var f in changes){progress("Downloading "+f.path,45+count*45/changes.Count);Download(f,stage);count++;}
        CheckStopped();NoLinks(journal);if(File.Exists(journal))File.Delete(journal);
        ClearDirectory(previous);Directory.CreateDirectory(previous);
        string installed=Path.Combine(Work,"installed.json");NoLinks(installed);
        bool previousManifest=File.Exists(installed);bool newVersion=previousManifest && Verify(File.ReadAllText(installed)).sequence!=manifest.sequence;
        if(previousManifest)Write(Path.Combine(Work,"recovery-manifest.json"),File.ReadAllText(installed));
        var transaction=new Transaction {phase="applying",previousManifest=previousManifest,keepRollback=newVersion,changes=changes.Select(f=>new Change{path=f.path,existed=File.Exists(SafePath(Root,f.path)),sha256=f.sha256}).ToArray()};
        Write(journal,Json.Serialize(transaction));
        try {
            foreach(var f in changes){CheckStopped();string target=SafePath(Root,f.path),old=SafePath(previous,f.path);Directory.CreateDirectory(Path.GetDirectoryName(target));
                if(File.Exists(target)){Directory.CreateDirectory(Path.GetDirectoryName(old));File.Move(target,old);}File.Move(SafePath(stage,f.path),target);
            }
            Write(installed,wrapper);Write(highPath,Math.Max(highest,manifest.sequence).ToString());transaction.phase="complete";Write(journal,Json.Serialize(transaction));Finish(transaction);
        }catch{Recover();throw;}
        progress("Update verified and installed.",100);return manifest.release+": "+changes.Count+" files updated ("+(bytes/1048576.0).ToString("0.0")+" MB downloaded).";
    }
    public string Rollback() {
        CheckStopped();Recover();string rollback=Path.Combine(Work,"rollback.json");NoLinks(rollback);if(!File.Exists(rollback))return "No previous version is stored. Repair does not replace a saved rollback.";
        var t=Json.Deserialize<Transaction>(File.ReadAllText(rollback));
        t.phase="applying";Write(Path.Combine(Work,"transaction.json"),Json.Serialize(t));Recover();return "The previous client files have been restored.";
    }
    public string Installed() {
        try {string p=Path.Combine(Root,"RELEASE.json");NoLinks(p);var x=Json.Deserialize<Dictionary<string,object>>(File.ReadAllText(p));return Convert.ToString(x["release_id"]);}catch{return "Client installation needed";}
    }
}

class MainForm:Form {
    Label release=new Label(),status=new Label(),message=new Label();ProgressBar bar=new ProgressBar();
    Button play=new Button(),update=new Button(),repair=new Button(),rollback=new Button(),web=new Button();Engine engine;bool busy;
    public MainForm() {
        Text="PN Ragnarok";ClientSize=new Size(660,430);MinimumSize=Size;MaximumSize=Size;StartPosition=FormStartPosition.CenterScreen;
        BackColor=Color.FromArgb(18,25,35);ForeColor=Color.FromArgb(230,237,245);Font=new Font("Segoe UI",10);FormBorderStyle=FormBorderStyle.FixedSingle;MaximizeBox=false;
        var eyebrow=new Label{Text="PN  /  PRIVATE LAN",Location=new Point(30,28),Size=new Size(570,25),ForeColor=Color.FromArgb(140,178,216)};
        var title=new Label{Text="Ragnarok",Font=new Font("Segoe UI",28,FontStyle.Bold),Location=new Point(26,58),Size=new Size(580,60)};
        release.Location=new Point(30,130);release.Size=new Size(600,25);status.Location=new Point(30,166);status.Size=new Size(600,25);status.Text="Server: checking 192.168.10.18...";
        message.Location=new Point(30,290);message.Size=new Size(600,64);message.Text="Updates download only changed files. Repair verifies the full client. Your saved settings and screenshots are kept.";
        bar.Location=new Point(30,365);bar.Size=new Size(600,8);bar.Maximum=100;
        Button[] buttons={play,update,repair,rollback,web};string[] titles={"Play","Update","Repair","Rollback","Status page"};
        for(int i=0;i<buttons.Length;i++){var b=buttons[i];b.Text=titles[i];b.Location=new Point(30+i*121,220);b.Size=new Size(114,42);b.FlatStyle=FlatStyle.Flat;b.FlatAppearance.BorderColor=Color.FromArgb(74,95,119);b.BackColor=i==0?Color.FromArgb(179,216,255):Color.FromArgb(30,43,60);b.ForeColor=i==0?Color.FromArgb(20,39,63):ForeColor;Controls.Add(b);}
        Controls.AddRange(new Control[]{eyebrow,title,release,status,message,bar});
        var turbo=new Button{Text="Turbo Setup",Location=new Point(30,390),Size=new Size(150,28),FlatStyle=FlatStyle.Flat,BackColor=Color.FromArgb(30,43,60)};
        turbo.Click+=(s,e)=>{try{string path=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"PNTurboConfig.exe");Engine.NoLinks(path);if(!File.Exists(path))throw new IOException("Update the client first to install Turbo Setup.");Process.Start(new ProcessStartInfo(path){WorkingDirectory=AppDomain.CurrentDomain.BaseDirectory});}catch(Exception ex){MessageBox.Show(this,ex.Message,"Turbo Setup");}};Controls.Add(turbo);
        engine=new Engine(AppDomain.CurrentDomain.BaseDirectory,Report);release.Text=engine.Installed();
        play.Click+=(s,e)=>Run(()=>engine.Update(),true);update.Click+=(s,e)=>Run(()=>engine.Update(),false);repair.Click+=(s,e)=>Run(()=>engine.Update(),false);
        rollback.Click+=(s,e)=>Run(()=>engine.Rollback(),false);web.Click+=(s,e)=>Process.Start(Engine.Feed);
        Shown+=(s,e)=>{Run(()=>{engine.Recover();return "Ready.";},false);RefreshStatus();};
        var timer=new System.Windows.Forms.Timer{Interval=30000};timer.Tick+=(s,e)=>RefreshStatus();timer.Start();
        FormClosing+=(s,e)=>{if(busy){e.Cancel=true;message.Text="Please wait for the current update to finish.";}};
    }
    void RefreshStatus(){ThreadPool.QueueUserWorkItem(_=>{string text;bool online=false;try{var s=engine.Json.Deserialize<Dictionary<string,object>>(Engine.Text(Engine.Feed+"status.json"));online=s.ContainsKey("game_online")&&(bool)s["game_online"];text=online?"Server online  ·  192.168.10.18":"Server offline or starting  ·  192.168.10.18";}catch{text="Cannot reach LAN status  ·  192.168.10.18";}if(!IsDisposed&&IsHandleCreated)BeginInvoke((Action)(()=>{status.Text=text;status.ForeColor=online?Color.FromArgb(132,224,176):Color.FromArgb(255,192,140);}));});}
    void Report(string text,int percent){if(!IsDisposed&&IsHandleCreated)BeginInvoke((Action)(()=>{message.Text=text;if(percent>=0)bar.Value=Math.Max(0,Math.Min(100,percent));}));}
    void Run(Func<string> action,bool start){if(busy)return;busy=true;foreach(var b in new[]{play,update,repair,rollback})b.Enabled=false;
        var worker=new BackgroundWorker();worker.DoWork+=(s,e)=>e.Result=action();worker.RunWorkerCompleted+=(s,e)=>{busy=false;foreach(var b in new[]{play,update,repair,rollback})b.Enabled=true;release.Text=engine.Installed();
            if(e.Error!=null){message.Text=e.Error.Message;bar.Value=0;}else{message.Text=(string)e.Result;bar.Value=100;if(start){var p=new ProcessStartInfo(Path.Combine(engine.Root,"Ragexe.exe")){WorkingDirectory=engine.Root,UseShellExecute=true};Process.Start(p);}}
        };worker.RunWorkerAsync();}
}
static class Program {
    [STAThread] static int Main(string[] args) {
        try {using(var mutex=new Mutex(false,"Local\\PNLauncher-"+AppDomain.CurrentDomain.BaseDirectory.ToLowerInvariant().GetHashCode())){
            if(!mutex.WaitOne(0,false))throw new IOException("PN Launcher is already running for this folder.");
            if(args.Length>0){var e=new Engine(AppDomain.CurrentDomain.BaseDirectory,(m,p)=>Console.WriteLine(m));
                if(args[0]=="--verify-manifest"){Console.WriteLine(e.Verify(File.ReadAllText(args[1])).release);return 0;}
                if(args[0]=="--update"){Console.WriteLine(e.Update());return 0;}
                if(args[0]=="--rollback"){Console.WriteLine(e.Rollback());return 0;}
                if(args[0]=="--self-test"){SelfTest.Run();return 0;}
                if(args[0]=="--render-preview"){using(var form=new MainForm()){form.CreateControl();using(var bitmap=new Bitmap(form.ClientSize.Width,form.ClientSize.Height)){using(var g=Graphics.FromImage(bitmap))g.Clear(form.BackColor);foreach(Control c in form.Controls){IntPtr handle=c.Handle;c.DrawToBitmap(bitmap,c.Bounds);}bitmap.Save(args[1],System.Drawing.Imaging.ImageFormat.Png);}}return 0;}
                throw new ArgumentException("Unknown command.");
            }
            Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);Application.Run(new MainForm());return 0;
        }}catch(Exception e){if(args.Length>0)Console.Error.WriteLine(e.Message);else MessageBox.Show(e.Message,"PN Launcher");return 1;}
    }
}
