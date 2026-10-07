using System;
using System.IO;
using System.Linq;
static class SelfTest {
    static void Assert(bool result,string name){if(!result)throw new Exception("FAIL: "+name);}
    public static void Run(){
        ReliabilityTest.Run();
        StartupTest.Run();
        foreach(string bad in new[]{"../outside","/absolute","C:/file","a\\b","a//b","a/../b","a./b","CON.txt","a/LPT1.exe","savedata/x","ScreenShot/x",".pn-updater/transaction.json","PNLauncher.exe","a:b","x\n.exe"}){
            bool refused=false;try{Engine.ValidName(bad);}catch(IOException){refused=true;}Assert(refused,"path rejected: "+bad);
        }
        string root=Path.Combine(Path.GetTempPath(),"pn-launcher-test-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
        try {
            var e=new Engine(root,(m,p)=>{});string work=Path.Combine(root,".pn-updater");Directory.CreateDirectory(Path.Combine(work,"recovery"));
            File.WriteAllText(Path.Combine(root,"Setup.exe"),"original setup");
            Assert(GameSettings.Tool(root)=="Setup.exe","original settings fallback");
            File.WriteAllText(Path.Combine(root,"PNOpenSetup.exe"),"Lua setup");
            Assert(GameSettings.Tool(root)=="PNOpenSetup.exe","OpenSetup preferred over original setup");
            Directory.CreateDirectory(Path.Combine(root,"savedata"));
            string settings=Path.Combine(root,"savedata","OptionInfo.lua");
            string original="OptionInfoList[\"WIDTH\"] = 1920\nOptionInfoList[\"HEIGHT\"] = 1080\nOptionInfoList[\"RENDERSYSTEM\"] = 2\nOptionInfoList[\"ISFULLSCREENMODE\"] = 0\n";
            File.WriteAllText(settings,original);File.WriteAllText(Path.Combine(root,"PN-Turbo.ini"),"personal Turbo");
            string snapshot=GameSettings.Backup(e);
            Assert(File.ReadAllText(Path.Combine(snapshot,"savedata","OptionInfo.lua"))==original&&File.ReadAllText(settings)==original,"settings snapshot preserves source and Lua content");
            Assert(File.ReadAllText(Path.Combine(snapshot,"PN-Turbo.ini"))=="personal Turbo","settings snapshot preserves Turbo preferences");
            Assert(GameSettings.Summary(root).Contains("1920 × 1080")&&GameSettings.Summary(root).Contains("DirectX 9"),"reads current Lua display settings");
            string prefs=Path.Combine(work,"launcher-settings.json");File.WriteAllText(prefs,"{\"autoRefresh\":false,\"minimizeOnPlay\":false}");
            Assert(!LauncherPreferences.Load(prefs).autoRefresh&&!LauncherPreferences.Load(prefs).minimizeOnPlay,"launcher preferences persist independently of game settings");
            File.WriteAllText(prefs,"damaged");Assert(LauncherPreferences.Load(prefs).autoRefresh,"damaged preferences use safe defaults");
            Console.WriteLine("PASS: OpenSetup selection and original fallback, Lua display summary, settings snapshots and independent launcher preferences");
            File.WriteAllText(Path.Combine(root,"existing.txt"),"new");File.WriteAllText(Path.Combine(root,"added.txt"),"new");
            File.WriteAllText(Path.Combine(work,"recovery","existing.txt"),"old");
            var t=new Transaction{phase="applying",previousManifest=false,changes=new[]{new Change{path="existing.txt",existed=true,sha256=Engine.Hash(Path.Combine(root,"existing.txt"))},new Change{path="added.txt",existed=false,sha256=Engine.Hash(Path.Combine(root,"added.txt"))}}};
            File.WriteAllText(Path.Combine(work,"transaction.json"),e.Json.Serialize(t));e.Recover();
            Assert(File.ReadAllText(Path.Combine(root,"existing.txt"))=="old","crash restores replaced file");Assert(!File.Exists(Path.Combine(root,"added.txt")),"crash removes added file");e.Recover();
            // Crash before any existing file was moved: keep original, discard stage.
            Directory.CreateDirectory(Path.Combine(work,"stage"));File.WriteAllText(Path.Combine(work,"stage","existing.txt"),"new");
            t.changes=t.changes.Take(1).ToArray();File.WriteAllText(Path.Combine(work,"transaction.json"),e.Json.Serialize(t));e.Recover();Assert(File.ReadAllText(Path.Combine(root,"existing.txt"))=="old","pre-apply crash leaves original");
            Directory.CreateDirectory(Path.Combine(work,"previous"));File.WriteAllText(Path.Combine(work,"previous","existing.txt"),"last version");
            Directory.CreateDirectory(Path.Combine(work,"recovery"));File.WriteAllText(Path.Combine(work,"recovery","existing.txt"),"corrupted repair input");
            t.phase="complete";t.keepRollback=false;e.Write(Path.Combine(work,"transaction.json"),e.Json.Serialize(t));e.Recover();
            Assert(File.ReadAllText(Path.Combine(work,"previous","existing.txt"))=="last version","repair retains previous version backup");
            Directory.CreateDirectory(Path.Combine(work,"recovery"));File.WriteAllText(Path.Combine(work,"recovery","existing.txt"),"new prior version");
            t.phase="complete";t.keepRollback=true;e.Write(Path.Combine(work,"transaction.json"),e.Json.Serialize(t));e.Recover();
            Assert(File.ReadAllText(Path.Combine(work,"previous","existing.txt"))=="new prior version","completed update promotes rollback");
            bool rejected=false;try{e.Verify("{\"payload\":\"e30=\",\"signature\":\"AAAA\"}");}catch{rejected=true;}Assert(rejected,"unsigned/forged manifest rejected");
            // A refusal must be detected before rolling back any other file.
            foreach(bool missingManifest in new[]{false,true}) {
                string refusedRoot=Path.Combine(root,"refused-"+missingManifest);
                using(var refused=new Engine(refusedRoot,(m,p)=>{})) {
                    string previous=Path.Combine(refused.Work,"previous");Directory.CreateDirectory(previous);
                    string existing=Path.Combine(refusedRoot,"existing.txt"),added=Path.Combine(refusedRoot,"added.txt");
                    File.WriteAllText(existing,"current");File.WriteAllText(added,"user changed");
                    File.WriteAllText(Path.Combine(previous,"existing.txt"),"previous");
                    File.WriteAllText(Path.Combine(refused.Work,"installed.json"),"current manifest");
                    var pending=new Transaction{phase="complete",backupFolder="previous",previousManifest=missingManifest,changes=new[]{
                        new Change{path="added.txt",existed=false,sha256=new string('0',64)},
                        new Change{path="existing.txt",existed=true,sha256=Engine.Hash(existing)}}};
                    if(missingManifest)pending.changes=pending.changes.Skip(1).ToArray();
                    string rollback=Path.Combine(refused.Work,"rollback.json");refused.Write(rollback,refused.Json.Serialize(pending));
                    string originalRollback=File.ReadAllText(rollback);
                    rejected=false;try{refused.Rollback();}catch(IOException){rejected=true;}
                    Assert(rejected,"invalid rollback prerequisites refused");
                    Assert(File.ReadAllText(existing)=="current"&&File.ReadAllText(Path.Combine(previous,"existing.txt"))=="previous", "rollback refusal preserves all installed files and backups");
                    Assert(!File.Exists(Path.Combine(refused.Work,"transaction.json")),"rollback refusal creates no recovery journal");
                    Assert(File.ReadAllText(added)=="user changed"&&File.ReadAllText(Path.Combine(refused.Work,"installed.json"))=="current manifest"&&File.ReadAllText(rollback)==originalRollback,"rollback refusal preserves user changes and metadata");
                    pending.phase="applying";string journal=Path.Combine(refused.Work,"transaction.json");refused.Write(journal,refused.Json.Serialize(pending));
                    string originalJournal=File.ReadAllText(journal);
                    rejected=false;try{refused.Recover();}catch(IOException){rejected=true;}
                    Assert(rejected&&File.ReadAllText(existing)=="current"&&File.ReadAllText(Path.Combine(previous,"existing.txt"))=="previous", "recovery refusal preserves all installed files and backups");
                    Assert(File.ReadAllText(journal)==originalJournal&&File.ReadAllText(Path.Combine(refused.Work,"installed.json"))=="current manifest","recovery refusal preserves journal and installed metadata");
                }
            }
            Console.WriteLine("PASS: changed-file and missing-manifest rollback/recovery refusals preserve files, backups, journals and installed metadata");
            // Reverse rollback order would restore data.txt before deleting or
            // replacing the running versioned updater. Refuse the whole set.
            string rollbackRoot=Path.Combine(root,"rollback-test");Directory.CreateDirectory(rollbackRoot);
            string self=Path.Combine(rollbackRoot,"PNLauncher-20260928.exe"),data=Path.Combine(rollbackRoot,"data.txt");
            File.WriteAllText(self,"current executable");File.WriteAllText(data,"current data");
            using(var versioned=new Engine(rollbackRoot,(m,p)=>{},self.ToUpperInvariant())) {
                string previous=Path.Combine(versioned.Work,"previous"),journal=Path.Combine(versioned.Work,"transaction.json"),rollback=Path.Combine(versioned.Work,"rollback.json");
                Directory.CreateDirectory(previous);File.WriteAllText(Path.Combine(previous,"data.txt"),"previous data");
                File.WriteAllText(Path.Combine(versioned.Work,"installed.json"),"current manifest");
                File.WriteAllText(Path.Combine(versioned.Work,"previous-manifest.json"),"previous manifest");
                var pending=new Transaction{phase="complete",previousManifest=true,backupFolder="previous",changes=new[]{
                    new Change{path="PNLauncher-20260928.exe",existed=false,sha256=Engine.Hash(self)},
                    new Change{path="data.txt",existed=true,sha256=Engine.Hash(data)}}};
                versioned.Write(rollback,versioned.Json.Serialize(pending));string originalRollback=File.ReadAllText(rollback);
                rejected=false;try{versioned.Rollback();}catch(IOException ex){rejected=ex.Message.Contains("preserved PNLauncher.exe");}
                Assert(rejected&&!File.Exists(journal),"self-deleting rollback refused before journal creation");
                Assert(File.ReadAllText(data)=="current data"&&File.ReadAllText(self)=="current executable"&&File.ReadAllText(Path.Combine(previous,"data.txt"))=="previous data","rollback preflight leaves all files unchanged");
                Assert(File.ReadAllText(rollback)==originalRollback&&File.ReadAllText(Path.Combine(versioned.Work,"installed.json"))=="current manifest","rollback preflight leaves metadata unchanged");
                // Recovery of an already-written rollback must also refuse
                // before any file changes, including replacing an existing exe.
                pending.phase="applying";pending.changes[0].existed=true;
                File.WriteAllText(Path.Combine(previous,"PNLauncher-20260928.exe"),"previous executable");
                versioned.Write(journal,versioned.Json.Serialize(pending));string originalJournal=File.ReadAllText(journal);
                rejected=false;try{versioned.Recover();}catch(IOException ex){rejected=ex.Message.Contains("preserved PNLauncher.exe");}
                Assert(rejected&&File.ReadAllText(journal)==originalJournal,"self-replacing recovery refused without changing journal");
                Assert(File.ReadAllText(data)=="current data"&&File.ReadAllText(self)=="current executable"&&File.ReadAllText(Path.Combine(previous,"PNLauncher-20260928.exe"))=="previous executable","recovery preflight prevents partial rollback");
                // The preserved launcher can recover the exact same journal.
                using(var preserved=new Engine(rollbackRoot,(m,p)=>{},Path.Combine(rollbackRoot,"PNLauncher.exe")))preserved.Recover();
                Assert(File.ReadAllText(data)=="previous data"&&File.ReadAllText(self)=="previous executable"&&!File.Exists(journal),"preserved launcher completes deferred recovery");
                Assert(File.ReadAllText(Path.Combine(versioned.Work,"installed.json"))=="previous manifest","preserved launcher restores previous manifest");
            }
            string large=Path.Combine(root,"large.grf");
            using(var file=new FileStream(large,FileMode.CreateNew,FileAccess.Write))file.SetLength(8L*1024*1024);
            using(var cache=new VerifiedFileCache()) {
                string first=cache.Hash(large);Assert(cache.Hash(large)==first && cache.HashReads==1,"same-session large-file checksum reused");
                bool locked=false;try{using(var file=new FileStream(large,FileMode.Open,FileAccess.Write))file.WriteByte(1);}catch(IOException){locked=true;}
                Assert(locked,"cached file cannot mutate without invalidation");
                using(var reader=new FileStream(large,FileMode.Open,FileAccess.Read,FileShare.ReadWrite))Assert(reader.ReadByte()==0,"game-compatible read while cached");
                cache.Clear();
                using(var file=new FileStream(large,FileMode.Open,FileAccess.Write))file.WriteByte(1);
                Assert(cache.Hash(large)!=first && cache.HashReads==2,"release invalidation checks changed content");
                cache.Clear();File.Move(large,large+".old");File.WriteAllText(large,"replacement");
                Assert(cache.Hash(large)==Engine.Hash(large),"replacement file is rehashed");
            }
            using(var file=new FileStream(large,FileMode.Open,FileAccess.Write))file.WriteByte(2);
            Assert(true,"disposing verification cache releases locks");
            // Cancelling before a check never requests a feed or touches installed files.
            string cancelledRoot=Path.Combine(root,"cancelled");Directory.CreateDirectory(cancelledRoot);
            File.WriteAllText(Path.Combine(cancelledRoot,"Ragexe.exe"),"unchanged client");
            using(var cancelled=new Engine(cancelledRoot,(m,p)=>{}))using(var token=new System.Threading.CancellationTokenSource()){
                token.Cancel();cancelled.Cancellation=token.Token;bool stopped=false;
                try{cancelled.Update();}catch(OperationCanceledException){stopped=true;}
                Assert(stopped,"cancelled check stops before network or recovery");
                Assert(File.ReadAllText(Path.Combine(cancelledRoot,"Ragexe.exe"))=="unchanged client"&&!File.Exists(Path.Combine(cancelled.Work,"transaction.json")),"cancellation preserves client and creates no transaction");
            }
            Assert(MainForm.FormatBytes(1024L*1024*1024)=="1.0 GiB","download metrics use binary units");
            // Stop a real loopback HTTP download after receiving bytes. The staged
            // partial file must never replace the installed file or create a journal.
            var listener=new System.Net.Sockets.TcpListener(System.Net.IPAddress.Loopback,0);listener.Start();
            int port=((System.Net.IPEndPoint)listener.LocalEndpoint).Port;
            var server=new System.Threading.Thread(()=>{
                try{using(var socket=listener.AcceptTcpClient())using(var stream=socket.GetStream()){
                    byte[] request=new byte[4096];stream.Read(request,0,request.Length);
                    byte[] header=System.Text.Encoding.ASCII.GetBytes("HTTP/1.1 200 OK\r\nContent-Length: 2097152\r\nConnection: close\r\n\r\n");stream.Write(header,0,header.Length);
                    byte[] payload=new byte[65536];for(int i=0;i<32;i++){stream.Write(payload,0,payload.Length);stream.Flush();}
                }}catch(System.IO.IOException){}catch(System.Net.Sockets.SocketException){}
            });server.IsBackground=true;server.Start();
            try{using(var cancelled=new Engine(cancelledRoot,(m,p)=>{}))using(var token=new System.Threading.CancellationTokenSource()){
                string stage=Path.Combine(cancelled.Work,"stage");Directory.CreateDirectory(stage);
                cancelled.Cancellation=token.Token;long received=0;cancelled.TransferProgress=(bytes,total,speed)=>{received=bytes;token.Cancel();};
                bool stopped=false;try{cancelled.Download(new Entry{path="Ragexe.exe",bytes=2097152,sha256=new string('0',64)},stage,"http://127.0.0.1:"+port+"/");}catch(OperationCanceledException){stopped=true;}
                Assert(stopped&&received>0,"in-flight download cancellation reports received bytes and stops");
                Assert(File.ReadAllText(Path.Combine(cancelledRoot,"Ragexe.exe"))=="unchanged client"&&!File.Exists(Path.Combine(cancelled.Work,"transaction.json")),"cancelled transfer never activates partial download");
            }}finally{listener.Stop();Assert(server.Join(5000),"loopback transfer server exits");}
            Console.WriteLine("PASS: cancellation before network and during HTTP transfer, preserved installed client, no partial activation, and download byte formatting");
            Console.WriteLine("PASS: immutable large-file verification cache, repeated check, locked-content protection, concurrent game read, activation invalidation, replacement and disposal");
            Console.WriteLine("PASS: release path restrictions, signature rejection, interrupted update recovery, idempotent recovery and pre-apply crash");
            Console.WriteLine("PASS: versioned self-delete/self-replace rollback preflight, no partial file or journal changes, and preserved-launcher recovery");
        }finally{Directory.Delete(root,true);}
    }
}
