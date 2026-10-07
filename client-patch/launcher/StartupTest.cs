using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Threading;
using System.Windows.Forms;

static class StartupTest {
    sealed class CompletionContext:SynchronizationContext {
        internal readonly ManualResetEvent Posted=new ManualResetEvent(false);
        internal Action Complete;
        public override void Post(SendOrPostCallback callback,object state){Complete=()=>callback(state);Posted.Set();}
    }
    static System.Collections.Generic.IEnumerable<Control> Controls(Control parent){
        foreach(Control child in parent.Controls){yield return child;foreach(var nested in Controls(child))yield return nested;}
    }
    static void Assert(bool ok,string message){if(!ok)throw new Exception("FAIL: "+message);}
    internal static void Run(){
        string root=Path.Combine(Path.GetTempPath(),"pn-startup-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
        try{foreach(string outcome in new[]{"success","failure","close"}){
            using(var finished=new ManualResetEvent(false))using(var form=new MainForm(root,true)){
                int launches=0;form.StartClient=path=>{launches++;};
                form.ShowInTaskbar=false;form.StartPosition=FormStartPosition.Manual;form.Location=new Point(-32000,-32000);form.Show();Application.DoEvents();
                // The console self-test has no Application.Run loop to install
                // a stable UI context between successive temporary windows.
                SynchronizationContext.SetSynchronizationContext(new WindowsFormsSynchronizationContext());
                var play=Controls(form).OfType<ActionButton>().First(b=>b.Primary&&b.Text.StartsWith("Play Ragnarok"));
                form.BeginStartupCheck(()=>{if(!finished.WaitOne(4000))throw new Exception("Test check stalled");if(outcome=="failure")throw new IOException("fixture verification failed");return "fixture client verified";});
                Assert(play.Enabled,"Play can queue during the startup check");play.PerformClick();
                Assert(launches==0&&!play.Enabled,"queued Play waits for verification without duplicate clicks");
                if(outcome=="close")form.Close();
                finished.Set();var clock=Stopwatch.StartNew();
                while(clock.ElapsedMilliseconds<3000&&!form.IsDisposed&&!play.Enabled){Application.DoEvents();Thread.Sleep(5);}
                Assert(launches==(outcome=="success"?1:0),"startup Play launches once only after success and never after failure or closing");
                if(!form.IsDisposed)form.Close();
            }
        }
        using(var form=new MainForm(root,true)){
            int launches=0;form.StartClient=path=>{launches++;};
            form.ShowInTaskbar=false;form.StartPosition=FormStartPosition.Manual;form.Location=new Point(-32000,-32000);form.Show();Application.DoEvents();
            var previous=SynchronizationContext.Current;var context=new CompletionContext();
            try{
                SynchronizationContext.SetSynchronizationContext(context);
                var play=Controls(form).OfType<ActionButton>().First(b=>b.Primary&&b.Text.StartsWith("Play Ragnarok"));
                var cancel=Controls(form).OfType<ActionButton>().First(b=>b.Text=="Cancel");
                form.BeginStartupCheck(()=>"fixture client verified");play.PerformClick();
                Assert(context.Posted.WaitOne(3000),"verification completed before cancelling queued Play");
                cancel.PerformClick();context.Complete();
                Assert(launches==0,"Cancel after verification but before its UI completion must stop queued Play");
            }finally{SynchronizationContext.SetSynchronizationContext(previous);context.Posted.Dispose();form.Close();}
        }
        }finally{Directory.Delete(root,true);}
        Console.WriteLine("PASS: queued startup Play waits for verification, launches once on success, and never launches after failure or closing");
    }
}
