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
            Console.WriteLine("PASS: release path restrictions, signature rejection, interrupted update recovery, idempotent recovery and pre-apply crash");
        }finally{Directory.Delete(root,true);}
    }
}
