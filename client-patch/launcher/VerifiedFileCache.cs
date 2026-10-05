// Reuse large-file hashes only while Windows guarantees the same immutable file.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using Microsoft.Win32.SafeHandles;

sealed class VerifiedFileCache : IDisposable {
    const long MinimumBytes=8L*1024*1024;
    [StructLayout(LayoutKind.Sequential)] struct FileIdentity {
        public uint attributes, creationLow, creationHigh, accessLow, accessHigh,
            writeLow, writeHigh, volume, sizeHigh, sizeLow, links, indexHigh, indexLow;
    }
    [DllImport("kernel32.dll",SetLastError=true)]
    static extern bool GetFileInformationByHandle(SafeFileHandle handle,out FileIdentity info);
    sealed class Cached : IDisposable {
        public FileStream stream;
        public FileIdentity identity;
        public string hash;
        public void Dispose(){stream.Dispose();}
    }
    readonly Dictionary<string,Cached> files=new Dictionary<string,Cached>(StringComparer.OrdinalIgnoreCase);
    internal int HashReads {get;private set;}
    static FileIdentity Identity(FileStream stream) {
        FileIdentity identity;
        if(!GetFileInformationByHandle(stream.SafeFileHandle,out identity))throw new Win32Exception(Marshal.GetLastWin32Error());
        return identity;
    }
    static bool Same(FileIdentity a,FileIdentity b) {
        return a.volume==b.volume && a.indexHigh==b.indexHigh && a.indexLow==b.indexLow &&
            a.sizeHigh==b.sizeHigh && a.sizeLow==b.sizeLow;
    }
    public string Hash(string path) {
        path=Path.GetFullPath(path);Engine.NoLinks(path);
        // FileShare.Read denies writes and deletion until cache invalidation.
        // Open the current path on every lookup: directory replacement cannot
        // cause us to accept a hash from a different file at the same path.
        FileStream stream=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read);
        try {
            var identity=Identity(stream);Cached cached;
            if(files.TryGetValue(path,out cached)) {
                if(Same(identity,cached.identity))return cached.hash;
                files.Remove(path);cached.Dispose();
            }
            string hash;
            using(var algorithm=SHA256.Create())hash=BitConverter.ToString(algorithm.ComputeHash(stream)).Replace("-","").ToLowerInvariant();
            HashReads++;
            if(stream.Length>=MinimumBytes) {
                files.Add(path,new Cached{stream=stream,identity=identity,hash=hash});stream=null;
            }
            return hash;
        } finally {if(stream!=null)stream.Dispose();}
    }
    public void Clear(){foreach(var file in files.Values)file.Dispose();files.Clear();}
    public void Dispose(){Clear();}
}
