"""Exercise loading-edge numeric leases without an engine or player settings."""
import test_fast_load as native

helpers = r'''
static int experimentInts[]{1,2,1};
static float experimentFloats[]{1.f,2.5f,1.f};
static unsigned experimentReads=0,experimentWrites=0;
static bool experimentWriteOk=true;
static unsigned ExperimentIndex(const char* name){
    if(!std::strcmp(name,"mod_forcedata"))return 0;
    if(!std::strcmp(name,"mod_forcetouchdata"))return 1;
    require(!std::strcmp(name,"cl_forcepreload"),"known experiment switch");return 2;
}
static bool ExperimentRead(const char* name,int* n,float* f){
    ++experimentReads;const auto i=ExperimentIndex(name);*n=experimentInts[i];*f=experimentFloats[i];return true;
}
static bool ExperimentWrite(const char* name,int n,float f){
    ++experimentWrites;if(!experimentWriteOk)return false;
    const auto i=ExperimentIndex(name);experimentInts[i]=n;experimentFloats[i]=f;return true;
}
static void CheckExperiments(){
    skload::LoadingExperiments e;e.read=ExperimentRead;e.write=ExperimentWrite;
    e.Begin(0);require(experimentReads==0 && experimentWrites==0,"disabled experiments do no native work");
    e.Begin(1);require(e.active && experimentInts[0]==0 && experimentInts[1]==2 && experimentInts[2]==1,"selected experiment alone changes");
    e.Begin(1);require(experimentWrites==1 && experimentReads==1,"repeated redraw does no extra lease work");
    require(e.Complete() && experimentInts[0]==1 && !e.active,"loading completion restores original value");
    e.Begin(7);experimentInts[2]=3;experimentFloats[2]=3.f;
    require(e.Complete() && experimentInts[1]==2 && experimentFloats[1]==2.5f && experimentInts[2]==3 && e.conflicts==1,"preserve exact originals and foreign script changes");
    e.Begin(1);experimentWriteOk=false;
    require(!e.Complete() && e.active && e.failures==1,"failed restore retains ownership for explicit cleanup");
    experimentWriteOk=true;require(e.Complete() && !e.active,"explicit cleanup can recover");
    const unsigned writes=experimentWrites;experimentInts[0]=0;experimentFloats[0]=0.f;e.Begin(1);
    require(!e.active && experimentWrites==writes,"already disabled forcing needs no write or restore");
    require(skload::ConfiguredMode(true,false,false,false,false,false,false,false,1)==257,"experiment alone keeps the loading and completion hook");
    require(!skload::ConfiguredMode(false,true,true,true,true,true,true,true,7),"master off gates all experiments");
}
'''
assert native.FIXTURE.count('int main() {') == 1
native.FIXTURE = native.FIXTURE.replace('int main() {', helpers+'\nint main() {\n    CheckExperiments();')
if __name__ == '__main__':
    native.main()
