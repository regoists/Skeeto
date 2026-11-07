#pragma once
// Included after the plugin's VGUI text/rectangle helpers.
static unsigned worldFont;static int worldFontSize;
static unsigned WorldArrowColor(int color){
    static constexpr unsigned colors[]={0x65dcd1,0x66dc65,0xffd34e,0xff9b45,0x60a5fa,0xbf8cff,0xebf0f5};
    return colors[Limit(color,0,6)];
}
static void WorldTextFit(int x,int y,const WCHAR* text,int width,int pixels,unsigned color){
    WCHAR clipped[64]{};minimap::FitText(clipped,64,text,width,pixels);Text(x,y,clipped,color,worldFont);
}
static void WorldLine(worldnav::Point a,worldnav::Point b,unsigned color,int alpha,int radius);
static void WorldHudPosition(int width,int height,int boxW,int boxH,int& x,int& y){
    constexpr int margin=8;
    const int screenX=Limit(width-2*margin,0,width),travelY=Limit(height-2*margin-boxH,0,height);
    const int center=margin+(screenX*settings.worldHudX+5000)/10000;
    x=Limit(center-boxW/2,margin,Limit(width-margin-boxW,margin,width));
    y=margin+(travelY*settings.worldHudY+5000)/10000;
}
static void DrawWorldHudEditPreview(int width,int height,int pixels){
    const WCHAR* message=UiText(UiId::NavHud26,L"沿地面箭头前进");
    int chars=0;while(message[chars]&&chars<30)++chars;
    const int boxW=Limit((chars+3)*pixels+20,210,width/2),boxH=pixels+20;
    int x=0,y=0;WorldHudPosition(width,height,boxW,boxH,x,y);
    const unsigned color=WorldArrowColor(settings.worldArrowColor);
    Rect(x,y,boxW,boxH,10,20,27,190);Rect(x,y,2,boxH,(color>>16)&255,(color>>8)&255,color&255,210);
    const int cx=x+21,cy=y+boxH/2;
    worldnav::Point tip{float(cx),float(cy-6),0},a{float(cx-6),float(cy+5),0},b{float(cx+6),float(cy+5),0};
    WorldLine(a,tip,color,230,1);WorldLine(b,tip,color,230,1);
    Text(x+40,y+9,message,color,worldFont);
    Rect(x-2,y-2,boxW+4,2,242,188,112,255);Rect(x-2,y+boxH,boxW+4,2,242,188,112,255);
    Rect(x-2,y,2,boxH,242,188,112,255);Rect(x+boxW,y,2,boxH,242,188,112,255);
    const int hintW=Limit(width/3,300,420),hintH=2*(pixels+4)+16,hintX=(width-hintW)/2,hintY=height-hintH-18;
    Rect(hintX,hintY,hintW,hintH,17,26,32,220);
    WorldTextFit(hintX+8,hintY+6,UiText(UiId::NavWorldEditHint,L"方向键移动位置（按住连移，Shift 大步）"),hintW-16,pixels,minimap::Amber);
    WorldTextFit(hintX+8,hintY+pixels+10,UiText(UiId::NavWorldEditExitHint,L"回车或 Esc 完成"),hintW-16,pixels,0xdce7e8);
}
static void WorldLine(worldnav::Point a,worldnav::Point b,unsigned color,int alpha,int radius){
    VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,11)(surface,(color>>16)&255,(color>>8)&255,color&255,alpha);
    bool horizontal=nav::Abs(b.x-a.x)>nav::Abs(b.y-a.y);
    for(int i=-radius;i<=radius;++i)VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,15)(surface,int(a.x)+(horizontal?0:i),int(a.y)+(horizontal?i:0),int(b.x)+(horizontal?0:i),int(b.y)+(horizontal?i:0));
}
static void WorldStroke(worldnav::Point a,worldnav::Point b,unsigned color,int alpha,int radius){
    WorldLine(a,b,0x08171d,alpha*3/4,radius+1);WorldLine(a,b,color,alpha,radius);
}
static void DrawWorldNavigation(int width,int height){
    if(!settings.navigation||!settings.worldNavigation){worldOverlay.Clear();return;}
    if(!worldHudEditing&&(mapEditor.active||GameTextInputVisible()))return;
    int pixels=Limit(height/80,13,20);
    if(!worldFont)worldFont=VCall<unsigned(__thiscall*)(void*)>(surface,63)(surface);
    if(worldFont&&worldFontSize!=pixels&&VCall<bool(__thiscall*)(void*,unsigned,const char*,int,int,int,int,int,int,int)>(surface,64)(surface,worldFont,"Microsoft YaHei",pixels,500,0,0,0x90,0,0xffff))worldFontSize=pixels;
    if(worldHudEditing){DrawWorldHudEditPreview(width,height,pixels);++worldPaints;return;}
    performance::Measure cost(worldCost);
    worldnav::Camera camera;unsigned now=GetTickCount();bool routed=worldnav::Routed(navigator);
    bool cameraOK=false;
    if(worldReady){const auto& matrix=VCall<const Matrix&(__thiscall*)(void*)>(engine,37)(engine);cameraOK=Readable(&matrix,sizeof matrix)&&camera.Set(matrix,width,height);}
    if(worldReady&&!cameraOK)return;
    const unsigned arrowColor=WorldArrowColor(settings.worldArrowColor);
    if(cameraOK){
        worldOverlay.Prepare(mesh,navigator,view.player,meshSerial,now);
        worldnav::TraceContext context{worldTrace,nullptr};int local=VCall<int(__thiscall*)(void*)>(engine,12)(engine);
        if(local>0&&local<=64)context.local=VCall<void*(__thiscall*)(void*,int)>(entities,3)(entities,local);
        auto started=performance::Now();unsigned rays=0;
        if(routed){rays+=worldOverlay.CheckGround(camera,now,worldnav::CastScene,&context);
            for(int i=0;i<3&&performance::Microseconds(performance::Now()-started)<350;++i){unsigned used=worldOverlay.CheckOne(camera,now,worldnav::CastScene,&context);rays+=used;if(!used)break;}}
        worldOverlay.visibleCount=0;
        if(routed)for(unsigned i=0;i<worldOverlay.path.count;++i){const auto& mark=worldOverlay.path.marks[i];
            if(!mark.grounded||!mark.visible||!worldOverlay.Fresh(mark,camera.eye,now))continue;
            worldnav::Point p[3];if(!camera.Project(mark.points[0],p[0])||!camera.Project(mark.points[1],p[1])||!camera.Project(mark.points[2],p[2]))continue;
            float size=nav::Max(nav::Abs(p[1].x-p[2].x),nav::Abs(p[1].y-p[2].y));if(size<4||size>150||p[0].y>height*.86f)continue;
            int opacity=Limit(int(220-p[0].depth*.055f),120,220),stroke=size>28?2:1;
            WorldStroke(p[1],p[0],arrowColor,opacity,stroke);WorldStroke(p[2],p[0],arrowColor,opacity,stroke);++worldOverlay.visibleCount;
        }
        // The destination is a world marker only when a route exists and both
        // its base and label position can be seen. Direction-only uses the card.
        if(routed&&worldOverlay.path.hasTarget){
            Vec base=worldOverlay.path.target+Vec{0,0,8},top=worldOverlay.path.target+Vec{0,0,40},distance=base-camera.eye;
            worldnav::Point p,q;Vec eyeDelta=camera.eye-worldOverlay.targetEye;
            bool fresh=worldOverlay.targetChecked&&now-worldOverlay.targetAt<=180&&eyeDelta.Dot(eyeDelta)<32*32;
            bool candidate=distance.Dot(distance)<worldnav::Reach*worldnav::Reach&&camera.Project(top,p)&&camera.Project(base,q);
            if(candidate&&!fresh&&rays<=12&&performance::Microseconds(performance::Now()-started)<350){
                worldOverlay.targetVisible=worldnav::ClearSight(worldnav::CastScene,&context,camera.eye,base)&&worldnav::ClearSight(worldnav::CastScene,&context,camera.eye,top);
                worldOverlay.traceCalls+=2;worldOverlay.targetEye=camera.eye;worldOverlay.targetAt=now;worldOverlay.targetChecked=true;fresh=true;
            }
            if(candidate&&fresh&&worldOverlay.targetVisible&&p.x>14&&p.x<width-14&&p.y>14&&p.y<height-14){
                worldnav::Point diamond[4]={{p.x,p.y-8,0},{p.x+8,p.y,0},{p.x,p.y+8,0},{p.x-8,p.y,0}};
                WorldStroke({p.x,p.y+8,0},q,arrowColor,180,0);
                for(int j=0;j<4;++j)WorldStroke(diamond[j],diamond[(j+1)%4],arrowColor,230,1);
                if(p.x+pixels*5<width-8&&p.y>pixels+8)Text(int(p.x)+14,int(p.y)-pixels/2,navigator.selected>=0&&navigator.goals[navigator.selected].rescue?UiText(UiId::NavHud27, L"撤离点"):UiText(UiId::NavHud28, L"安全屋"),arrowColor,worldFont);
            }
        }
    }
    const WCHAR* message=UiText(UiId::NavHud29, L"场景接口不可用");unsigned color=minimap::Amber;float right=0,front=1;
    if(!worldReady)message=UiText(UiId::NavHud30, L"场景导航接口不可用");
    else if(!routed){auto status=minimap::NavigationStatus(navigator,false);message=status.text;color=status.color;
        if(worldOverlay.path.hasTarget)camera.Bearing(worldOverlay.path.target,right,front);
    }else {
        message=worldnav::CueText(worldOverlay.path.cue);color=0x65dcd1;
        if(worldOverlay.path.cue==worldnav::Cue::Special||worldOverlay.path.cue==worldnav::Cue::LadderUp||worldOverlay.path.cue==worldnav::Cue::LadderDown)color=minimap::Amber;
        if(worldOverlay.path.hasNext&&navigator.state!=guidance::State::AtGoal){camera.Bearing(worldOverlay.path.next,right,front);
            if(front<0)message=UiText(UiId::NavHud31, L"路线在身后 请转身");
            else if(nav::Abs(right)>front*1.6f)message=right<0?UiText(UiId::NavHud32, L"路线在左侧"):UiText(UiId::NavHud33, L"路线在右侧");
            else if(worldOverlay.path.cue==worldnav::Cue::Left){right=-2;front=1;}
            else if(worldOverlay.path.cue==worldnav::Cue::Right){right=2;front=1;}
        }
    }
    int chars=0;while(message[chars]&&chars<30)++chars;
    int boxW=Limit((chars+3)*pixels+20,210,width/2),boxH=pixels+20,x=0,y=0;
    WorldHudPosition(width,height,boxW,boxH,x,y);
    Rect(x,y,boxW,boxH,10,20,27,190);Rect(x,y,2,boxH,(color>>16)&255,(color>>8)&255,color&255,210);
    float cx=float(x+21),cy=float(y+boxH/2);worldnav::Point tip{cx,cy-6,0},a{cx-6,cy+5,0},b{cx+6,cy+5,0};
    if(front<0){tip={cx,cy+6,0};a={cx-6,cy-5,0};b={cx+6,cy-5,0};}
    else if(nav::Abs(right)>front*.7f){float side=right<0?-1.f:1.f;tip={cx+6*side,cy,0};a={cx-5*side,cy-6,0};b={cx-5*side,cy+6,0};}
    if(worldReady&&navigator.state==guidance::State::AtGoal){Rect(int(cx)-3,int(cy)-3,6,6,(arrowColor>>16)&255,(arrowColor>>8)&255,arrowColor&255,230);}
    else if(worldReady&&(routed||worldOverlay.path.hasTarget)){WorldLine(a,tip,arrowColor,230,1);WorldLine(b,tip,arrowColor,230,1);}
    Text(x+40,y+9,message,color,worldFont);++worldPaints;
}

