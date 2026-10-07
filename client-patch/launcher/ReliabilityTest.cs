using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

static class ReliabilityTest {
    static void Assert(bool ok,string message){if(!ok)throw new Exception("FAIL: "+message);}
    internal static void Run(){
        string root=Path.Combine(Path.GetTempPath(),"pn-reliability-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
        try{
            string small=Path.Combine(root,"small.lua");File.WriteAllText(small,"trusted fixture");
            using(var cache=new VerifiedFileCache()){
                string hash=cache.Hash(small);Assert(cache.Hash(small)==hash&&cache.HashReads==1,"small signed files reuse an immutable checksum");
                bool locked=false;try{File.WriteAllText(small,"changed fixture");}catch(IOException){locked=true;}
                Assert(locked,"small verified files stay immutable until invalidation");
                cache.Clear();File.WriteAllText(small,"changed fixture");Assert(cache.Hash(small)!=hash,"small file changes detected after cache invalidation");
            }
            Console.WriteLine("PASS: small-file checksum reuse, write protection and invalidation");
            ChecksumAndPaths(root);
            StalledDownload(root);
            RetryDownload(root,false,false,false);
            RetryDownload(root,true,false,false);
            RetryDownload(root,false,true,false);
            RetryDownload(root,false,false,true);
            LocalChecks(root);
        }finally{Directory.Delete(root,true);}
    }
    static void ChecksumAndPaths(string root){
        byte[] bytes=new byte[1024*1024+17];for(int i=0;i<bytes.Length;i++)bytes[i]=(byte)(i%251);
        string expected;using(var hash=System.Security.Cryptography.SHA256.Create())expected=BitConverter.ToString(hash.ComputeHash(bytes)).Replace("-","").ToLowerInvariant();
        using(var stream=new MemoryStream(bytes))Assert(Checksum.Stream(stream,CancellationToken.None)==expected,"streaming checksum matches SHA-256 across a block boundary");
        string target=Path.Combine(root,"actual"),link=Path.Combine(root,"linked");Directory.CreateDirectory(target);
        var command=new ProcessStartInfo("cmd.exe","/c mklink /J \""+link+"\" \""+target+"\""){
            UseShellExecute=false,CreateNoWindow=true,WindowStyle=ProcessWindowStyle.Hidden,RedirectStandardOutput=true,RedirectStandardError=true};
        using(var process=Process.Start(command)){process.StandardOutput.ReadToEnd();process.StandardError.ReadToEnd();process.WaitForExit();Assert(process.ExitCode==0,"private junction fixture created");}
        try{bool refused=false;try{Engine.NoLinks(Path.Combine(link,"missing.lua"));}catch(IOException){refused=true;}Assert(refused,"missing file beneath a junction is still refused");}
        finally{Directory.Delete(link);}
        Console.WriteLine("PASS: SHA-256 block boundary and reparse-point refusal for a missing descendant");
    }
    internal static void StalledDownload(string root){
        var listener=new TcpListener(IPAddress.Loopback,0);listener.Start();
        int port=((IPEndPoint)listener.LocalEndpoint).Port;
        using(var stop=new ManualResetEvent(false)){
            var server=new Thread(()=>{try{using(var socket=listener.AcceptTcpClient())using(var stream=socket.GetStream()){
                stream.ReadTimeout=4000;byte[] request=new byte[4096];stream.Read(request,0,request.Length);stop.WaitOne(4000);
            }}catch(IOException){}catch(SocketException){}});server.IsBackground=true;server.Start();
            try{using(var engine=new Engine(root,(m,p)=>{}))using(var token=new CancellationTokenSource()){
                string stage=Path.Combine(engine.Work,"stage");Directory.CreateDirectory(stage);engine.Cancellation=token.Token;
                token.CancelAfter(150);var clock=Stopwatch.StartNew();bool cancelled=false;
                try{engine.Download(new Entry{path="slow.grf",bytes=1,sha256=new string('0',64)},stage,"http://127.0.0.1:"+port+"/");}
                catch(OperationCanceledException){cancelled=true;}catch(WebException){}
                Assert(cancelled&&clock.ElapsedMilliseconds<1500,"cancel aborts a stalled HTTP response within 1.5 seconds");
                Assert(!File.Exists(Path.Combine(stage,"slow.grf")),"stalled cancelled download leaves no partial file");
                Console.WriteLine("PASS: stalled HTTP cancellation in "+clock.ElapsedMilliseconds+" ms");
            }}finally{stop.Set();listener.Stop();Assert(server.Join(5000),"stalled loopback server exits");}
        }
    }
    static void RetryDownload(string root,bool interrupted,bool corrupt,bool unavailable){
        var listener=new TcpListener(IPAddress.Loopback,0);listener.Start();
        int port=((IPEndPoint)listener.LocalEndpoint).Port;int requests=0;byte[] payload=Encoding.UTF8.GetBytes("verified update fixture");
        string digest;using(var stream=new MemoryStream(payload))digest=Checksum.Stream(stream,CancellationToken.None);
        using(var stop=new ManualResetEvent(false)){
            var server=new Thread(()=>{try{while(!stop.WaitOne(0)){using(var socket=listener.AcceptTcpClient())using(var stream=socket.GetStream()){
                stream.ReadTimeout=3000;byte[] request=new byte[4096];stream.Read(request,0,request.Length);int attempt=Interlocked.Increment(ref requests);
                bool fail=unavailable||(!interrupted&&!corrupt&&attempt<3);
                byte[] header=Encoding.ASCII.GetBytes("HTTP/1.1 "+(fail?"503 Service Unavailable":"200 OK")+"\r\nContent-Length: "+(fail?0:payload.Length)+"\r\nConnection: close\r\n\r\n");
                stream.Write(header,0,header.Length);if(!fail)stream.Write(payload,0,interrupted&&attempt==1?3:payload.Length);stream.Flush();
            }}}catch(IOException){}catch(SocketException){}catch(ObjectDisposedException){}});server.IsBackground=true;server.Start();
            try{using(var engine=new Engine(root,(m,p)=>{})){
                string stage=Path.Combine(engine.Work,"retry-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(stage);
                bool failed=false;try{engine.Download(new Entry{path="fixture.grf",bytes=payload.Length,sha256=corrupt?new string('0',64):digest},stage,"http://127.0.0.1:"+port+"/");}
                catch(IOException){failed=true;}catch(WebException){failed=true;}
                Assert(failed==(corrupt||unavailable),"transient retry success and permanent failure classification");
                Assert(requests==(corrupt?1:interrupted?2:3),"HTTP retry count is bounded and checksum failure is not retried");
                string target=Path.Combine(stage,"fixture.grf");
                Assert(failed?!File.Exists(target):Engine.Hash(target)==digest,"retry verifies final bytes and failures remove partial files");
                Assert(!File.Exists(Path.Combine(engine.Work,"transaction.json")),"transfer tests never activate a client transaction");
            }}finally{stop.Set();listener.Stop();Assert(server.Join(5000),"retry loopback server exits");}
        }
        Console.WriteLine("PASS: "+(corrupt?"checksum failure refused without retry":unavailable?"unavailable server stops after three attempts":interrupted?"interrupted body retries without stale partial bytes":"503 response retries and verifies the object"));
    }
    static void LocalChecks(string root){
        using(var engine=new Engine(root,(m,p)=>{})){
            string managed=Path.Combine(root,"managed.txt"),config=Path.Combine(root,"preferences.ini");
            File.WriteAllText(managed,"original");File.WriteAllText(config,"personal");
            var manifest=new Manifest{files=new[]{new Entry{path="managed.txt",bytes=8,sha256=Engine.Hash(managed)},new Entry{path="preferences.ini",preserve=true,bytes=0,sha256=new string('0',64)}}};
            Assert(engine.CheckFiles(manifest).Count==0,"local verification accepts signed expected bytes and preserves existing preferences");
            engine.verifiedFiles.Clear();File.WriteAllText(managed,"modified");Assert(engine.CheckFiles(manifest).Count==1,"same-length corruption is detected by local verification");
            engine.verifiedFiles.Clear();File.Delete(managed);Assert(engine.CheckFiles(manifest).Count==1,"missing managed files block local launch");
            engine.Write(Path.Combine(engine.Work,"installed.json"),"{\"payload\":\"e30=\",\"signature\":\"AAAA\"}");
            bool refused=false;try{engine.VerifyInstalled();}catch(IOException){refused=true;}Assert(refused,"installed-client launch rejects forged metadata");
            string ini=Path.Combine(root,"editable.ini");File.WriteAllText(ini,"before");string first=engine.verifiedFiles.Hash(ini);File.WriteAllText(ini,"after!");
            Assert(engine.verifiedFiles.Hash(ini)!=first,"INI files remain editable and are rehashed");
        }
        string prefs=Path.Combine(root,"old-preferences.json");File.WriteAllText(prefs,"{\"autoRefresh\":false}");
        Assert(LauncherPreferences.Load(prefs).checkOnOpen,"older preference files enable the new startup check by default");
        File.WriteAllText(prefs,"{\"checkOnOpen\":false}");Assert(!LauncherPreferences.Load(prefs).checkOnOpen,"startup check can be disabled independently");
        Console.WriteLine("PASS: local checksum/missing-file/signature refusals, preserved preferences, editable INI files and startup preference compatibility");
    }
}
