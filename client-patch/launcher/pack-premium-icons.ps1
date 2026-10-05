param([string]$Assets=(Join-Path $PSScriptRoot 'assets/premium'))
$ErrorActionPreference='Stop'
if($PSVersionTable.PSEdition -eq 'Core'){
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -Assets $Assets
    if($LASTEXITCODE){throw 'Premium icon packaging failed'}
    return
}
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Collections.Generic;
public static class PNPremiumIconPack {
    static Bitmap SizeIcon(Image source,int size){var result=new Bitmap(size,size,PixelFormat.Format32bppArgb);using(var g=Graphics.FromImage(result)){
        g.CompositingMode=CompositingMode.SourceCopy;g.CompositingQuality=CompositingQuality.HighQuality;g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.PixelOffsetMode=PixelOffsetMode.HighQuality;
        using(var attributes=new ImageAttributes()){attributes.SetWrapMode(WrapMode.TileFlipXY);g.DrawImage(source,new Rectangle(0,0,size,size),0,0,source.Width,source.Height,GraphicsUnit.Pixel,attributes);}
    }return result;}
    static byte[] IconFrame(Bitmap image){int size=image.Width;using(var stream=new MemoryStream())using(var writer=new BinaryWriter(stream)){
        if(size==256){image.Save(stream,ImageFormat.Png);return stream.ToArray();}
        // Small uncompressed DIB frames are readable by .NET Framework 4.x as
        // well as Explorer. Only the 256 px frame uses PNG compression.
        int maskStride=((size+31)/32)*4;
        writer.Write(40);writer.Write(size);writer.Write(size*2);writer.Write((ushort)1);writer.Write((ushort)32);
        writer.Write(0);writer.Write(size*size*4);writer.Write(0);writer.Write(0);writer.Write(0);writer.Write(0);
        for(int y=size-1;y>=0;y--)for(int x=0;x<size;x++){Color pixel=image.GetPixel(x,y);writer.Write(pixel.B);writer.Write(pixel.G);writer.Write(pixel.R);writer.Write(pixel.A);}
        for(int y=size-1;y>=0;y--){byte[] mask=new byte[maskStride];for(int x=0;x<size;x++)if(image.GetPixel(x,y).A==0)mask[x/8]|=(byte)(0x80>>(x%8));writer.Write(mask);}
        return stream.ToArray();
    }}
    public static void Pack(string folder,string name){int[] sizes={16,24,32,48,64,128,256};var frames=new List<byte[]>();
        using(var source=Image.FromFile(Path.Combine(folder,name+".png"))){foreach(int size in sizes)using(var image=SizeIcon(source,size)){
            frames.Add(IconFrame(image));if(size==256)image.Save(Path.Combine(folder,name+"-256.png"),ImageFormat.Png);
        }}
        using(var file=File.Create(Path.Combine(folder,name+".ico")))using(var writer=new BinaryWriter(file)){
            writer.Write((ushort)0);writer.Write((ushort)1);writer.Write((ushort)sizes.Length);int offset=6+16*sizes.Length;
            for(int i=0;i<sizes.Length;i++){writer.Write((byte)(sizes[i]==256?0:sizes[i]));writer.Write((byte)(sizes[i]==256?0:sizes[i]));writer.Write((byte)0);writer.Write((byte)0);writer.Write((ushort)1);writer.Write((ushort)32);writer.Write(frames[i].Length);writer.Write(offset);offset+=frames[i].Length;}
            foreach(var data in frames)writer.Write(data);
        }
    }
    public static void Preview(string folder){using(var bitmap=new Bitmap(576,360))using(var g=Graphics.FromImage(bitmap)){
        g.Clear(Color.FromArgb(13,20,29));g.TextRenderingHint=System.Drawing.Text.TextRenderingHint.AntiAliasGridFit;
        using(var color=new SolidBrush(Color.FromArgb(231,193,126)))using(var font=new Font("Segoe UI",13,FontStyle.Bold)){
            g.DrawString("PN LAUNCHER",font,color,74,20);g.DrawString("RAGNAROK",font,color,367,20);
        }
        string[] names={"pn-launcher","ragexe"};for(int index=0;index<names.Length;index++)using(var source=Image.FromFile(Path.Combine(folder,names[index]+".png"))){
            using(var icon=SizeIcon(source,208))g.DrawImageUnscaled(icon,40+288*index,57);
            int[] sizes={16,24,32,48};for(int i=0;i<sizes.Length;i++)using(var icon=SizeIcon(source,sizes[i]))g.DrawImageUnscaled(icon,72+288*index+44*i,291+(48-sizes[i])/2);
        }
        bitmap.Save(Path.Combine(folder,"premium-preview.png"),ImageFormat.Png);
    }}
}
'@
$assetPath=[IO.Path]::GetFullPath($Assets)
[PNPremiumIconPack]::Pack($assetPath,'pn-launcher')
[PNPremiumIconPack]::Pack($assetPath,'ragexe')
[PNPremiumIconPack]::Preview($assetPath)
Write-Output 'Packaged both premium icons at 16, 24, 32, 48, 64, 128 and 256 px, preserving transparency.'
