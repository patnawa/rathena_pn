param([string]$Output=(Join-Path $PSScriptRoot 'assets/poring'))
$ErrorActionPreference='Stop'
if($PSVersionTable.PSEdition -eq 'Core'){
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -Output $Output
    if($LASTEXITCODE){throw 'Alternate icon generation failed'}
    return
}
New-Item -ItemType Directory -Force -Path $Output | Out-Null
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Collections.Generic;
public static class PNPoringIcons {
    static PointF[] Points(params float[] xy){var p=new PointF[xy.Length/2];for(int i=0;i<p.Length;i++)p[i]=new PointF(xy[2*i],xy[2*i+1]);return p;}
    static void Fill(Graphics g,Brush b,params float[] xy){g.FillPolygon(b,Points(xy));}
    static GraphicsPath Gate(){var p=new GraphicsPath();p.AddLine(78,190,78,108);p.AddBezier(78,108,78,76,99,57,128,57);p.AddBezier(128,57,157,57,178,76,178,108);p.AddLine(178,108,178,190);p.CloseFigure();return p;}
    static GraphicsPath Tile(){var p=new GraphicsPath();p.AddArc(12,12,88,88,180,90);p.AddArc(156,12,88,88,270,90);p.AddArc(156,156,88,88,0,90);p.AddArc(12,156,88,88,90,90);p.CloseFigure();return p;}
    static Bitmap Draw(int size,bool game){
        int resolution=Math.Max(64,size*2);var drawing=new Bitmap(resolution,resolution,PixelFormat.Format32bppArgb);
        using(var g=Graphics.FromImage(drawing)){
            g.SmoothingMode=SmoothingMode.AntiAlias;g.PixelOffsetMode=PixelOffsetMode.HighQuality;g.ScaleTransform(resolution/256f,resolution/256f);
            using(var tile=Tile())using(var bg=new LinearGradientBrush(new Rectangle(12,12,232,232),Color.FromArgb(102,63,151),Color.FromArgb(39,31,74),60f))g.FillPath(bg,tile);
            using(var tile=Tile())using(var edge=new Pen(Color.FromArgb(135,206,177,245),2.5f))g.DrawPath(edge,tile);
            if(size>=32)using(var sparkle=new SolidBrush(Color.FromArgb(190,238,224,255))){Fill(g,sparkle,36,75,40,85,50,89,40,93,36,103,32,93,22,89,32,85);Fill(g,sparkle,219,172,222,180,230,183,222,186,219,194,216,186,208,183,216,180);}
            using(var gold=new LinearGradientBrush(new Rectangle(0,28,256,185),Color.FromArgb(255,237,172),Color.FromArgb(238,160,70),85f)){
                if(!game){
                    using(var shadow=new SolidBrush(Color.FromArgb(110,20,14,39)))g.FillEllipse(shadow,54,207,151,25);
                    using(var body=new GraphicsPath()){
                        body.AddBezier(128,76,179,73,203,114,221,165);body.AddBezier(221,165,240,205,211,225,178,223);
                        body.AddBezier(178,223,149,238,104,238,77,223);body.AddBezier(77,223,38,224,20,204,37,165);body.AddBezier(37,165,57,110,80,76,128,76);body.CloseFigure();
                        using(var pink=new LinearGradientBrush(new Rectangle(30,80,200,160),Color.FromArgb(255,190,224),Color.FromArgb(233,96,168),90f))g.FillPath(pink,body);
                        using(var outline=new Pen(Color.FromArgb(153,55,122),size<=24?4:3))g.DrawPath(outline,body);
                    }
                    Fill(g,gold,82,91,72,43,103,58,128,26,154,58,184,43,174,91);
                    using(var rim=new Pen(Color.FromArgb(195,116,42),4)){rim.LineJoin=LineJoin.Round;g.DrawLines(rim,Points(82,91,72,43,103,58,128,26,154,58,184,43,174,91));}
                    using(var b=new SolidBrush(Color.FromArgb(255,225,147)))g.FillRectangle(b,82,80,92,13);
                    using(var gem=new SolidBrush(Color.FromArgb(59,212,228)))Fill(g,gem,128,53,138,67,128,81,118,67);
                    using(var highlight=new SolidBrush(Color.FromArgb(190,255,236,249)))g.FillEllipse(highlight,74,114,34,16);
                    using(var dark=new SolidBrush(Color.FromArgb(59,37,73))){g.FillEllipse(dark,88,148,13,21);g.FillEllipse(dark,155,148,13,21);}
                    if(size>=32)using(var white=new SolidBrush(Color.FromArgb(255,238,255))){g.FillEllipse(white,91,150,4,6);g.FillEllipse(white,158,150,4,6);}
                    using(var cheek=new SolidBrush(Color.FromArgb(165,237,100,161))){g.FillEllipse(cheek,66,171,30,12);g.FillEllipse(cheek,162,171,30,12);}
                    using(var smile=new GraphicsPath()){smile.AddBezier(114,175,120,187,136,187,142,175);using(var pen=new Pen(Color.FromArgb(94,47,87),4)){pen.StartCap=LineCap.Round;pen.EndCap=LineCap.Round;g.DrawPath(pen,smile);}}
                }else{
                    // A crystal and crown silhouette, with soft gold wings.
                    Fill(g,gold,86,115,35,87,46,128,61,149,96,181,87,158,70,139,102,161);
                    Fill(g,gold,170,115,221,87,210,128,195,149,160,181,169,158,186,139,154,161);
                    using(var darkGold=new SolidBrush(Color.FromArgb(182,116,56))){Fill(g,darkGold,48,112,67,143,94,171,88,154,68,134);Fill(g,darkGold,208,112,189,143,162,171,168,154,188,134);}
                    Fill(g,gold,65,91,53,49,94,74,128,29,162,74,203,49,191,91,177,109,79,109);
                    using(var pen=new Pen(Color.FromArgb(191,121,48),size<=24?4:3)){pen.LineJoin=LineJoin.Round;g.DrawPolygon(pen,Points(65,91,53,49,94,74,128,29,162,74,203,49,191,91,177,109,79,109));}
                    Fill(g,gold,128,76,170,119,151,190,128,219,105,190,86,119);
                    using(var blue=new SolidBrush(Color.FromArgb(53,172,216)))Fill(g,blue,128,86,160,121,143,186,128,204,113,186,96,121);
                    using(var light=new SolidBrush(Color.FromArgb(183,253,254)))Fill(g,light,128,86,128,139,96,121);
                    using(var cyan=new SolidBrush(Color.FromArgb(100,234,243)))Fill(g,cyan,128,86,160,121,128,139);
                    using(var deep=new SolidBrush(Color.FromArgb(38,106,175)))Fill(g,deep,128,139,160,121,143,186,128,204);
                    using(var soft=new SolidBrush(Color.FromArgb(95,221,237)))Fill(g,soft,96,121,128,139,128,204,113,186);
                    if(size>=32)using(var white=new Pen(Color.FromArgb(192,223,255,255),2))g.DrawLines(white,Points(128,86,96,121,113,186,128,204));
                    using(var jewel=new SolidBrush(Color.FromArgb(192,255,248)))Fill(g,jewel,128,49,138,63,128,76,118,63);
                }
            }
        }
        var result=new Bitmap(size,size,PixelFormat.Format32bppArgb);using(var g=Graphics.FromImage(result)){g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.DrawImage(drawing,0,0,size,size);}drawing.Dispose();return result;
    }
    public static void Write(string folder,bool game){string name=game?"ragexe":"pn-launcher";int[] sizes={16,24,32,48,64,128,256};var frames=new List<byte[]>();
        foreach(int size in sizes)using(var bmp=Draw(size,game))using(var stream=new MemoryStream()){bmp.Save(stream,ImageFormat.Png);frames.Add(stream.ToArray());if(size==256)bmp.Save(Path.Combine(folder,name+".png"),ImageFormat.Png);}
        using(var f=File.Create(Path.Combine(folder,name+".ico")))using(var w=new BinaryWriter(f)){w.Write((ushort)0);w.Write((ushort)1);w.Write((ushort)sizes.Length);int offset=6+16*sizes.Length;
            for(int i=0;i<sizes.Length;i++){w.Write((byte)(sizes[i]==256?0:sizes[i]));w.Write((byte)(sizes[i]==256?0:sizes[i]));w.Write((byte)0);w.Write((byte)0);w.Write((ushort)1);w.Write((ushort)32);w.Write(frames[i].Length);w.Write(offset);offset+=frames[i].Length;}
            foreach(var data in frames)w.Write(data);
        }
    }
    public static void Preview(string folder){using(var bmp=new Bitmap(576,348))using(var g=Graphics.FromImage(bmp)){
        g.Clear(Color.FromArgb(13,20,29));g.TextRenderingHint=System.Drawing.Text.TextRenderingHint.AntiAliasGridFit;
        using(var text=new SolidBrush(Color.FromArgb(231,193,126)))using(var font=new Font("Segoe UI",13,FontStyle.Bold)){
            g.DrawString("PN LAUNCHER",font,text,74,20);g.DrawString("RAGNAROK",font,text,367,20);
        }
        using(var a=Draw(192,false))g.DrawImageUnscaled(a,48,61);using(var b=Draw(192,true))g.DrawImageUnscaled(b,336,61);
        int[] sizes={16,24,32,48};for(int i=0;i<sizes.Length;i++){using(var a=Draw(sizes[i],false))g.DrawImageUnscaled(a,72+i*44,286+(48-sizes[i])/2);using(var b=Draw(sizes[i],true))g.DrawImageUnscaled(b,360+i*44,286+(48-sizes[i])/2);}
        bmp.Save(Path.Combine(folder,"poring-preview.png"),ImageFormat.Png);
    }}
}
'@
$iconOutput=[IO.Path]::GetFullPath($Output)
[PNPoringIcons]::Write($iconOutput,$false)
[PNPoringIcons]::Write($iconOutput,$true)
[PNPoringIcons]::Preview($iconOutput)
Write-Output 'Created Poring and crystal-crown icons with seven Windows resolutions.'
