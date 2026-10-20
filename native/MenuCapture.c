/* Bounded source-layer capture, armed only by an explicit diagnostic request.
   Retain resources while deduplicating so reused addresses cannot alias a slot. */
typedef struct {ID3D11Resource *source,*target;uint64_t shader;} MenuCaptureEntry;
static MenuCaptureEntry menuCaptureEntries[12];
static UINT menuCaptureCount;
static LONG menuCaptureObserved;
static ULONGLONG menuCaptureDeadline;
static void CaptureMenuLayer(ID3D11DeviceContext *ctx,UINT count,UINT first,INT base,UINT indexed) {
    if(!diagnostics)return;
    if(menuCaptureObserved!=menuCaptureGeneration){
        for(UINT i=0;i<menuCaptureCount;++i){ID3D11Resource_Release(menuCaptureEntries[i].source);ID3D11Resource_Release(menuCaptureEntries[i].target);}
        menuCaptureCount=0;menuCaptureObserved=menuCaptureGeneration;menuCaptureDeadline=GetTickCount64()+2000;
    }
    if(!menuCaptureObserved||menuCaptureCount>=12||GetTickCount64()>menuCaptureDeadline||ID3D11DeviceContext_GetType(ctx)!=D3D11_DEVICE_CONTEXT_IMMEDIATE)return;
    ID3D11PixelShader *ps=0;ID3D11DeviceContext_PSGetShader(ctx,&ps,0,0);uint64_t hash=GetShaderHash(ps);if(ps)ID3D11PixelShader_Release(ps);
    if(hash!=0x96e1f6f048250fecull&&hash!=0x87d48047026c963dull&&hash!=0x2d58759ed62f1f84ull&&hash!=0x155ebdf3384010f4ull)return;
    ID3D11ShaderResourceView *srv=0;ID3D11DeviceContext_PSGetShaderResources(ctx,0,1,&srv);if(!srv)return;
    ID3D11Resource *source=0;ID3D11ShaderResourceView_GetResource(srv,&source);ID3D11ShaderResourceView_Release(srv);
    UINT width=0,height=0;if(!TextureSize(source,&width,&height)||width!=1920||height!=1080){ID3D11Resource_Release(source);return;}
    ID3D11RenderTargetView *rt=0;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rt,0);if(!rt){ID3D11Resource_Release(source);return;}
    ID3D11Resource *target=0;ID3D11RenderTargetView_GetResource(rt,&target);ID3D11RenderTargetView_Release(rt);
    for(UINT i=0;i<menuCaptureCount;++i)if(menuCaptureEntries[i].source==source&&menuCaptureEntries[i].target==target&&menuCaptureEntries[i].shader==hash){ID3D11Resource_Release(source);ID3D11Resource_Release(target);return;}
    UINT slot=menuCaptureCount++;menuCaptureEntries[slot]=(MenuCaptureEntry){source,target,hash};
    WCHAR stem[90],part[17];HexName(part,(uint64_t)menuCaptureObserved);lstrcpyW(stem,L"menu-");lstrcatW(stem,part);lstrcatW(stem,L"-");HexName(part,slot);lstrcatW(stem,part+14);lstrcatW(stem,L"-");HexName(part,hash);lstrcatW(stem,part);lstrcatW(stem,L"-source");CaptureTexture(ctx,source,stem);
    ID3D11Buffer *cb[2]={0,0},*vb=0,*ib=0;UINT stride=0,offset=0,indexOffset=0;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
    ID3D11DeviceContext_VSGetConstantBuffers(ctx,0,2,cb);ID3D11DeviceContext_IAGetVertexBuffers(ctx,0,1,&vb,&stride,&offset);ID3D11DeviceContext_IAGetIndexBuffer(ctx,&ib,&format,&indexOffset);
    WCHAR name[100];const WCHAR *suffix[4]={L"-vs-cb0.bin",L"-vs-cb1.bin",L"-vb.bin",L"-ib.bin"};ID3D11Buffer *buffers[4]={cb[0],cb[1],vb,ib};
    for(UINT b=0;b<4;++b)if(buffers[b]){lstrcpyW(name,stem);lstrcatW(name,suffix[b]);DumpBuffer(ctx,buffers[b],name);ID3D11Buffer_Release(buffers[b]);}
    UINT args[8]={count,first,(UINT)base,stride,offset,indexOffset,(UINT)format,indexed};lstrcpyW(name,stem);lstrcatW(name,L"-draw.bin");SaveData(name,args,sizeof(args));
    DrawLine line;line.length=0;LineText(&line,"MENU_LAYER generation=");LineInt(&line,menuCaptureObserved);LineText(&line," slot=");LineInt(&line,(long)slot);LineText(&line," ps=");LineHash(&line,hash);TextureSize(target,&width,&height);LineText(&line," target=");LineSize(&line,width,height);LineText(&line," srcId=");LineInt(&line,MenuTraceId(source));LineText(&line," dstId=");LineInt(&line,MenuTraceId(target));LineText(&line," art=");LineInt(&line,MenuArt(source));LineText(&line,"\r\n");line.text[line.length]=0;UILog(line.text);
}
