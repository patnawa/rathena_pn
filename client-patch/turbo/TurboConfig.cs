// PN Turbo Setup. GPL-3.0-or-later. .NET Framework 4.x.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
class TurboBinding {public string Key="";public int Delay=10;}
class TurboSettings {
    public bool DefaultState,AltRight;public string PauseKey="P";public int Delay=10,RightDelay=100;
    public TurboBinding[] Smart=Enumerable.Range(0,10).Select(i=>new TurboBinding()).ToArray(),Repeat=Enumerable.Range(0,10).Select(i=>new TurboBinding()).ToArray();
    public static readonly string[] Keys=new[]{""}.Concat(Enumerable.Range(1,12).Select(i=>"F"+i)).Concat(Enumerable.Range('A',26).Select(i=>((char)i).ToString())).Concat(Enumerable.Range(0,10).Select(i=>i.ToString())).ToArray();
    public static TurboSettings Load(string path){
        var settings=new TurboSettings();if(!File.Exists(path)){for(int i=0;i<3;i++){settings.Smart[i].Key="F"+(i+1);settings.Repeat[i].Key="F"+(i+5);}return settings;}
        var values=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);string section="";
        foreach(string raw in File.ReadAllLines(path)){string line=raw.Trim();if(line.Length==0||line.StartsWith(";")||line.StartsWith("#"))continue;
            if(line.StartsWith("[")&&line.EndsWith("]")){section=line.Substring(1,line.Length-2);continue;}
            int split=line.IndexOf('=');if(split>=0)values[section+"/"+line.Substring(0,split).Trim()]=line.Substring(split+1).Trim();}
        Func<string,string,string> get=(key,otherwise)=>values.ContainsKey(key)?values[key]:otherwise;
        settings.DefaultState=get("General/DefaultState","0")=="1";settings.PauseKey=get("General/PauseKey","P").ToUpperInvariant();
        settings.Delay=Number(get("General/DelayMs","10"));settings.AltRight=get("AltRightClick/Enabled","0")=="1";settings.RightDelay=Number(get("AltRightClick/DelayMs","100"));
        for(int group=0;group<2;group++)for(int i=0;i<10;i++){
            string prefix=group==0?"SmartKeys/":"TurboKeys/";var item=(group==0?settings.Smart:settings.Repeat)[i];
            item.Key=get(prefix+"Key"+(i+1),"").ToUpperInvariant();item.Delay=Number(get(prefix+"Delay"+(i+1),settings.Delay.ToString()));}
        settings.Validate();return settings;
    }
    static int Number(string value){int n;if(!Int32.TryParse(value,out n)||n<10||n>5000)throw new InvalidDataException("Delay must be between 10 and 5000 ms.");return n;}
    public void Validate(){
        if(String.IsNullOrEmpty(PauseKey)||!Keys.Contains(PauseKey))throw new InvalidDataException("Choose a valid toggle key.");
        Number(Delay.ToString());Number(RightDelay.ToString());var used=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach(var item in Smart.Concat(Repeat)){if(!Keys.Contains(item.Key))throw new InvalidDataException("Unsupported key: "+item.Key);Number(item.Delay.ToString());if(item.Key!=""&&!used.Add(item.Key))throw new InvalidDataException(item.Key+" is assigned more than once. Use one mode per key.");}
    }
    public string Text(){Validate();var b=new StringBuilder();b.AppendLine("; PN Turbo - settings reload while the game is running.");
        b.AppendLine("[General]").AppendLine("PauseKey="+PauseKey).AppendLine("DefaultState="+(DefaultState?1:0)).AppendLine("DelayMs="+Delay);
        for(int group=0;group<2;group++){b.AppendLine().AppendLine(group==0?"[SmartKeys]":"[TurboKeys]");var rows=group==0?Smart:Repeat;
            for(int i=0;i<rows.Length;i++)b.AppendLine("Key"+(i+1)+"="+rows[i].Key).AppendLine("Delay"+(i+1)+"="+rows[i].Delay);}
        b.AppendLine().AppendLine("[AltRightClick]").AppendLine("Enabled="+(AltRight?1:0)).AppendLine("DelayMs="+RightDelay);return b.ToString();
    }
    public void Save(string path){string temp=path+".new";
        foreach(string file in new[]{path,temp})if(File.Exists(file)&&(File.GetAttributes(file)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked settings files are not supported.");
        byte[] body=Encoding.Unicode.GetBytes(Text());using(var stream=new FileStream(temp,FileMode.Create,FileAccess.Write,FileShare.None)){byte[] bom=Encoding.Unicode.GetPreamble();stream.Write(bom,0,bom.Length);stream.Write(body,0,body.Length);stream.Flush(true);}
        if(File.Exists(path))File.Replace(temp,path,null);else File.Move(temp,path);
    }
}
class TurboForm:Form {
    string path;CheckBox enabled=new CheckBox(),right=new CheckBox();NumericUpDown rightDelay=new NumericUpDown();ComboBox pause=new ComboBox();Label status=new Label();
    DataGridView smart=Grid(),repeat=Grid();
    static DataGridView Grid(){var grid=new DataGridView{Dock=DockStyle.Fill,AllowUserToAddRows=false,AllowUserToDeleteRows=false,AutoSizeColumnsMode=DataGridViewAutoSizeColumnsMode.Fill,RowHeadersVisible=false,BackgroundColor=Color.White,BorderStyle=BorderStyle.None};
        var key=new DataGridViewComboBoxColumn{HeaderText="Hold this key",Name="Key",FillWeight=60};key.Items.AddRange(TurboSettings.Keys);grid.Columns.Add(key);grid.Columns.Add(new DataGridViewTextBoxColumn{HeaderText="Repeat delay (ms)",Name="Delay",FillWeight=40});for(int i=0;i<10;i++)grid.Rows.Add("",10);return grid;}
    public TurboForm(string file){path=file;Text="PN Turbo Setup";ClientSize=new Size(560,545);FormBorderStyle=FormBorderStyle.FixedDialog;MaximizeBox=false;StartPosition=FormStartPosition.CenterScreen;Font=new Font("Segoe UI",10);
        var title=new Label{Text="Hold a key. Repeat its action.",Font=new Font("Segoe UI",16,FontStyle.Bold),Location=new Point(18,14),Size=new Size(525,35)};
        var help=new Label{Text="Smart-cast uses your mouse target. Key repeat is for self skills and items.\nEnter or Esc pauses turbo; Alt + the toggle key enables it again.",Location=new Point(20,56),Size=new Size(525,50)};
        enabled.Text="Enable when the game starts";enabled.SetBounds(20,108,320,26);
        var toggle=new Label{Text="Toggle: Alt +",Location=new Point(350,110),Size=new Size(90,26)};pause.DropDownStyle=ComboBoxStyle.DropDownList;pause.Items.AddRange(TurboSettings.Keys.Where(k=>k!="").ToArray());pause.SetBounds(440,108,90,28);
        var tabs=new TabControl{Location=new Point(20,145),Size=new Size(520,285)};var a=new TabPage("Smart-cast");var b=new TabPage("Key repeat");a.Controls.Add(smart);b.Controls.Add(repeat);tabs.TabPages.AddRange(new[]{a,b});
        right.Text="Repeat Alt + right-click";right.SetBounds(20,441,255,28);rightDelay.Minimum=10;rightDelay.Maximum=5000;rightDelay.SetBounds(325,441,85,28);var unit=new Label{Text="ms delay",Location=new Point(418,444),Size=new Size(110,25)};
        var save=new Button{Text="Save settings",Location=new Point(390,489),Size=new Size(150,34)};var import=new Button{Text="Import settings",Location=new Point(20,489),Size=new Size(140,34)};status.SetBounds(172,487,210,48);status.ForeColor=Color.DarkGreen;
        save.Click+=(s,e)=>{try{Read().Save(path);status.Text="Saved. Release held keys\nto use the new settings.";}catch(Exception ex){MessageBox.Show(this,ex.Message,"Turbo settings",MessageBoxButtons.OK,MessageBoxIcon.Warning);}};
        import.Click+=(s,e)=>{using(var dialog=new OpenFileDialog{Filter="Turbo settings (*.ini)|*.ini",Title="Import turbo key settings"})if(dialog.ShowDialog(this)==DialogResult.OK)try{ShowSettings(TurboSettings.Load(dialog.FileName));status.Text="Imported. Click Save to apply.";}catch(Exception ex){MessageBox.Show(this,ex.Message,"Import settings");}};
        Controls.AddRange(new Control[]{title,help,enabled,toggle,pause,tabs,right,rightDelay,unit,save,import,status});ShowSettings(TurboSettings.Load(path));
    }
    void ShowSettings(TurboSettings s){enabled.Checked=s.DefaultState;right.Checked=s.AltRight;rightDelay.Value=s.RightDelay;pause.SelectedItem=s.PauseKey;
        for(int i=0;i<10;i++){smart.Rows[i].Cells[0].Value=s.Smart[i].Key;smart.Rows[i].Cells[1].Value=s.Smart[i].Delay;repeat.Rows[i].Cells[0].Value=s.Repeat[i].Key;repeat.Rows[i].Cells[1].Value=s.Repeat[i].Delay;}}
    TurboSettings Read(){smart.EndEdit();repeat.EndEdit();var s=new TurboSettings{DefaultState=enabled.Checked,AltRight=right.Checked,RightDelay=(int)rightDelay.Value,PauseKey=Convert.ToString(pause.SelectedItem)};
        for(int i=0;i<10;i++)foreach(bool first in new[]{true,false}){var row=(first?smart:repeat).Rows[i];var item=(first?s.Smart:s.Repeat)[i];item.Key=Convert.ToString(row.Cells[0].Value);int delay;if(!Int32.TryParse(Convert.ToString(row.Cells[1].Value),out delay))throw new InvalidDataException("Enter a whole-number delay in milliseconds.");item.Delay=delay;}s.Validate();return s;}
}
class TurboPreviewForm:TurboForm {
    public TurboPreviewForm(string path):base(path){ShowInTaskbar=false;Opacity=0;}
    protected override bool ShowWithoutActivation {get{return true;}}
}
static class Program {
    [STAThread]static int Main(string[] args){try{
        string path=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"PN-Turbo.ini");
        if(args.Length>0&&args[0]=="--self-test"){
            string test=Path.Combine(Path.GetTempPath(),"pn-turbo-"+Guid.NewGuid().ToString("N")+".ini");
            try{var s=TurboSettings.Load(test);s.Repeat[1].Delay=123;s.Save(test);var loaded=TurboSettings.Load(test);if(loaded.Smart[0].Key!="F1"||loaded.Repeat[1].Delay!=123||loaded.DefaultState)throw new Exception("Round trip failed");loaded.Repeat[0].Key="F1";bool rejected=false;try{loaded.Validate();}catch(InvalidDataException){rejected=true;}if(!rejected)throw new Exception("Duplicate key accepted");Console.WriteLine("PASS: default profile, Unicode/atomic round trip, per-key delay and duplicate rejection");}finally{File.Delete(test);File.Delete(test+".new");}return 0;}
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        if(args.Length>1&&args[0]=="--render-preview"){using(var form=new TurboPreviewForm(path)){form.Show();Application.DoEvents();using(var bitmap=new Bitmap(form.ClientSize.Width,form.ClientSize.Height)){using(var g=Graphics.FromImage(bitmap))g.Clear(form.BackColor);foreach(Control c in form.Controls){IntPtr handle=c.Handle;c.DrawToBitmap(bitmap,c.Bounds);}bitmap.Save(args[1],System.Drawing.Imaging.ImageFormat.Png);}}return 0;}
        Application.Run(new TurboForm(path));return 0;
    }catch(Exception e){if(args.Length>0)Console.Error.WriteLine(e.Message);else MessageBox.Show(e.Message,"PN Turbo Setup");return 1;}}
}
