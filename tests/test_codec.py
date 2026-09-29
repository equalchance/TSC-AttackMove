"""Run the production event codec with isolated engine globals."""
from pathlib import Path
import json
import re
import subprocess
from test_behavior import function


def run(tree: Path, out: Path, compiler: str, environment: dict, commands_path: Path) -> dict:
    out.mkdir(parents=True, exist_ok=True)
    hooks=(tree/'src/extensions/event/eventext_hooks.cpp').read_text()
    events=(tree/'src/extensions/event/eventext.cpp').read_text()
    codec='\n'.join(function(hooks,s) for s in ['static int _Add_Compressed_Events(', 'static int _Extract_Compressed_Events(', 'static int Extract_Uncompressed_Events('])
    # Replace only global reads needing a running engine. Codec and queue logic are unchanged.
    codec=codec.replace('Session.CommProtocol','test_protocol').replace('Session.FrameSendRate','test_send_rate').replace('PlayerPtr->HeapID','test_house')
    lengths=re.search(r'unsigned char EventClassExt::EventLength\[EXT_EVENT_COUNT\] = \{.*?\};',events,re.S)[0]
    constructor=function(events,'EventClassExt::EventClassExt(int index, const TargetClass& unit, const Cell& cell)')
    classification=function(events,'bool EventClassExt::Is_Vinifera_Event(EventType type)')
    fixture=r'''
#include "always.h"
#include "eventext.h"
#include "queue.h"
#include "attackmove_logic.h"
#include "version.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <array>
#include <algorithm>
#undef DEBUG_INFO
#undef DEBUG_ERROR
#define DEBUG_INFO(...) ((void)0)
#define DEBUG_ERROR(...) ((void)0)
long frame_storage=100; long& Frame=frame_storage;
bool debug_storage=false; bool& Debug_Print_Events=debug_storage;
EventClass::EventClass() { std::memset(this,0,sizeof(*this)); }
QueueClass<EventClass,MAX_EVENTS> out_storage;
QueueClass<EventClass,MAX_EVENTS*64> do_storage;
QueueClass<EventClass,MAX_EVENTS>& OutList=out_storage;
QueueClass<EventClass,MAX_EVENTS*64>& DoList=do_storage;
int CurrentPacketPlayerId=1, test_house=1, test_send_rate=3;
CommProtocolType test_protocol=COMM_PROTOCOL_MULTI_E_COMP;
struct TestTarget:TargetClass { TestTarget(int id) { RTTI=RTTI_UNIT; ID=id; } };
'''+lengths+'\n'+constructor+'\n'+classification+'\n'+codec+r'''
int main() {
    assert(EventClassExt::Is_Vinifera_Event(static_cast<EventType>(36)));
    assert(EventClassExt::Is_Vinifera_Event(static_cast<EventType>(37)));
    assert(EXT_EVENT_ATTACK_MOVE == 38);
    assert(EventClassExt::Event_Length(static_cast<EventType>(36))==sizeof(EventClassExt::Data.PlayerOptions));
    assert(EventClassExt::Event_Length(static_cast<EventType>(37))==sizeof(EventClassExt::Data.ResponseTime2));
    assert(EventClassExt::Event_Length(static_cast<EventType>(38))==sizeof(EventClassExt::Data.AttackMove));
    using namespace AttackMoveLogic;
    for (int bits=0;bits<16;++bits) {
        const bool mobile=bits&1, allied=bits&2, passive=bits&4, civilian=bits&8;
        assert(Enemy_Unit(mobile,allied,passive,civilian)==(bits==1));
    }
    const TestTarget first(1), second(2), duplicate(1);
    assert(Same_Target<xTargetClass>(first,duplicate));
    assert(!Same_Target<xTargetClass>(first,second));
    const auto baseline=State_CRC(123,true,10,20,30,45,0);
    assert(baseline!=State_CRC(123,false,10,20,30,45,0));
    assert(baseline!=State_CRC(123,true,11,20,30,45,0));
    assert(baseline!=State_CRC(123,true,10,21,30,45,0));
    assert(baseline!=State_CRC(123,true,10,20,31,45,0));
    assert(baseline!=State_CRC(123,true,10,20,30,46,0));
    assert(baseline!=State_CRC(123,true,10,20,30,45,1));
    const Input transit={true,true,false,true,false,false,false,false};
    assert(Decide(transit)==Step::Transit);
    auto s=transit;s.active=false;assert(Decide(s)==Step::Inactive);
    s=transit;s.eligible=false;assert(Decide(s)==Step::Cancel);
    s=transit;s.allowed_mission=false;assert(Decide(s)==Step::Cancel);
    s=transit;s.suspended=true;assert(Decide(s)==Step::Suspend);
    s=transit;s.target_valid=true;assert(Decide(s)==Step::Engage);
    s.arrived=true;assert(Decide(s)==Step::Engage);
    s=transit;s.attack_mission=true;assert(Decide(s)==Step::Resume);
    s.arrived=true;assert(Decide(s)==Step::Finish);
    s=transit;s.movement_ended=true;assert(Decide(s)==Step::Finish);
    int timer=0, scans=0;
    for(int frame=0;frame<=150;++frame) {
        const bool due=Scan_Due(timer,15,frame);
        assert(due==(frame%15==0));scans+=due;
        assert(!Scan_Due(timer,15,frame)); // event execution + AI in the same frame
    }
    assert(scans==11);
    timer=0;assert(Scan_Due(timer,0,0)&&timer==15);
    timer=static_cast<int>(0xfffffff8u);
    assert(Scan_Due(timer,15,0xfffffff8u));
    assert(!Scan_Due(timer,15,0xffffffffu));
    assert(Scan_Due(timer,15,7));
    std::array<unsigned,4> ids={9,4,7,2};
    std::sort(ids.begin(),ids.end());
    do {
        unsigned best=100;
        for(auto id:ids) if(Better_Target(10,id,10,best)) best=id;
        assert(best==2);
    } while(std::next_permutation(ids.begin(),ids.end()));
    assert(!Queue_Fits(64,1,64)&&Queue_Fits(63,1,64)&&!Queue_Fits(0,65,64));
    assert(!Queue_Fits(0,0,64)&&!Queue_Fits(-1,1,64));
    for(unsigned crc: {0u,1u,0x12345678u,0xffffffffu}) {
        assert(Handshake(crc)!=crc);assert(Handshake(crc)==(crc^Protocol));
    }
    int cases=0;
    for(auto protocol: {COMM_PROTOCOL_MULTI_E_COMP,COMM_PROTOCOL_SINGLE_E_COMP}) {
      test_protocol=protocol;
      for(int n: {1,2,17,64}) {
        OutList.Init();DoList.Init();
        for(int i=0;i<n;++i) {
            EventClassExt e(1,TestTarget(i+10),Cell(11+i,23));
            assert(!e.IsExecuted && e.ID==1 && e.Frame==100);
            const auto* raw=reinterpret_cast<const unsigned char*>(&e.Data);
            for(size_t j=sizeof(e.Data.AttackMove);j<sizeof(e.Data);++j) assert(raw[j]==0);
            assert(OutList.Add(e.As_Event()));
        }
        char buffer[4096]={};
        const int header=offsetof(EventClass,Data)+sizeof(EventClass::Data.FrameInfo);
        EventClass frame;
        frame.Type=EVENT_FRAMEINFO;frame.ID=1;
        frame.Frame=protocol==COMM_PROTOCOL_MULTI_E_COMP?105:104;
        std::memcpy(buffer,&frame,header);
        int processed=-1;
        const int size=_Add_Compressed_Events(buffer,sizeof(buffer),4,header,64,processed);
        assert(processed==n && OutList.Count==0 && DoList.Count==n);
        std::array<EventClass,64> expected;
        for(int i=0;i<n;++i) expected[i]=DoList[i];
        DoList.Init();CurrentPacketPlayerId=1;
        assert(_Extract_Compressed_Events(buffer,size)==n+1);
        for(int i=0;i<n;++i) {
            auto& a=DoList[i+1];auto& b=expected[i];
            assert(a.Type==b.Type && a.ID==b.ID && a.Frame==b.Frame && !a.IsExecuted);
            assert(std::memcmp(&a.Data,&b.Data,sizeof(a.Data))==0);
        }
        // A forged issuer cannot enqueue any event.
        DoList.Init();CurrentPacketPlayerId=2;
        assert(_Extract_Compressed_Events(buffer,size)==0 && DoList.Count==0);
        CurrentPacketPlayerId=1;
        // Truncate inside the last fixed payload: only complete events may be added.
        DoList.Init();assert(_Extract_Compressed_Events(buffer,size-1)==n);
        ++cases;
      }
    }
    for (auto protocol : {COMM_PROTOCOL_MULTI_E_COMP,COMM_PROTOCOL_SINGLE_E_COMP}) {
      for (int event_id : {36,37}) {
        test_protocol=protocol;OutList.Init();DoList.Init();
        EventClass legacy;
        legacy.Type=static_cast<EventType>(event_id);legacy.ID=1;legacy.Frame=Frame;
        auto payload=reinterpret_cast<unsigned char*>(&legacy.Data);
        const int payload_size=EventClassExt::Event_Length(legacy.Type);
        for(int i=0;i<payload_size;++i) payload[i]=static_cast<unsigned char>(i+3);
        assert(OutList.Add(legacy));
        char wire[128]={};int processedLegacy=0;
        const int bytes=_Add_Compressed_Events(wire,sizeof(wire),0,0,64,processedLegacy);
        assert(processedLegacy==1 && bytes>0);
        CurrentPacketPlayerId=1;
        _Extract_Compressed_Events(wire,bytes);
        assert(DoList.Count==1 && DoList.First().Type==legacy.Type);
        assert(std::memcmp(&DoList.First().Data,&legacy.Data,payload_size)==0);
      }
    }
    OutList.Init();DoList.Init();
    EventClassExt e(1,TestTarget(1),Cell(20,30));OutList.Add(e.As_Event());
    char tiny[1]={};int processed=99;
    assert(_Add_Compressed_Events(tiny,1,0,0,64,processed)==0 && processed==0 && OutList.Count==1);
    // Event queue wrap-around preserves order and bytes.
    OutList.Init();
    for(int cycle=0;cycle<200;++cycle) { assert(OutList.Add(e.As_Event()));assert(OutList.First().Type==e.Type);OutList.Next(); }
    DoList.Init();CurrentPacketPlayerId=1;
    auto uncompressed=e.As_Event();
    uncompressed.IsExecuted=true;
    assert(Extract_Uncompressed_Events(&uncompressed,sizeof(uncompressed))==1);
    assert(!DoList.First().IsExecuted);
    assert(std::memcmp(&DoList.First().Data,&e.Data,sizeof(e.Data))==0);
    DoList.Init();CurrentPacketPlayerId=2;
    assert(Extract_Uncompressed_Events(&uncompressed,sizeof(uncompressed))==0 && DoList.Count==0);
    CurrentPacketPlayerId=1;
    assert(Extract_Uncompressed_Events(&uncompressed,sizeof(uncompressed)-1)==0);
    std::printf("PASS: decisions, cadence, queue boundaries/wrap, handshake, %d actual codec round trips, forged IDs and truncation\n",cases);
}
'''
    src=out/'native.cpp';src.write_text(fixture,encoding='utf-8')
    commands=json.loads(commands_path.read_text())
    proto=next(row for row in commands if row['file'].endswith('eventext.cpp'))
    _,args=proto['command'].split(' ',1)
    args=args[:args.index(' /Fo')]
    args=args.replace('/DNDEBUG','/UNDEBUG').replace('-DNDEBUG','/UNDEBUG').replace('/Gr','/Gd')
    args+=' /UNDEBUG'
    args+=' /Gy /Fo"'+str(out/'native.obj')+'" /Fd"'+str(out/'native.pdb')+'" /Fe"'+str(out/'native.exe')+'" "'+str(src)+'" /link /OPT:REF'
    rsp=out/'native.rsp';rsp.write_text(args)
    result=subprocess.run([compiler,'@'+str(rsp)],cwd=out,capture_output=True,text=True,env=environment)
    (out/'compile.log').write_text(result.stdout+result.stderr)
    if result.returncode:
        return {'name':'production event codec','status':'FAIL','output':result.stdout+result.stderr}
    result=subprocess.run([str(out/'native.exe')],cwd=out,capture_output=True,text=True,env=environment)
    (out/'run.log').write_text(result.stdout+result.stderr)
    return {'name':'production event codec','status':'PASS' if result.returncode==0 else 'FAIL',
            'exit_code':result.returncode,'output':result.stdout+result.stderr}
