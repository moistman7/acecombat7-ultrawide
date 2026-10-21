/* F8 diagnostic: logs the UI-sized draws that follow the key press (shaders, targets, viewport, quad bounds). */
static LONG drawLogBudget,drawLogMarker,rawDumpBudget,hudDumpBudget;
static LONG menuCaptureGeneration;
typedef struct {char text[900];int length;} DrawLine;
static void LineText(DrawLine *line,const char *s) {while(*s&&line->length<880)line->text[line->length++]=*s++;}
static void LineInt(DrawLine *line,long v) {
    char digits[16];int count=0;unsigned long u;if(v<0){LineText(line,"-");u=(unsigned long)(-v);}else u=(unsigned long)v;
    if(!u)digits[count++]='0';while(u&&count<15){digits[count++]=(char)('0'+u%10);u/=10;}while(count&&line->length<880)line->text[line->length++]=digits[--count];
}
static void LineHash(DrawLine *line,uint64_t h) {const char *x="0123456789abcdef";for(int shift=60;shift>=0&&line->length<880;shift-=4)line->text[line->length++]=x[(h>>shift)&15];}
static void LineSize(DrawLine *line,UINT w,UINT h) {LineInt(line,(long)w);LineText(line,"x");LineInt(line,(long)h);}
static void LineRound(DrawLine *line,float f) {LineInt(line,(long)(f+(f<0?-.5f:.5f)));}
static void LineFloat(DrawLine *line,float f) {if(!(f<1e9f&&f>-1e9f)){LineText(line,"big ");return;}if(f<0){LineText(line,"-");f=-f;}long whole=(long)f;long frac=(long)((f-(float)whole)*100+.5f);if(frac>99){frac=0;++whole;}LineInt(line,whole);LineText(line,".");if(frac<10)LineText(line,"0");LineInt(line,frac);LineText(line," ");}
static int TextureSize(ID3D11Resource *resource,UINT *w,UINT *h) {
    D3D11_RESOURCE_DIMENSION dim;ID3D11Resource_GetType(resource,&dim);if(dim!=D3D11_RESOURCE_DIMENSION_TEXTURE2D)return 0;
    D3D11_TEXTURE2D_DESC desc;ID3D11Texture2D_GetDesc((ID3D11Texture2D*)resource,&desc);*w=desc.Width;*h=desc.Height;return 1;
}
static void LogDraw(ID3D11DeviceContext *ctx,int kind,UINT count,UINT first,INT base) {
    if(drawLogBudget<=0)return;
    UINT rw=0,rh=0;ID3D11RenderTargetView *rt=0;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rt,0);
    if(rt){ID3D11Resource *res=0;ID3D11RenderTargetView_GetResource(rt,&res);if(res){TextureSize(res,&rw,&rh);ID3D11Resource_Release(res);}ID3D11RenderTargetView_Release(rt);}
    if(rw<1000)return;
    if(InterlockedDecrement(&drawLogBudget)<0)return;
    DrawLine line;line.length=0;
    ID3D11VertexShader *vs=0;ID3D11DeviceContext_VSGetShader(ctx,&vs,0,0);uint64_t vh=GetShaderHash(vs);
    ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t ph=GetShaderHash(ps);
    LineText(&line,"DRAW#");LineInt(&line,drawLogMarker);LineText(&line," kind=");LineInt(&line,kind);LineText(&line," n=");LineInt(&line,(long)count);
    LineText(&line," vs=");LineHash(&line,vh);LineText(&line," ps=");LineHash(&line,ph);LineText(&line," rt=");LineSize(&line,rw,rh);
    D3D11_VIEWPORT vp[16];UINT vpCount=16;ID3D11DeviceContext_RSGetViewports(ctx,&vpCount,vp);
    if(vpCount){LineText(&line," vp=");LineRound(&line,vp[0].TopLeftX);LineText(&line,",");LineRound(&line,vp[0].TopLeftY);LineText(&line,",");LineSize(&line,(UINT)vp[0].Width,(UINT)vp[0].Height);}
    D3D11_RECT sc[16];UINT scCount=16;ID3D11DeviceContext_RSGetScissorRects(ctx,&scCount,sc);
    if(scCount){LineText(&line," sc=");LineInt(&line,sc[0].left);LineText(&line,",");LineInt(&line,sc[0].top);LineText(&line,",");LineInt(&line,sc[0].right);LineText(&line,",");LineInt(&line,sc[0].bottom);}
    ID3D11ShaderResourceView *srv=0;ID3D11DeviceContext_PSGetShaderResources(ctx,0,1,&srv);
    if(srv){ID3D11Resource *res=0;ID3D11ShaderResourceView_GetResource(srv,&res);UINT tw=0,th=0;if(res){if(TextureSize(res,&tw,&th)){LineText(&line," t0=");LineSize(&line,tw,th);}ID3D11Resource_Release(res);}ID3D11ShaderResourceView_Release(srv);}
    int six=vh==0x8f3937c557a87704ull;
    if(kind==0&&count>=6&&!(count%6)&&count<=12288&&base>=0&&vpCount&&(six||vh==0x47897216336c5479ull)) {
        ID3D11Buffer *cb=0,*vb=0,*ib=0;UINT stride=0,offset=0,indexOffset=0;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
        ID3D11DeviceContext_VSGetConstantBuffers(ctx,0,1,&cb);ID3D11DeviceContext_IAGetVertexBuffers(ctx,0,1,&vb,&stride,&offset);ID3D11DeviceContext_IAGetIndexBuffer(ctx,&ib,&format,&indexOffset);
        BufferMirror *cm=Mirror((ID3D11Resource*)cb,1),*vm=Mirror((ID3D11Resource*)vb,1),*im=Mirror((ID3D11Resource*)ib,1);InitialSnapshot(ctx,cm);InitialSnapshot(ctx,vm);InitialSnapshot(ctx,im);
        if(cb)ID3D11Buffer_Release(cb);if(vb)ID3D11Buffer_Release(vb);if(ib)ID3D11Buffer_Release(ib);
        UINT indexSize=format==DXGI_FORMAT_R16_UINT?2:format==DXGI_FORMAT_R32_UINT?4:0;
        if(cm&&vm&&im&&cm->valid&&vm->valid&&im->valid&&stride==40&&cm->size>=96&&indexSize&&(uint64_t)indexOffset+((uint64_t)first+count)*indexSize<=im->size) {
            float *matrix=(float*)cm->data;UINT x=six?8:0,y=six?12:4,t=six?20:12;
            float minx=100000,maxx=-100000,miny=100000,maxy=-100000;int ok=1;
            for(UINT i=0;i<count&&ok;++i){BYTE *ip=im->data+indexOffset+(first+i)*indexSize;UINT index=indexSize==2?*(USHORT*)ip:*(UINT*)ip;uint64_t position=(uint64_t)offset+((uint64_t)(UINT)base+index)*stride;if(position+stride>vm->size){ok=0;break;}float *vertex=(float*)(vm->data+position);float px=vertex[6],py=vertex[7];float cx=px*matrix[x]+py*matrix[y]+matrix[t],cy=px*matrix[x+1]+py*matrix[y+1]+matrix[t+1];if(cx<minx)minx=cx;if(cx>maxx)maxx=cx;if(cy<miny)miny=cy;if(cy>maxy)maxy=cy;}
            if(ok){float left=vp[0].TopLeftX+(minx+1)*vp[0].Width*.5f,right=vp[0].TopLeftX+(maxx+1)*vp[0].Width*.5f,top=vp[0].TopLeftY+(1-maxy)*vp[0].Height*.5f,bottom=vp[0].TopLeftY+(1-miny)*vp[0].Height*.5f;
                LineText(&line," bounds=");LineRound(&line,left);LineText(&line,",");LineRound(&line,top);LineText(&line,",");LineRound(&line,right);LineText(&line,",");LineRound(&line,bottom);}
        }
    }
    if(kind==0&&vh==0x7f147c83c9952b66ull&&rawDumpBudget>0&&InterlockedDecrement(&rawDumpBudget)>=0) {
        ID3D11Buffer *cb=0,*vb=0;UINT stride=0,offset=0;
        ID3D11DeviceContext_VSGetConstantBuffers(ctx,0,1,&cb);ID3D11DeviceContext_IAGetVertexBuffers(ctx,0,1,&vb,&stride,&offset);
        BufferMirror *cm=Mirror((ID3D11Resource*)cb,1),*vm=Mirror((ID3D11Resource*)vb,1);InitialSnapshot(ctx,cm);InitialSnapshot(ctx,vm);
        if(cb)ID3D11Buffer_Release(cb);if(vb)ID3D11Buffer_Release(vb);
        DrawLine raw;raw.length=0;LineText(&raw,"RAW stride=");LineInt(&raw,(long)stride);LineText(&raw," off=");LineInt(&raw,(long)offset);LineText(&raw," base=");LineInt(&raw,base);LineText(&raw," first=");LineInt(&raw,(long)first);LineText(&raw," n=");LineInt(&raw,(long)count);
        if(cm&&cm->valid){LineText(&raw," cb0:");for(UINT i=0;i<24&&(i+1)*4<=cm->size;++i)LineFloat(&raw,((float*)cm->data)[i]);}
        if(vm&&vm->valid&&stride&&stride<=64&&base>=0){for(UINT v=0;v<2;++v){uint64_t position=(uint64_t)offset+((uint64_t)(UINT)base+v)*stride;if(position+stride>vm->size)break;LineText(&raw," v");LineInt(&raw,(long)v);LineText(&raw,":");for(UINT i=0;i<stride/4;++i)LineFloat(&raw,((float*)(vm->data+position))[i]);}}
        LineText(&raw,"\r\n");raw.text[raw.length]=0;UILog(raw.text);
    }
    /* HUD compositors: the blend-mode rectangles (cb0[19],[20]), mode switches (cb0[18],[23..25]) and view mapping (cb1[126..128]). */
    if((IsHUDFinalBlend(ph)||ph==0xeb4ce6dce4ada70aull)&&hudDumpBudget>0&&InterlockedDecrement(&hudDumpBudget)>=0) {
        ID3D11Buffer *cb[2]={0,0};ID3D11DeviceContext_PSGetConstantBuffers(ctx,0,2,cb);
        DrawLine raw;raw.length=0;LineText(&raw,"HUDCB ps=");LineHash(&raw,ph);
        for(UINT s=0;s<2;++s){BufferMirror *m=Mirror((ID3D11Resource*)cb[s],1);InitialSnapshot(ctx,m);if(cb[s])ID3D11Buffer_Release(cb[s]);
            UINT from=s?504:72,to=s?516:104;LineText(&raw,s?" cb1[126..128]:":" cb0[18..25]:");
            if(m&&m->valid)for(UINT i=from;i<to&&(i+1)*4<=m->size;++i)LineFloat(&raw,((float*)m->data)[i]);}
        LineText(&raw,"\r\n");raw.text[raw.length]=0;UILog(raw.text);
    }
    if(vs)ID3D11VertexShader_Release(vs);if(ps)ID3D11PixelShader_Release(ps);
    LineText(&line,"\r\n");line.text[line.length]=0;UILog(line.text);
}

/* Runtime debug toggles: F10 hides the UI-plane composite, F11 logs why ApplyUI skips the menu background. */
static LONG skipPlate;
static int ShouldSkipPlate(ID3D11DeviceContext *ctx) {
    ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t hash=GetShaderHash(ps);if(ps)ID3D11PixelShader_Release(ps);
    if(hash!=0x96e1f6f048250fecull)return 0;
    UINT w=0,h=0;return PrimaryTarget(ctx,&w,&h)&&w*9>h*16;
}
static void BailLog(ID3D11DeviceContext *ctx,int line) {
    ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t hash=GetShaderHash(ps);if(ps)ID3D11PixelShader_Release(ps);
    if(hash!=0x5eae2582c19da3f5ull)return;
    if(InterlockedDecrement(&bailBudget)<0)return;
    DrawLine out;out.length=0;LineText(&out,line<0?"ApplyUI(5eae): background widened\r\n":"ApplyUI(5eae): bailed at UIAdjust.c line ");
    if(line>=0){LineInt(&out,line);LineText(&out,"\r\n");}
    out.text[out.length]=0;UILog(out.text);
}

/* Timestamped logging for the debug keys, so screenshots (named by time) can be matched to key state. */
static void StampLog(const char *message) {
    SYSTEMTIME now;GetLocalTime(&now);DrawLine out;out.length=0;
    if(now.wHour<10)LineText(&out,"0");LineInt(&out,now.wHour);LineText(&out,":");
    if(now.wMinute<10)LineText(&out,"0");LineInt(&out,now.wMinute);LineText(&out,":");
    if(now.wSecond<10)LineText(&out,"0");LineInt(&out,now.wSecond);LineText(&out," ");
    LineText(&out,message);out.text[out.length]=0;UILog(out.text);
}

/* F8 or the Steam screenshot key (F12): log the next large-target draws and the ApplyUI decisions, with the debug-key state. */
static void ArmDrawLog(const char *key) {
    if(!diagnostics)return; /* Steam screenshots must not trigger readbacks/log IO in release mode. */
    if(diagnostics){InterlockedIncrement(&menuCaptureGeneration);FILETIME stamp;GetSystemTimeAsFileTime(&stamp);InterlockedExchange64(&hudCaptureSerial,((LONG64)stamp.dwHighDateTime<<32)|stamp.dwLowDateTime);InterlockedExchange(&hudCaptureMask,3);}
    InterlockedIncrement(&drawLogMarker);InterlockedExchange(&drawLogBudget,2500);InterlockedExchange(&bailBudget,60);InterlockedExchange(&rawDumpBudget,6);InterlockedExchange(&hudDumpBudget,4);
    DrawLine out;out.length=0;LineText(&out,"---- ");LineText(&out,key);LineText(&out," draw log #");LineInt(&out,drawLogMarker);
    LineText(&out," | F9 widen=");LineInt(&out,forceWiden);LineText(&out," F10 hidePlate=");LineInt(&out,skipPlate);
    LineText(&out," hudMode=");LineInt(&out,hudMode);LineText(&out," legacy=");LineInt(&out,legacyUI);LineText(&out," text=");LineInt(&out,textCorrection);LineText(&out," edge=");LineInt(&out,menuEdgeExtension);
    LineText(&out," ----\r\n");out.text[out.length]=0;StampLog(out.text);
}
