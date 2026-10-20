/* No readbacks: bounded identity history plus a two-second F12 trace. */
static const GUID menuTraceIdTag={0x948faa37,0xc242,0x493b,{0xaa,0x7d,0x64,0xd2,0xaf,0x33,0x41,0xd8}};
static LONG menuTraceNextId,menuTraceSequence,menuTraceHistory,menuTraceGeneration,menuTraceBudget;
static ULONGLONG menuTraceDeadline;
static LONG MenuTraceId(ID3D11Resource *resource) {
    LONG id=0;UINT size=sizeof(id);if(!resource)return 0;
    ID3D11Resource_GetPrivateData(resource,&menuTraceIdTag,&size,&id);
    if(!id){id=InterlockedIncrement(&menuTraceNextId);ID3D11Resource_SetPrivateData(resource,&menuTraceIdTag,sizeof(id),&id);}return id;
}
static void TraceMenuArt(const char *event,ID3D11Resource *src,ID3D11Resource *dst,uint64_t ps,UINT count,int full,UINT old,UINT next) {
    if(!diagnostics)return;
    if(menuTraceGeneration!=menuCaptureGeneration){menuTraceGeneration=menuCaptureGeneration;menuTraceBudget=2048;menuTraceDeadline=GetTickCount64()+2000;}
    UINT tagged=MenuArt(src)||old||next;
    int capture=menuTraceGeneration&&GetTickCount64()<=menuTraceDeadline&&menuTraceBudget>0;
    if(!capture&&(!tagged||menuTraceHistory>=256))return;
    UINT sw=0,sh=0,dw=0,dh=0;if(src)TextureSize(src,&sw,&sh);if(dst)TextureSize(dst,&dw,&dh);
    if(!tagged&&!(sw==1920&&sh==1080)&&!(dw==1920&&dh==1080))return;
    if(capture)--menuTraceBudget;else ++menuTraceHistory;
    DrawLine line;line.length=0;LineText(&line,"MENU_TRACE seq=");LineInt(&line,InterlockedIncrement(&menuTraceSequence));LineText(&line," gen=");LineInt(&line,menuTraceGeneration);
    LineText(&line," event=");LineText(&line,event);LineText(&line," srcId=");LineInt(&line,MenuTraceId(src));LineText(&line," src=");LineSize(&line,sw,sh);LineText(&line," srcArt=");LineInt(&line,MenuArt(src));
    LineText(&line," dstId=");LineInt(&line,MenuTraceId(dst));LineText(&line," dst=");LineSize(&line,dw,dh);LineText(&line," old=");LineInt(&line,old);LineText(&line," next=");LineInt(&line,next);
    LineText(&line," ps=");LineHash(&line,ps);LineText(&line," count=");LineInt(&line,count);LineText(&line," full=");LineInt(&line,full);LineText(&line,"\r\n");line.text[line.length]=0;UILog(line.text);
    if(capture&&!menuTraceBudget)UILog("MENU_TRACE capture event limit reached.\r\n");
}
