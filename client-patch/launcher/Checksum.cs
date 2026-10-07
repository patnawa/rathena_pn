using System;
using System.IO;
using System.Security.Cryptography;
using System.Threading;

static class Checksum {
    [ThreadStatic] static byte[] buffer;
    internal static string Stream(Stream stream,CancellationToken cancellation){
        if(buffer==null)buffer=new byte[1024*1024];
        using(var hash=SHA256.Create()){
            int count;
            while(true){cancellation.ThrowIfCancellationRequested();count=stream.Read(buffer,0,buffer.Length);if(count==0)break;
                hash.TransformBlock(buffer,0,count,buffer,0);}
            cancellation.ThrowIfCancellationRequested();hash.TransformFinalBlock(buffer,0,0);
            return BitConverter.ToString(hash.Hash).Replace("-","").ToLowerInvariant();
        }
    }
    internal static string File(string path,CancellationToken cancellation){
        using(var stream=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read,65536,FileOptions.SequentialScan))return Stream(stream,cancellation);
    }
}
