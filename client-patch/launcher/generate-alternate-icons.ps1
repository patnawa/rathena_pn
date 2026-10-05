param([string]$Output=(Join-Path $PSScriptRoot 'assets/alternate'))
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
public static class PNEmblemIcons {
    static PointF[] Points(params float[] xy){var p=new PointF[xy.Length/2];for(int i=0;i<p.Length;i++)p[i]=new PointF(xy[2*i],xy[2*i+1]);return p;}
    static void Fill(Graphics g,Brush b,params float[] xy){g.FillPolygon(b,Points(xy));}
    static GraphicsPath Gate(){var p=new GraphicsPath();p.AddLine(78,190,78,108);p.AddBezier(78,108,78,76,99,57,128,57);p.AddBezier(128,57,157,57,178,76,178,108);p.AddLine(178,108,178,190);p.CloseFigure();return p;}
    static Bitmap Draw(int size,bool game){
        // Draw small icons at twice their output resolution for smooth silhouettes.
        int resolution=Math.Max(64,size*2);var drawing=new Bitmap(resolution,resolution,PixelFormat.Format32bppArgb);
        using(var g=Graphics.FromImage(drawing)){
            g.SmoothingMode=SmoothingMode.AntiAlias;g.PixelOffsetMode=PixelOffsetMode.HighQuality;g.ScaleTransform(resolution/256f,resolution/256f);
            using(var bg=new LinearGradientBrush(new Rectangle(20,20,216,216),Color.FromArgb(45,42,78),Color.FromArgb(10,24,38),65f))g.FillEllipse(bg,14,14,228,228);
            using(var glow=new GraphicsPath()){
                glow.AddEllipse(18,18,220,220);using(var b=new PathGradientBrush(glow)){b.CenterPoint=new PointF(128,104);b.CenterColor=Color.FromArgb(game?45:65,74,194,222);b.SurroundColors=new[]{Color.FromArgb(0,74,194,222)};g.FillPath(b,glow);}
            }
            using(var gold=new LinearGradientBrush(new Rectangle(0,12,256,230),Color.FromArgb(255,228,165),Color.FromArgb(164,108,57),85f)){
                using(var p=new Pen(gold,size<=24?6:3))g.DrawEllipse(p,22,22,212,212);
                if(size>=32){
                    using(var p=new Pen(Color.FromArgb(62,186,184,207),1.5f))g.DrawEllipse(p,29,29,198,198);
                    using(var p=new Pen(Color.FromArgb(115,224,194,143),2))for(int a=0;a<360;a+=30){double angle=a*Math.PI/180;g.DrawLine(p,128+(float)Math.Cos(angle)*98,128+(float)Math.Sin(angle)*98,128+(float)Math.Cos(angle)*103,128+(float)Math.Sin(angle)*103);}
                }
                Fill(g,gold,128,6,135,18,128,30,121,18);
                if(!game){
                    using(var gate=Gate()){
                        using(var light=new LinearGradientBrush(new Rectangle(78,60,100,133),Color.FromArgb(52,90,148),Color.FromArgb(9,25,39),90f))g.FillPath(light,gate);
                        using(var p=new Pen(gold,9)){p.LineJoin=LineJoin.Round;g.DrawPath(p,gate);}
                    }
                    if(size>=32)using(var p=new Pen(Color.FromArgb(105,112,216,239),2)){g.DrawArc(p,88,68,80,80,180,180);g.DrawLine(p,88,108,88,181);g.DrawLine(p,168,108,168,181);}
                    using(var cyan=new SolidBrush(Color.FromArgb(105,228,244)))Fill(g,cyan,128,87,149,121,128,166,107,121);
                    using(var white=new SolidBrush(Color.FromArgb(215,254,255)))Fill(g,white,128,87,128,127,107,121);
                    using(var blue=new SolidBrush(Color.FromArgb(36,125,190)))Fill(g,blue,128,127,149,121,128,166);
                    using(var p=new Pen(Color.FromArgb(230,181,247,250),size<=24?4:2))g.DrawPolygon(p,Points(128,87,149,121,128,166,107,121));
                    using(var p=new Pen(gold,5)){p.StartCap=LineCap.Round;p.EndCap=LineCap.Round;g.DrawLine(p,70,202,186,202);if(size>=32)g.DrawLine(p,84,213,172,213);}
                    if(size>=32)using(var p=new Pen(Color.FromArgb(185,184,244,250),2)){g.DrawLine(p,128,174,128,182);g.DrawLine(p,124,178,132,178);}
                }else{
                    // Wing silhouettes remain distinct from the portal at 16 px.
                    Fill(g,gold,112,122,34,53,47,95,99,147,43,106,58,141,107,166,64,151,80,177,119,183);
                    Fill(g,gold,144,122,222,53,209,95,157,147,213,106,198,141,149,166,192,151,176,177,137,183);
                    if(size>=32)using(var b=new SolidBrush(Color.FromArgb(100,97,57,37))){Fill(g,b,46,77,101,140,95,130,42,64);Fill(g,b,210,77,155,140,161,130,214,64);}
                    using(var silver=new SolidBrush(Color.FromArgb(223,244,247)))Fill(g,silver,128,38,144,77,135,146,121,146,112,77);
                    using(var steel=new SolidBrush(Color.FromArgb(91,151,179)))Fill(g,steel,128,38,144,77,135,146,128,146);
                    Fill(g,gold,87,145,97,133,117,143,139,143,159,133,169,145,159,160,141,155,115,155,97,160);
                    using(var b=new SolidBrush(Color.FromArgb(206,165,94)))g.FillRectangle(b,121,153,14,45);
                    Fill(g,gold,128,190,141,203,128,217,115,203);
                    using(var b=new SolidBrush(Color.FromArgb(107,231,244)))Fill(g,b,128,138,137,149,128,160,119,149);
                    if(size>=32)using(var p=new Pen(Color.FromArgb(225,244,215,159),2)){g.DrawLine(p,118,169,137,173);g.DrawLine(p,118,182,137,186);}
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
        bmp.Save(Path.Combine(folder,"emblem-preview.png"),ImageFormat.Png);
    }}
}
'@
$iconOutput=[IO.Path]::GetFullPath($Output)
[PNEmblemIcons]::Write($iconOutput,$false)
[PNEmblemIcons]::Write($iconOutput,$true)
[PNEmblemIcons]::Preview($iconOutput)
Write-Output 'Created alternate portal and winged-sword icons with seven Windows resolutions.'
