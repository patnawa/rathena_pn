param([string]$Output=(Join-Path $PSScriptRoot 'assets'))
$ErrorActionPreference='Stop'
if($PSVersionTable.PSEdition -eq 'Core'){
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -Output $Output
    if($LASTEXITCODE){throw 'Icon generation failed'}
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
public static class PNIconBuilder {
    static GraphicsPath Round(float x,float y,float w,float h,float r){var p=new GraphicsPath();float d=r*2;p.AddArc(x,y,d,d,180,90);p.AddArc(x+w-d,y,d,d,270,90);p.AddArc(x+w-d,y+h-d,d,d,0,90);p.AddArc(x,y+h-d,d,d,90,90);p.CloseFigure();return p;}
    static PointF[] Points(params float[] xy){var p=new PointF[xy.Length/2];for(int i=0;i<p.Length;i++)p[i]=new PointF(xy[i*2],xy[i*2+1]);return p;}
    static void Fill(Graphics g,Brush b,params float[] xy){g.FillPolygon(b,Points(xy));}
    static Bitmap Draw(int size,bool game){var bmp=new Bitmap(size,size,PixelFormat.Format32bppArgb);using(var g=Graphics.FromImage(bmp)){
        g.SmoothingMode=SmoothingMode.AntiAlias;g.PixelOffsetMode=PixelOffsetMode.HighQuality;g.ScaleTransform(size/256f,size/256f);
        using(var bg=new LinearGradientBrush(new Rectangle(0,0,256,256),Color.FromArgb(38,62,81),Color.FromArgb(13,23,37),65f))
        using(var frame=Round(7,7,242,242,54))g.FillPath(bg,frame);
        using(var frame=Round(19,19,218,218,43))using(var pen=new Pen(Color.FromArgb(166,231,193,126),size<=24?5:3))g.DrawPath(pen,frame);
        using(var gold=new LinearGradientBrush(new Rectangle(0,40,256,180),Color.FromArgb(255,230,178),Color.FromArgb(197,142,66),80f)){
            if(!game){
                using(var p=new GraphicsPath(FillMode.Alternate)){
                    p.AddPolygon(Points(48,181,48,74,94,74,110,77,122,87,132,99,132,119,122,134,110,141,94,144,72,144,72,181));
                    p.AddPolygon(Points(72,96,72,123,92,123,104,118,110,109,104,100,92,96));g.FillPath(gold,p);
                }
                Fill(g,gold,141,181,141,74,165,74,194,139,194,74,216,74,216,181,192,181,163,117,163,181);
                if(size>=32){Fill(g,gold,128,30,137,39,128,48,119,39);Fill(g,gold,128,208,137,217,128,226,119,217);}
            }else{
                using(var shield=new GraphicsPath()){
                    shield.AddLines(Points(128,39,202,66,202,128));shield.AddBezier(202,128,202,167,168,196,128,219);
                    shield.AddBezier(128,219,88,196,54,167,54,128);shield.AddLine(54,128,54,66);shield.CloseFigure();
                    using(var b=new SolidBrush(Color.FromArgb(19,43,58)))g.FillPath(b,shield);using(var p=new Pen(gold,7))g.DrawPath(p,shield);
                }
                if(size>=32)using(var p=new Pen(Color.FromArgb(56,81,96),3))g.DrawPolygon(p,Points(128,56,184,77,184,127,170,163,128,201,86,163,72,127,72,77));
                using(var b=new SolidBrush(Color.FromArgb(228,238,240)))Fill(g,b,128,60,145,89,135,148,121,148,111,89);
                using(var b=new SolidBrush(Color.FromArgb(155,181,197)))Fill(g,b,128,60,145,89,135,148,128,148);
                Fill(g,gold,89,143,104,138,116,147,140,147,152,138,167,143,164,158,146,156,110,156,92,158);
                g.FillRectangle(gold,121,156,14,29);Fill(g,gold,128,180,139,191,128,202,117,191);
            }
        }
    }return bmp;}
    public static void Write(string folder,bool game){string name=game?"ragexe":"pn-launcher";int[] sizes={16,24,32,48,64,128,256};var frames=new List<byte[]>();
        foreach(int size in sizes)using(var bmp=Draw(size,game))using(var stream=new MemoryStream()){bmp.Save(stream,ImageFormat.Png);frames.Add(stream.ToArray());if(size==256)bmp.Save(Path.Combine(folder,name+".png"),ImageFormat.Png);}
        using(var file=File.Create(Path.Combine(folder,name+".ico")))using(var w=new BinaryWriter(file)){
            w.Write((ushort)0);w.Write((ushort)1);w.Write((ushort)sizes.Length);int offset=6+16*sizes.Length;
            for(int i=0;i<sizes.Length;i++){w.Write((byte)(sizes[i]==256?0:sizes[i]));w.Write((byte)(sizes[i]==256?0:sizes[i]));w.Write((byte)0);w.Write((byte)0);w.Write((ushort)1);w.Write((ushort)32);w.Write(frames[i].Length);w.Write(offset);offset+=frames[i].Length;}
            foreach(var data in frames)w.Write(data);
        }
    }
}
'@
[PNIconBuilder]::Write([IO.Path]::GetFullPath($Output),$false)
[PNIconBuilder]::Write([IO.Path]::GetFullPath($Output),$true)
Write-Output "Created PN Launcher and Ragexe icons: 16, 24, 32, 48, 64, 128 and 256 px."
