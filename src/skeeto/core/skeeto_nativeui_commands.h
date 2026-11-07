#pragma once
static void NativeUiCommand(int* cmd,int target) {
    using namespace sknativeui;
    const int argc=CmdArgC(cmd);const char* action=CmdArg(cmd,1);
    Config c=NativeUiConfig(target);bool changed=false;
    if(argc==1 || (argc==2 && !_stricmp(action,"status"))){
        char status[224]{};const unsigned b=g_nativeUiBindings[target].load();
        snprintf(status,sizeof(status),"[Skeeto] %s enabled=%d status=%d; root=%d history=%d input=%d outputColors=%d frameColors=%d (status: 0 off, 1 waiting, 2 active, 3 unsupported, 4 fault/font)",
            target?"console_ui":"chat_ui",c.v[Enabled],g_nativeUiStatus[target].load(),!!(b&1),!!(b&2),!!(b&4),!!(b&128),!!(b&256));ConEcho(status);
        if(argc==2)return;
    }
    if(argc==2 && (!_stricmp(action,"on") || !_stricmp(action,"off"))){c.v[Enabled]=!_stricmp(action,"on");changed=true;}
    else if(argc==2 && !_stricmp(action,"reset")){const int on=c.v[Enabled];c=Defaults(target);c.v[Enabled]=on;changed=true;}
    else if(argc==4 && !_stricmp(action,"set")){
        for(int i=0;i<Count;++i)if(!strcmp(CmdArg(cmd,2),kFields[i].key)){
            if(!Applies((Field)i,target))break;
            int value=0;if(!WorldNamesParseInt(CmdArg(cmd,3),kFields[i].lo,kFields[i].hi,&value))break;
            c.v[i]=value;changed=true;break;
        }
    }else if(argc==6 && !_stricmp(action,"color")){
        static constexpr struct {const char* name;Field field;} colors[]={
            {"text",Text},{"input",Input},{"background",Background},{"input_background",InputBackground},
            {"survivor",Survivor},{"infected",Infected},{"developer",Developer},
            {"title_text",TitleText},{"title_background",TitleBackground},
        };
        int r=0,g=0,b=0;
        if(WorldNamesParseInt(CmdArg(cmd,3),0,255,&r) && WorldNamesParseInt(CmdArg(cmd,4),0,255,&g)
            && WorldNamesParseInt(CmdArg(cmd,5),0,255,&b)){
            for(const auto& item:colors)if(!strcmp(CmdArg(cmd,2),item.name)){
                if((target && (item.field==Survivor || item.field==Infected))
                    || (!target && item.field>=Developer))break;
                c.v[item.field]=(r<<16)|(g<<8)|b;changed=true;break;
            }
        }
    }
    if(changed){NativeUiChanged(target,c);ConEcho("[Skeeto] Native UI settings updated; native controls update on Paint.");return;}
    char line[256]{};const char* name=target?"skeeto_console_ui":"skeeto_chat_ui";
    snprintf(line,sizeof(line),"%s: on | off | reset | set <key> <integer> | color <target> R G B",name);ConEcho(line);
    ConEcho("[Skeeto] Colors: text, input, background, input_background, survivor/infected (chat), developer/title_text/title_background (console).");
}
static void SkCmd_ChatUi(int* cmd){NativeUiCommand(cmd,0);}
static void SkCmd_ConsoleUi(int* cmd){NativeUiCommand(cmd,1);}
