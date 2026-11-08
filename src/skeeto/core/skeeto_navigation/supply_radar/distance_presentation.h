#pragma once
#include "radar_abi.h"
namespace distance_presentation {
struct State {
    bool valid=false;unsigned key=0,last=0;float shown=0;int number=0;
    int Update(float meters,unsigned target,unsigned now){
        if(!valid||key!=target||meters-shown>20||shown-meters>20){valid=true;key=target;shown=meters;number=int(meters+.5f);last=now;return number;}
        unsigned dt=now-last;last=now;if(dt>250)dt=250;float blend=float(dt)/(80.f+float(dt));
        shown+=(meters-shown)*blend;
        if(shown>float(number)+.65f||shown<float(number)-.65f)number=int(shown+.5f);
        return number;
    }
};
}
