using System;
using System.IO;
using System.Linq;
static class SelfTest {
    static void Assert(bool result,string name){if(!result)throw new Exception("FAIL: "+name);}
    public static void Run(){
        foreach(string bad in new[]{"../outside","/absolute","C:/file","a\\b","a//b","a/../b","a./b","CON.txt","a/LPT1.exe","savedata/x","ScreenShot/x",".pn-updater/transaction.json","PNLauncher.exe","a:b","x\n.exe"}){
            bool refused=false;try{Engine.ValidName(bad);}catch(IOException){refused=true;}Assert(refused,"path rejected: "+bad);
        }
        string root=Path.Combine(Path.GetTempPath(),"pn-launcher-test-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
        try {
            var e=new Engine(root,(m,p)=>{});string work=Path.Combine(root,".pn-updater");Directory.CreateDirectory(Path.Combine(work,"recovery"));
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
            Console.WriteLine("PASS: immutable large-file verification cache, repeated check, locked-content protection, concurrent game read, activation invalidation, replacement and disposal");
            Console.WriteLine("PASS: release path restrictions, signature rejection, interrupted update recovery, idempotent recovery and pre-apply crash");
            Console.WriteLine("PASS: versioned self-delete/self-replace rollback preflight, no partial file or journal changes, and preserved-launcher recovery");
        }finally{Directory.Delete(root,true);}
    }
}
