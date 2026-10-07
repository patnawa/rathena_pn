using System;
using System.IO;
using System.Net;
using System.Threading;

static class Network {
    internal static int Read(Stream stream,byte[] buffer,CancellationToken cancellation){
        cancellation.ThrowIfCancellationRequested();
        try{return stream.Read(buffer,0,buffer.Length);}
        catch(IOException error){cancellation.ThrowIfCancellationRequested();throw new WebException("The update connection was interrupted.",error,WebExceptionStatus.ReceiveFailure,null);}
    }
    static bool Transient(Exception error){
        if(error is EndOfStreamException)return true;
        var web=error as WebException;if(web==null)return false;
        var response=web.Response as HttpWebResponse;
        if(response!=null){int code=(int)response.StatusCode;return code==408||code==429||code==500||code==502||code==503||code==504;}
        return web.Status==WebExceptionStatus.ConnectFailure||web.Status==WebExceptionStatus.NameResolutionFailure||
            web.Status==WebExceptionStatus.Timeout||web.Status==WebExceptionStatus.ReceiveFailure||
            web.Status==WebExceptionStatus.SendFailure||web.Status==WebExceptionStatus.ConnectionClosed||web.Status==WebExceptionStatus.KeepAliveFailure;
    }
    internal static T Fetch<T>(string url,CancellationToken cancellation,Func<HttpWebResponse,T> read,
                              int timeout=15000,int attempts=3,Action<int> retry=null){
        for(int attempt=0;;attempt++){
            cancellation.ThrowIfCancellationRequested();var request=Engine.Request(url);
            request.Timeout=timeout;request.ReadWriteTimeout=timeout;
            try{using(cancellation.Register(request.Abort))using(var response=(HttpWebResponse)request.GetResponse())return read(response);}
            catch(Exception error){
                bool transient=Transient(error);var web=error as WebException;if(web!=null&&web.Response!=null)web.Response.Dispose();
                cancellation.ThrowIfCancellationRequested();
                if(!transient||attempt+1>=attempts)throw;
                if(retry!=null)retry(attempt+1);
                if(cancellation.WaitHandle.WaitOne(attempt==0?150:350))cancellation.ThrowIfCancellationRequested();
            }
        }
    }
}
