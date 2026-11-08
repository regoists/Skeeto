#pragma once
// Fixed calendar-day totals. Worker only; independent of the ten detail slots.
#include <ctime>
#include <cstdint>
#include "skeeto_mainui_services.h"

struct MainUiBattleDay { int date=0; int64_t common=0,special=0; };
struct MainUiBattleDays { MainUiBattleDay rows[7]{}; };
static int MainUiBattleDate(const tm& day) {
    return (day.tm_year+1900)*10000+(day.tm_mon+1)*100+day.tm_mday;
}
static bool MainUiBattleWindow(time_t now,int (&keys)[7]) {
    tm today{};if(localtime_s(&today,&now))return false;
    today.tm_hour=today.tm_min=today.tm_sec=0;today.tm_mday-=6;
    for(int i=0;i<7;++i){tm day=today;day.tm_mday+=i;day.tm_isdst=-1;
        if(mktime(&day)==(time_t)-1)return false;keys[i]=MainUiBattleDate(day);}
    return true;
}
static bool MainUiBattleDaysValid(const MainUiBattleDays& days) {
    for(int i=0;i<7;++i){const auto& row=days.rows[i];
        if(row.common<0||row.special<0)return false;
        if(!row.date){if(row.common||row.special)return false;continue;}
        if(row.date<0)return false;
        tm day{};day.tm_year=row.date/10000-1900;day.tm_mon=row.date/100%100-1;day.tm_mday=row.date%100;day.tm_hour=12;day.tm_isdst=-1;
        if(mktime(&day)==(time_t)-1||MainUiBattleDate(day)!=row.date)return false;
        for(int j=0;j<i;++j)if(days.rows[j].date==row.date)return false;
    }
    return true;
}
static void MainUiBattleDaysAdd(MainUiBattleDays& days,const MainUiBattleRecord& record,time_t now) {
    if(record.result!=0)return;
    int keys[7]{};tm completed{};time_t stamp=(time_t)record.time;
    if(!MainUiBattleWindow(now,keys)||localtime_s(&completed,&stamp))return;
    const int date=MainUiBattleDate(completed);bool inWindow=false;
    for(int key:keys)if(key==date)inWindow=true;
    int slot=-1;
    for(int i=0;i<7;++i){auto& row=days.rows[i];bool keep=false;
        for(int key:keys)if(row.date==key)keep=true;
        if(!keep)row={};if(row.date==date)slot=i;
    }
    if(!inWindow)return;
    if(slot<0)for(int i=0;i<7;++i)if(!days.rows[i].date){slot=i;days.rows[i].date=date;break;}
    if(slot<0)return; // All seven distinct dates can only fill the seven-day window.
    auto& row=days.rows[slot];
    if(record.statCount>0&&record.stats[0]>0)row.common+=record.stats[0];
    for(int k=1;k<=6&&k<record.statCount;++k)if(record.stats[k]>0)row.special+=record.stats[k];
}
