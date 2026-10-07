// PN launcher dashboard. GPL-3.0-or-later. Midgard artwork is embedded locally.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Web.Script.Serialization;
using System.Windows.Forms;

static class Palette {
    public static readonly Color Background=Color.FromArgb(13,20,29), Sidebar=Color.FromArgb(17,26,37),
        Card=Color.FromArgb(23,34,46), Border=Color.FromArgb(43,58,72), Text=Color.FromArgb(235,238,239),
        Muted=Color.FromArgb(150,167,182), Gold=Color.FromArgb(231,193,126), Green=Color.FromArgb(112,210,162),
        Red=Color.FromArgb(245,155,143);
    public static GraphicsPath Round(RectangleF r,float radius) {
        var path=new GraphicsPath();float d=radius*2;
        path.AddArc(r.X,r.Y,d,d,180,90);path.AddArc(r.Right-d,r.Y,d,d,270,90);
        path.AddArc(r.Right-d,r.Bottom-d,d,d,0,90);path.AddArc(r.X,r.Bottom-d,d,d,90,90);path.CloseFigure();return path;
    }
}

class Surface:Panel {
    public Surface(){DoubleBuffered=true;BackColor=Palette.Card;}
    protected override void OnPaint(PaintEventArgs e){base.OnPaint(e);e.Graphics.SmoothingMode=SmoothingMode.AntiAlias;
        using(var p=Palette.Round(new RectangleF(0.5f,0.5f,Width-1,Height-1),12))using(var pen=new Pen(Palette.Border))e.Graphics.DrawPath(pen,p);}
}

class ActionButton:Button {
    public bool Primary,Navigation,Selected;
    bool hover;
    public ActionButton(){FlatStyle=FlatStyle.Flat;FlatAppearance.BorderSize=0;BackColor=Palette.Card;ForeColor=Palette.Text;
        Font=new Font("Segoe UI",9.5f);Cursor=Cursors.Hand;DoubleBuffered=true;UseVisualStyleBackColor=false;}
    protected override void OnMouseEnter(EventArgs e){hover=true;Invalidate();base.OnMouseEnter(e);}
    protected override void OnMouseLeave(EventArgs e){hover=false;Invalidate();base.OnMouseLeave(e);}
    protected override void OnPaint(PaintEventArgs e){var g=e.Graphics;g.SmoothingMode=SmoothingMode.AntiAlias;
        g.Clear(Parent==null?Palette.Background:Parent.BackColor);
        Color fill=Primary?(hover?Color.FromArgb(248,211,151):Palette.Gold):(Selected?Color.FromArgb(39,53,66):(hover?Color.FromArgb(43,58,72):BackColor));
        if(!Enabled)fill=Color.FromArgb(35,44,53);
        using(var path=Palette.Round(new RectangleF(1,1,Width-2,Height-2),7)){
            using(var brush=new SolidBrush(fill))g.FillPath(brush,path);
            if(!Navigation&&!Primary)using(var pen=new Pen(Palette.Border))g.DrawPath(pen,path);
            if(Focused)using(var pen=new Pen(Palette.Gold)){pen.DashStyle=DashStyle.Dot;g.DrawPath(pen,path);}
        }
        var color=!Enabled?Color.FromArgb(111,127,140):(Primary?Palette.Background:(Selected?Palette.Gold:ForeColor));
        TextRenderer.DrawText(g,Text,Font,new Rectangle(Navigation?16:5,0,Width-(Navigation?20:10),Height),color,
            TextFormatFlags.VerticalCenter|TextFormatFlags.SingleLine|TextFormatFlags.EndEllipsis|TextFormatFlags.NoPrefix|TextFormatFlags.NoPadding|(Navigation?TextFormatFlags.Left:TextFormatFlags.HorizontalCenter));
    }
}

class PatchBar:Control {
    int value;
    public int Value{get{return value;}set{this.value=Math.Max(0,Math.Min(100,value));Invalidate();}}
    public PatchBar(){DoubleBuffered=true;BackColor=Palette.Card;}
    protected override void OnPaint(PaintEventArgs e){e.Graphics.SmoothingMode=SmoothingMode.AntiAlias;
        using(var p=Palette.Round(new RectangleF(0,0,Width,Height),Height/2f))using(var b=new SolidBrush(Palette.Border))e.Graphics.FillPath(b,p);
        if(value>0)using(var p=Palette.Round(new RectangleF(0,0,Math.Max(Height,Width*value/100f),Height),Height/2f))
            using(var b=new LinearGradientBrush(ClientRectangle,Palette.Gold,Color.FromArgb(247,218,168),0f))e.Graphics.FillPath(b,p);
    }
}

class RealmArt:Panel {
    readonly Image artwork;
    public RealmArt(){DoubleBuffered=true;BackColor=Palette.Card;
        using(var stream=typeof(RealmArt).Assembly.GetManifestResourceStream("midgard-hero.jpg"))if(stream!=null)using(var image=Image.FromStream(stream))artwork=new Bitmap(image);
    }
    protected override void OnPaintBackground(PaintEventArgs e){
        var g=e.Graphics;g.SmoothingMode=SmoothingMode.AntiAlias;
        using(var clip=Palette.Round(new RectangleF(0,0,Width,Height),12)){
            var state=g.Save();g.SetClip(clip);g.Clear(BackColor);
            if(artwork!=null){float scale=Math.Max((float)Width/artwork.Width,(float)Height/artwork.Height);
                g.DrawImage(artwork,new RectangleF(Width-artwork.Width*scale,(Height-artwork.Height*scale)/2,artwork.Width*scale,artwork.Height*scale));}
            using(var fade=new LinearGradientBrush(ClientRectangle,Color.FromArgb(245,13,24,34),Color.FromArgb(0,13,24,34),0f))g.FillRectangle(fade,ClientRectangle);
            using(var shade=new SolidBrush(Color.FromArgb(25,13,24,34)))g.FillRectangle(shade,ClientRectangle);
            g.Restore(state);using(var pen=new Pen(Palette.Border))g.DrawPath(pen,clip);
        }
    }
    protected override void Dispose(bool disposing){if(disposing&&artwork!=null)artwork.Dispose();base.Dispose(disposing);}
}

class MainForm:Form {
    readonly Engine engine;
    readonly PictureBox brandMark=new PictureBox();
    readonly ToolTip tips=new ToolTip {AutoPopDelay=12000,InitialDelay=350,ReshowDelay=100};
    readonly System.Windows.Forms.Timer statusTimer=new System.Windows.Forms.Timer {Interval=30000};
    readonly System.Windows.Forms.Timer uiTimer=new System.Windows.Forms.Timer {Interval=120};
    readonly object progressLock=new object();
    readonly List<string> activity=new List<string>();
    readonly List<ActionButton> operationButtons=new List<ActionButton>();
    readonly List<ActionButton> toolButtons=new List<ActionButton>();
    readonly Label sidebarStatus,serverValue,serverDetail,clientValue,clientDetail,diskValue,phase,detail,percent,transfer,heading,subheading;
    readonly Label releaseSummary;
    readonly PatchBar bar;
    readonly ActionButton play,cancel,rollback,refresh;
    readonly ActionButton[] navigation;
    readonly Panel overview,notesPage,activityPage,settingsPage;
    readonly LauncherPreferences preferences;
    readonly Label displaySummary,settingsNotice;
    string preferencesPath {get{return Path.Combine(engine.Work,"launcher-settings.json");}}
    readonly TextBox notes,history;
    readonly bool preview;
    internal Action<string> StartClient=path=>Process.Start(new ProcessStartInfo(path){WorkingDirectory=Path.GetDirectoryName(path),UseShellExecute=true});
    CancellationTokenSource cancellation;
    readonly CancellationTokenSource statusCancellation=new CancellationTokenSource();
    bool busy,closing,canCancel,startupCheck,playAfterCheck,closeAfterCancel;
    int statusPending;
    string pendingText;
    int pendingPercent=-1;
    long pendingBytes,pendingTotal;
    double pendingSpeed;
    bool transferPending;
    string installedRelease="",lastResult="No update check in this session.",connection="Checking connection";
    DateTime? lastCheck;

    Label LabelAt(Control parent,string text,int x,int y,int width,int height,float size,Color color,bool bold=false){
        var l=new Label{Text=text,Bounds=new Rectangle(x,y,width,height),ForeColor=color,BackColor=Color.Transparent,
            Font=new Font("Segoe UI",size,bold?FontStyle.Bold:FontStyle.Regular),AutoEllipsis=true,UseMnemonic=false};parent.Controls.Add(l);return l;
    }
    ActionButton ButtonAt(Control parent,string text,int x,int y,int width,int height,Action click,bool primary=false){
        var b=new ActionButton{Text=text,Bounds=new Rectangle(x,y,width,height),Primary=primary,BackColor=parent.BackColor};
        b.Click+=(s,e)=>click();parent.Controls.Add(b);return b;
    }
    Surface CardAt(Control parent,int x,int y,int width,int height){var p=new Surface{Bounds=new Rectangle(x,y,width,height)};parent.Controls.Add(p);return p;}

    public MainForm():this(AppDomain.CurrentDomain.BaseDirectory,false){}
    internal MainForm(string root,bool renderPreview){
        preview=renderPreview;AutoScaleDimensions=new SizeF(96,96);AutoScaleMode=AutoScaleMode.Dpi;
        Text="PN Ragnarok · Launcher";ClientSize=new Size(1060,744);FormBorderStyle=FormBorderStyle.FixedSingle;MaximizeBox=false;
        using(var stream=typeof(MainForm).Assembly.GetManifestResourceStream("pn-launcher.ico"))if(stream!=null)using(var icon=new Icon(stream))Icon=(Icon)icon.Clone();
        using(var stream=typeof(MainForm).Assembly.GetManifestResourceStream("pn-launcher.png"))if(stream!=null)using(var image=Image.FromStream(stream))brandMark.Image=new Bitmap(image);
        StartPosition=FormStartPosition.CenterScreen;BackColor=Palette.Background;ForeColor=Palette.Text;Font=new Font("Segoe UI",10);KeyPreview=true;
        engine=new Engine(root,Report);engine.TransferProgress=ReportTransfer;preferences=LauncherPreferences.Load(preferencesPath);
        var sidebar=new Panel{Bounds=new Rectangle(0,0,206,744),BackColor=Palette.Sidebar};Controls.Add(sidebar);
        brandMark.Bounds=new Rectangle(25,21,72,72);brandMark.SizeMode=PictureBoxSizeMode.Zoom;brandMark.BackColor=Color.Transparent;sidebar.Controls.Add(brandMark);
        tips.SetToolTip(brandMark,"PN Ragnarok · Midgard");
        LabelAt(sidebar,"RAGNAROK",27,100,165,24,11,Palette.Text,true);
        LabelAt(sidebar,"YOUR GATE TO MIDGARD",27,127,165,18,7.5f,Palette.Muted);
        LabelAt(sidebar,"LAUNCHER",27,167,145,20,8,Palette.Muted,true);
        navigation=new[]{ButtonAt(sidebar,"Overview",15,197,176,43,()=>Navigate(0)),ButtonAt(sidebar,"What's new",15,247,176,43,()=>Navigate(1)),ButtonAt(sidebar,"Activity",15,297,176,43,()=>Navigate(2)),ButtonAt(sidebar,"Settings",15,347,176,43,()=>Navigate(3))};
        foreach(var b in navigation)b.Navigation=true;
        var line=new Panel{BackColor=Palette.Border,Bounds=new Rectangle(27,408,150,1)};sidebar.Controls.Add(line);
        LabelAt(sidebar,"NEED A HAND?",27,430,150,19,8,Palette.Muted,true);
        ButtonAt(sidebar,"Open status page  ↗",15,459,176,38,()=>OpenUrl(Engine.Feed)).Navigation=true;
        ButtonAt(sidebar,"Copy diagnostics",15,504,176,38,CopyDiagnostics).Navigation=true;
        LabelAt(sidebar,"PRIVATE LAN",27,630,150,20,8,Palette.Muted,true);
        sidebarStatus=LabelAt(sidebar,"●  Checking server",27,658,166,23,9,Palette.Gold);
        LabelAt(sidebar,"192.168.10.18",27,684,150,22,9,Palette.Muted);
        LabelAt(sidebar,"PN LAUNCHER  /  2.2",27,719,160,18,7.5f,Palette.Muted);

        heading=LabelAt(this,"Welcome, adventurer.",234,24,620,37,20,Palette.Text,true);
        subheading=LabelAt(this,"A little preparation. A whole world to explore.",235,65,780,25,10,Palette.Muted);
        overview=new Panel{Bounds=new Rectangle(234,108,798,608),BackColor=Palette.Background};Controls.Add(overview);
        var hero=new RealmArt{Bounds=new Rectangle(0,0,798,240)};overview.Controls.Add(hero);
        LabelAt(hero,"PN RAGNAROK  /  A WORLD OF ADVENTURE",25,24,430,20,8,Palette.Gold,true);
        LabelAt(hero,"Return to Midgard",22,57,505,54,27,Palette.Text,true);
        LabelAt(hero,"Gather your party. Rediscover your world.",25,119,410,26,10.5f,Color.FromArgb(192,204,211));
        play=ButtonAt(hero,"Play Ragnarok  →",25,169,202,46,Play,true);
        play.Font=new Font("Segoe UI",11,FontStyle.Bold);operationButtons.Add(play);
        LabelAt(hero,"Checks for updates before launch",241,184,288,20,8.5f,Palette.Muted);
        tips.SetToolTip(play,"Verify the signed release, download changed files, then start Ragnarok. Enter also plays.");

        var serverCard=CardAt(overview,0,254,258,101);
        LabelAt(serverCard,"SERVER CONNECTION",16,13,209,18,7.5f,Palette.Muted,true);
        serverValue=LabelAt(serverCard,"Checking…",16,38,215,27,13,Palette.Gold,true);
        serverDetail=LabelAt(serverCard,"Connecting to the PN LAN",16,72,235,18,8,Palette.Muted);
        refresh=ButtonAt(serverCard,"↻",219,12,27,25,RefreshStatus);refresh.Font=new Font("Segoe UI",13);tips.SetToolTip(refresh,"Refresh LAN server status");
        var clientCard=CardAt(overview,270,254,258,101);
        LabelAt(clientCard,"INSTALLED CLIENT",16,13,225,18,7.5f,Palette.Muted,true);
        clientValue=LabelAt(clientCard,"",16,38,225,28,11.5f,Palette.Text,true);
        clientDetail=LabelAt(clientCard,"",16,72,225,18,8,Palette.Muted);
        var diskCard=CardAt(overview,540,254,258,101);
        LabelAt(diskCard,"AVAILABLE SPACE",16,13,225,18,7.5f,Palette.Muted,true);
        diskValue=LabelAt(diskCard,"",16,38,225,27,13,Palette.Text,true);
        LabelAt(diskCard,"Updates keep a local recovery copy",16,72,232,18,8,Palette.Muted);

        var patch=CardAt(overview,0,369,798,143);
        phase=LabelAt(patch,"Ready when you are",18,15,590,27,12,Palette.Text,true);
        percent=LabelAt(patch,"",688,17,90,22,10,Palette.Gold,true);percent.TextAlign=ContentAlignment.TopRight;
        detail=LabelAt(patch,"Play or check for updates to verify your client.",18,47,760,22,9,Palette.Muted);
        bar=new PatchBar{Bounds=new Rectangle(18,79,762,6)};patch.Controls.Add(bar);
        transfer=LabelAt(patch,"Only changed files are downloaded.",18,99,491,27,8.5f,Palette.Muted);
        cancel=ButtonAt(patch,"Cancel",685,96,95,31,()=>{if(canCancel&&cancellation!=null){cancellation.Cancel();cancel.Enabled=false;detail.Text="Stopping safely…";}});cancel.Visible=false;
        tips.SetToolTip(cancel,"Stop checking or downloading. Installation finishes once it has started.");

        var update=ButtonAt(overview,"Check for updates",0,526,174,39,()=>Run(()=>engine.Update(),false,"Checking for updates",true));operationButtons.Add(update);
        var repair=ButtonAt(overview,"Verify & repair",186,526,163,39,()=>Run(()=>engine.Update(true),false,"Verifying your client",true));operationButtons.Add(repair);
        rollback=ButtonAt(overview,"Restore previous",361,526,163,39,()=>Run(()=>engine.Rollback(),false,"Restoring previous version",false));operationButtons.Add(rollback);
        var localPlay=ButtonAt(overview,"Play installed client",536,526,262,39,()=>Run(()=>engine.VerifyInstalled(),true,"Checking your installed client",true));operationButtons.Add(localPlay);
        tips.SetToolTip(localPlay,"Start your installed version when the patch server is unavailable. Local game files are checked first.");
        tips.SetToolTip(update,"Check every signed client file and install only missing or changed files. F5 also checks.");
        tips.SetToolTip(repair,"Read all client checksums again and repair missing or changed files. Personal settings are preserved.");
        tips.SetToolTip(rollback,"Restore the previous version saved by an update. Repair keeps this backup.");
        LabelAt(overview,"QUICK ACCESS",0,587,111,18,7.5f,Palette.Muted,true);
        toolButtons.Add(ButtonAt(overview,"Game settings",123,578,144,30,OpenGameSettings));
        toolButtons.Add(ButtonAt(overview,"Turbo setup",279,578,136,30,()=>LaunchTool("PNTurboConfig.exe","Turbo setup")));
        ButtonAt(overview,"Screenshots  ↗",427,578,167,30,()=>OpenFolder("ScreenShot"));
        ButtonAt(overview,"Client folder  ↗",606,578,192,30,()=>OpenFolder(""));

        notesPage=new Panel{Bounds=overview.Bounds,BackColor=Palette.Background,Visible=false};Controls.Add(notesPage);
        var notesCard=CardAt(notesPage,0,0,798,608);
        LabelAt(notesCard,"YOUR INSTALLED RELEASE",24,22,700,22,8,Palette.Gold,true);
        releaseSummary=LabelAt(notesCard,"",24,56,750,65,17,Palette.Text,true);
        notes=TextArea(notesCard,24,132,750,446);
        activityPage=new Panel{Bounds=overview.Bounds,BackColor=Palette.Background,Visible=false};Controls.Add(activityPage);
        var historyCard=CardAt(activityPage,0,0,798,608);
        LabelAt(historyCard,"LAUNCHER ACTIVITY",24,22,720,24,9,Palette.Gold,true);
        LabelAt(historyCard,"Checks, updates and useful details in one place.",24,57,725,28,12,Palette.Text,true);
        history=TextArea(historyCard,24,105,750,441);
        ButtonAt(historyCard,"Copy activity",24,558,157,31,()=>CopyText(history.Text,"Activity copied to clipboard."));
        ButtonAt(historyCard,"Open log file  ↗",193,558,167,31,()=>OpenLog());
        settingsPage=new Panel{Bounds=overview.Bounds,BackColor=Palette.Background,Visible=false};Controls.Add(settingsPage);
        var displayCard=CardAt(settingsPage,0,0,798,212);
        LabelAt(displayCard,"MAKE MIDGARD YOUR OWN",24,18,700,20,8,Palette.Gold,true);
        LabelAt(displayCard,"Display & sound",24,49,700,31,18,Palette.Text,true);
        displaySummary=LabelAt(displayCard,GameSettings.Summary(engine.Root),24,90,745,23,10,Palette.Text);
        LabelAt(displayCard,"Resolution, window mode, graphics device, music and effects.",24,119,745,24,9,Palette.Muted);
        toolButtons.Add(ButtonAt(displayCard,"Open game settings",24,157,210,37,OpenGameSettings,true));
        toolButtons.Add(ButtonAt(displayCard,"Original setup",246,157,154,37,()=>LaunchTool("Setup.exe","Original setup")));
        bool openSetup=File.Exists(Path.Combine(engine.Root,"PNOpenSetup.exe"));
        LabelAt(displayCard,openSetup?"OpenSetup 3.5 · Lua edition":"Original Setup available",422,168,349,22,8.5f,openSetup?Palette.Green:Palette.Muted);
        var tools=CardAt(settingsPage,0,226,390,152);
        LabelAt(tools,"PLAY YOUR WAY",20,17,350,20,8,Palette.Gold,true);
        LabelAt(tools,"Keys & Turbo",20,45,345,26,14,Palette.Text,true);
        LabelAt(tools,"Configure your PN Turbo keys and timing.",20,80,350,22,9,Palette.Muted);
        toolButtons.Add(ButtonAt(tools,"Configure Turbo",20,111,174,30,()=>LaunchTool("PNTurboConfig.exe","Turbo setup")));
        var backup=CardAt(settingsPage,406,226,392,152);
        LabelAt(backup,"A LITTLE PEACE OF MIND",20,17,350,20,8,Palette.Gold,true);
        LabelAt(backup,"Save your setup",20,45,345,26,14,Palette.Text,true);
        LabelAt(backup,"Snapshot display, hotkeys, chat and Turbo.",20,80,350,22,9,Palette.Muted);
        toolButtons.Add(ButtonAt(backup,"Back up settings",20,111,174,30,BackupSettings));
        ButtonAt(backup,"View backups",206,111,164,30,()=>OpenFolder(".pn-updater/settings-backups"));
        var prefs=CardAt(settingsPage,0,392,798,150);
        LabelAt(prefs,"LAUNCHER PREFERENCES",24,16,700,20,8,Palette.Gold,true);
        var auto=new CheckBox{Text="Refresh server status automatically (every 30 seconds)",Bounds=new Rectangle(24,49,740,26),ForeColor=Palette.Text,BackColor=Palette.Card,Checked=preferences.autoRefresh};prefs.Controls.Add(auto);
        auto.CheckedChanged+=(s,e)=>{preferences.autoRefresh=auto.Checked;if(!preview){if(auto.Checked){statusTimer.Start();RefreshStatus();}else statusTimer.Stop();SavePreferences();}};
        var minimize=new CheckBox{Text="Minimize the launcher when Ragnarok starts",Bounds=new Rectangle(24,78,740,26),ForeColor=Palette.Text,BackColor=Palette.Card,Checked=preferences.minimizeOnPlay};prefs.Controls.Add(minimize);
        minimize.CheckedChanged+=(s,e)=>{preferences.minimizeOnPlay=minimize.Checked;SavePreferences();};
        var checkOnOpen=new CheckBox{Text="Check for updates when the launcher opens",Bounds=new Rectangle(24,107,740,26),ForeColor=Palette.Text,BackColor=Palette.Card,Checked=preferences.checkOnOpen};prefs.Controls.Add(checkOnOpen);
        checkOnOpen.CheckedChanged+=(s,e)=>{preferences.checkOnOpen=checkOnOpen.Checked;SavePreferences();};
        ButtonAt(settingsPage,"Saved game files  ↗",0,556,200,32,()=>OpenFolder("savedata"));
        ButtonAt(settingsPage,"Screenshots  ↗",212,556,176,32,()=>OpenFolder("ScreenShot"));
        ButtonAt(settingsPage,"Replays  ↗",400,556,166,32,()=>OpenFolder("Replay"));
        settingsNotice=LabelAt(settingsPage,"Local preferences never change your server connection.",0,593,798,18,8,Palette.Muted);
        LabelAt(this,"Your settings, screenshots and replays are preserved during updates.",234,723,770,18,8,Palette.Muted);
        ReadActivity();LoadInstalled();Navigate(0);RefreshControls();
        uiTimer.Tick+=(s,e)=>FlushProgress();uiTimer.Start();
        statusTimer.Tick+=(s,e)=>RefreshStatus();
        Shown+=(s,e)=>{if(preview)return;FitScreen();
            if(preferences.checkOnOpen&&File.Exists(Path.Combine(engine.Work,"installed.json"))&&Process.GetProcessesByName("Ragexe").Length==0)BeginStartupCheck(()=>engine.Update());
            else Run(()=>{engine.Recover();return "Choose Play to update, or Play installed client to use your local version.";},false,"Getting ready",false);
            RefreshStatus();if(preferences.autoRefresh)statusTimer.Start();};
        KeyDown+=(s,e)=>{if(e.KeyCode==Keys.F5){if(!busy)Run(()=>engine.Update(),false,"Checking for updates",true);e.Handled=true;}
            if(e.KeyCode==Keys.Escape&&canCancel&&cancellation!=null){cancellation.Cancel();cancel.Enabled=false;e.Handled=true;}};
        AcceptButton=play;
        FormClosing+=(s,e)=>{if(busy){e.Cancel=true;Navigate(0);if(canCancel&&cancellation!=null){closeAfterCancel=true;cancellation.Cancel();detail.Text="Stopping safely before closing…";}
            else detail.Text="Finishing the file operation. You can close the launcher when it completes.";}else closing=true;};
    }

    TextBox TextArea(Control parent,int x,int y,int width,int height){var text=new TextBox{Bounds=new Rectangle(x,y,width,height),ReadOnly=true,Multiline=true,BorderStyle=BorderStyle.None,
        BackColor=Palette.Card,ForeColor=Palette.Muted,Font=new Font("Segoe UI",10),ScrollBars=ScrollBars.Vertical};parent.Controls.Add(text);return text;}
    void FitScreen(){var area=Screen.FromControl(this).WorkingArea;
        if(Width<=area.Width-24&&Height<=area.Height-24)return;
        AutoScroll=true;AutoScrollMinSize=ClientSize;Size=new Size(Math.Min(Width,area.Width-24),Math.Min(Height,area.Height-24));
        Location=new Point(area.Left+(area.Width-Width)/2,area.Top+(area.Height-Height)/2);
    }
    void Navigate(int page){overview.Visible=page==0;notesPage.Visible=page==1;activityPage.Visible=page==2;settingsPage.Visible=page==3;
        if(page==3)displaySummary.Text=GameSettings.Summary(engine.Root);
        for(int i=0;i<navigation.Length;i++){navigation[i].Selected=i==page;navigation[i].Invalidate();}
        heading.Text=page==0?"Welcome, adventurer.":page==1?"A world that keeps growing.":page==2?"Your launcher activity.":"Settle in. Make it yours.";
        subheading.Text=page==0?"A little preparation. A whole world to explore.":page==1?"What's included in the client installed on this PC.":page==2?"Recent checks and updates, ready when you need them.":"Game settings and a few thoughtful launcher preferences.";
        AcceptButton=page==0?play:null;
    }
    internal static string FriendlyRelease(string release){
        if(String.IsNullOrEmpty(release)||release=="Client installation needed")return "Installation needed";
        var parts=release.Split('-');if(parts.Length>=3&&parts[0]=="client")return System.Globalization.CultureInfo.InvariantCulture.TextInfo.ToTitleCase(String.Join(" ",parts.Skip(2).ToArray()));
        return release;
    }
    void LoadInstalled(){string installed=engine.Installed();installedRelease=installed;clientValue.Text=FriendlyRelease(installed);tips.SetToolTip(clientValue,installed);
        clientDetail.Text=installed=="Client installation needed"?"Choose Update for a fresh install":(lastCheck.HasValue?"Checked "+lastCheck.Value.ToString("HH:mm"):"Update check recommended");
        releaseSummary.Text=FriendlyRelease(installed);var text=new StringBuilder();text.AppendLine(installed).AppendLine();
        try{string path=Path.Combine(engine.Root,"RELEASE.json");Engine.NoLinks(path);var json=new JavaScriptSerializer {MaxJsonLength=8000000}.Deserialize<Dictionary<string,object>>(File.ReadAllText(path));
            object fixes;if(json.TryGetValue("fixes",out fixes)){var entries=fixes as System.Collections.IEnumerable;if(entries!=null)foreach(var item in entries)text.Append("•  ").AppendLine(Convert.ToString(item)).AppendLine();}
            else text.AppendLine("This release does not include a local change list.");
        }catch{text.AppendLine("Install or update the client to see its release notes here.");}
        notes.Text=text.ToString();try{diskValue.Text=FormatBytes(new DriveInfo(Path.GetPathRoot(engine.Root)).AvailableFreeSpace)+" free";}catch{diskValue.Text="Unavailable";}
    }
    void RefreshControls(){foreach(var b in operationButtons)b.Enabled=!busy;foreach(var b in toolButtons)b.Enabled=!busy;
        play.Enabled=!busy||(startupCheck&&!playAfterCheck&&!closeAfterCancel);
        bool saved=File.Exists(Path.Combine(engine.Work,"rollback.json"));rollback.Enabled=!busy&&saved;
        tips.SetToolTip(rollback,saved?"Restore the version saved before your last update.":"No previous version is stored yet. A completed version update saves one.");
        cancel.Visible=busy&&canCancel;cancel.Enabled=canCancel&&cancellation!=null&&!cancellation.IsCancellationRequested;
    }
    void Post(Action action){if(closing||IsDisposed||!IsHandleCreated)return;try{BeginInvoke((Action)(()=>{if(!closing&&!IsDisposed)action();}));}catch(InvalidOperationException){}}
    void RefreshStatus(){if(preview||Interlocked.CompareExchange(ref statusPending,1,0)!=0)return;refresh.Enabled=false;
        ThreadPool.QueueUserWorkItem(_=>{bool online=false;string value,info;Color color;
            try{var json=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(Engine.Text(Engine.Feed+"status.json",statusCancellation.Token,2500,1));object raw;
                online=json.TryGetValue("game_online",out raw)&&raw is bool&&(bool)raw;
                value=online?"●  Online":"●  Services offline";color=online?Palette.Green:Palette.Gold;
                info="Checked "+DateTime.Now.ToString("HH:mm")+"  ·  PN LAN";
                object services;if(!online&&json.TryGetValue("services",out services)){var map=services as Dictionary<string,object>;
                    if(map!=null){var unavailable=map.Where(p=>p.Value is bool&&!(bool)p.Value).Select(p=>p.Key).ToArray();if(unavailable.Length>0)info="Unavailable: "+String.Join(", ",unavailable);}}
            }catch{value="●  Cannot reach LAN";color=Palette.Red;info="Connect to the PN network and retry";}
            Interlocked.Exchange(ref statusPending,0);Post(()=>{connection=value;serverValue.Text=value;serverValue.ForeColor=color;serverDetail.Text=info;tips.SetToolTip(serverDetail,info);
                sidebarStatus.Text=online?"●  Server online":"●  Check connection";sidebarStatus.ForeColor=color;refresh.Enabled=true;});
        });
    }
    void Report(string text,int progress){lock(progressLock){pendingText=text;if(progress>=0)pendingPercent=progress;}}
    void ReportTransfer(long bytes,long total,double speed){lock(progressLock){pendingBytes=bytes;pendingTotal=total;pendingSpeed=speed;transferPending=true;}}
    void FlushProgress(){string text;int progress;long bytes,total;double speed;bool downloaded;
        lock(progressLock){text=pendingText;progress=pendingPercent;bytes=pendingBytes;total=pendingTotal;speed=pendingSpeed;downloaded=transferPending;pendingText=null;pendingPercent=-1;transferPending=false;}
        if(text!=null){detail.Text=text;tips.SetToolTip(detail,text);}
        if(progress>=0){bar.Value=progress;percent.Text=progress+"%";if(progress>=90&&busy){canCancel=false;RefreshControls();}}
        if(downloaded){if(total>0){bar.Value=45+(int)Math.Min(44,bytes*44.0/total);percent.Text=bar.Value+"%";}
            double seconds=speed>0?(total-bytes)/speed:0;
            transfer.Text=FormatBytes(bytes)+" / "+FormatBytes(total)+"  ·  "+FormatBytes((long)speed)+"/s"+(seconds>1?"  ·  about "+(seconds<60?Math.Ceiling(seconds)+" sec":Math.Ceiling(seconds/60)+" min")+" left":"");}
    }
    internal static string FormatBytes(long value){double n=Math.Max(0,value);string[] units={"B","KiB","MiB","GiB","TiB"};int unit=0;while(n>=1024&&unit<units.Length-1){n/=1024;unit++;}return n.ToString(unit==0?"0":"0.0")+" "+units[unit];}
    void Play(){if(busy){if(startupCheck){playAfterCheck=true;play.Enabled=false;phase.Text="Preparing to start Ragnarok";detail.Text="Ragnarok will start when this check finishes.";}return;}
        Run(()=>engine.Update(),true,"Preparing your adventure",true);}
    internal void BeginStartupCheck(Func<string> check){startupCheck=true;Run(check,false,"Checking and preparing your client",true);}
    void Run(Func<string> action,bool start,string title,bool cancellable){if(busy)return;Navigate(0);busy=true;canCancel=cancellable;
        cancellation=new CancellationTokenSource();engine.Cancellation=cancellation.Token;phase.Text=title;phase.ForeColor=Palette.Text;detail.Text="Preparing…";bar.Value=0;percent.Text="0%";
        transfer.Text=cancellable?"Your settings, screenshots and replays are kept.":"Preparing the local client.";RefreshControls();AppendActivity(title+".");
        var worker=new BackgroundWorker();worker.DoWork+=(s,e)=>e.Result=action();worker.RunWorkerCompleted+=(s,e)=>{
            bool launch=start||playAfterCheck;startupCheck=false;playAfterCheck=false;
            FlushProgress();busy=false;canCancel=false;engine.Cancellation=CancellationToken.None;cancellation.Dispose();cancellation=null;cancel.Visible=false;
            if(e.Error is OperationCanceledException){phase.Text="Check cancelled";phase.ForeColor=Palette.Gold;detail.Text="No update was installed. Check again whenever you're ready.";transfer.Text="Existing client files are unchanged.";bar.Value=0;percent.Text="";lastResult="Cancelled before installation.";}
            else if(e.Error!=null){phase.Text="Let's get you back on track";phase.ForeColor=Palette.Red;detail.Text=e.Error.Message;transfer.Text="Retry after checking your connection, free space and that the game is closed.";bar.Value=0;percent.Text="";lastResult=e.Error.Message;tips.SetToolTip(detail,e.Error.Message);}
            else{phase.Text=launch?"Your adventure is ready":"Ready when you are";phase.ForeColor=Palette.Green;detail.Text=(string)e.Result;lastResult=detail.Text;tips.SetToolTip(detail,detail.Text);bar.Value=100;percent.Text="100%";
                transfer.Text=cancellable?"Signed release verified. Your personal files are preserved.":"Local client ready.";if(cancellable)lastCheck=DateTime.Now;
                if(launch&&!closeAfterCancel)try{string path=Path.Combine(engine.Root,"Ragexe.exe");Engine.NoLinks(path);StartClient(path);phase.Text="Ragnarok launched";if(preferences.minimizeOnPlay)WindowState=FormWindowState.Minimized;}
                    catch(Exception ex){phase.Text="Could not start Ragnarok";phase.ForeColor=Palette.Red;detail.Text=ex.Message;lastResult=ex.Message;}
            }
            AppendActivity(lastResult);LoadInstalled();RefreshControls();worker.Dispose();if(closeAfterCancel)Close();
        };worker.RunWorkerAsync();
    }
    void SavePreferences(){if(preview)return;try{engine.Write(preferencesPath,new JavaScriptSerializer().Serialize(preferences));settingsNotice.Text="Launcher preferences saved.";}catch(Exception ex){ShowActionError("Save preferences",ex);}}
    void OpenGameSettings(){try{
        if(Process.GetProcessesByName("Ragexe").Length>0)throw new IOException("Close Ragnarok before changing game settings.");
        LaunchTool(GameSettings.Tool(engine.Root),"Game settings");
    }catch(Exception ex){ShowActionError("Game settings",ex);}}
    void BackupSettings(){try{string path=GameSettings.Backup(engine);settingsNotice.Text="Settings backed up · "+Path.GetFileName(path);tips.SetToolTip(settingsNotice,path);AppendActivity("Saved game settings backup: "+Path.GetFileName(path));}catch(Exception ex){ShowActionError("Back up settings",ex);}}
    void LaunchTool(string filename,string title){try{string path=Path.Combine(engine.Root,filename);Engine.NoLinks(path);if(!File.Exists(path))throw new IOException("Check for updates to install "+title+".");
        Process.Start(new ProcessStartInfo(path){WorkingDirectory=engine.Root,UseShellExecute=true});AppendActivity("Opened "+title+".");}catch(Exception ex){ShowActionError(title,ex);}}
    void OpenUrl(string url){try{Process.Start(new ProcessStartInfo(url){UseShellExecute=true});}catch(Exception ex){ShowActionError("Status page",ex);}}
    void OpenFolder(string folder){try{string path=Path.Combine(engine.Root,folder);Engine.NoLinks(path);if(!Directory.Exists(path))throw new IOException("This folder is not available yet. It appears after you save files or create a backup.");
        Process.Start(new ProcessStartInfo("explorer.exe","\""+path+"\""){UseShellExecute=true});}catch(Exception ex){ShowActionError("Open folder",ex);}}
    void ShowActionError(string title,Exception ex){AppendActivity(title+": "+ex.Message);MessageBox.Show(this,ex.Message,title,MessageBoxButtons.OK,MessageBoxIcon.Information);}
    void CopyText(string text,string result){try{Clipboard.SetText(String.IsNullOrEmpty(text)?"No launcher activity yet.":text);AppendActivity(result);}catch(Exception ex){ShowActionError("Clipboard",ex);}}
    void CopyDiagnostics(){CopyText("PN Ragnarok launcher 2.2"+Environment.NewLine+"Client: "+installedRelease+Environment.NewLine+"Folder: "+engine.Root+Environment.NewLine+
        "Server: "+connection+Environment.NewLine+"Status: "+Engine.Feed+Environment.NewLine+"Free space: "+diskValue.Text+Environment.NewLine+"Last result: "+lastResult+Environment.NewLine+
        "Saved rollback: "+File.Exists(Path.Combine(engine.Work,"rollback.json"))+Environment.NewLine+Environment.NewLine+history.Text,"Diagnostics copied to clipboard.");}
    string LogPath{get{return Path.Combine(engine.Work,"launcher.log");}}
    void ReadActivity(){try{Engine.NoLinks(LogPath);if(File.Exists(LogPath))activity.AddRange(File.ReadAllLines(LogPath).TakeLastCompatible(160));}catch{}history.Text=String.Join(Environment.NewLine,activity);}
    void AppendActivity(string text){activity.Add(DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss")+"  "+text.Replace('\r',' ').Replace('\n',' '));while(activity.Count>160)activity.RemoveAt(0);
        history.Text=String.Join(Environment.NewLine,activity);if(activityPage.Visible){history.SelectionStart=history.Text.Length;history.ScrollToCaret();}
        if(!preview)try{engine.Write(LogPath,history.Text+Environment.NewLine);}catch{ /* Logging must not prevent launching or recovery. */ }
    }
    void OpenLog(){try{Engine.NoLinks(LogPath);if(!File.Exists(LogPath))AppendActivity("Activity log opened.");Process.Start(new ProcessStartInfo("notepad.exe","\""+LogPath+"\""){UseShellExecute=true});}catch(Exception ex){ShowActionError("Activity log",ex);}}
    // Preview states exercise the same controls without network calls or updates.
    internal void PreviewState(string state){if(state=="downloading"){phase.Text="Downloading your update";detail.Text="Downloading data.grf";bar.Value=67;percent.Text="67%";transfer.Text="512.0 MiB / 1.0 GiB  ·  24.0 MiB/s  ·  about 22 sec left";busy=true;canCancel=true;cancellation=new CancellationTokenSource();RefreshControls();}
        if(state=="error"){phase.Text="Let's get you back on track";phase.ForeColor=Palette.Red;detail.Text="Unable to connect to the update server.";transfer.Text="Connect to the PN network, then check for updates again.";
            serverValue.Text="●  Cannot reach LAN";serverValue.ForeColor=Palette.Red;serverDetail.Text="Connect to the PN network and retry";sidebarStatus.Text="●  Check connection";sidebarStatus.ForeColor=Palette.Red;}
        if(state=="settings")Navigate(3);if(state=="notes")Navigate(1);if(state=="activity"){AppendActivity("Signed release verified. Client is up to date.");Navigate(2);}
        if(state=="online"||state=="downloading"){serverValue.Text="●  Online";serverValue.ForeColor=Palette.Green;serverDetail.Text="Checked just now  ·  PN LAN";sidebarStatus.Text="●  Server online";sidebarStatus.ForeColor=Palette.Green;}
    }
    internal void ScalePreview(float scale){var fonts=new Dictionary<Control,Font>();CaptureFonts(this,fonts);Scale(new SizeF(scale,scale));
        foreach(var item in fonts)item.Key.Font=new Font(item.Value.FontFamily,item.Value.Size*scale,item.Value.Style);
    }
    void CaptureFonts(Control parent,Dictionary<Control,Font> fonts){fonts[parent]=parent.Font;foreach(Control child in parent.Controls)CaptureFonts(child,fonts);}
    protected override void Dispose(bool disposing){if(disposing){statusCancellation.Cancel();statusTimer.Dispose();uiTimer.Dispose();tips.Dispose();engine.Dispose();if(cancellation!=null)cancellation.Dispose();if(brandMark.Image!=null){brandMark.Image.Dispose();brandMark.Image=null;}}base.Dispose(disposing);}
}

static class EnumerableCompatibility {
    public static IEnumerable<T> TakeLastCompatible<T>(this IEnumerable<T> source,int count){var list=source.ToList();return list.Skip(Math.Max(0,list.Count-count));}
}
