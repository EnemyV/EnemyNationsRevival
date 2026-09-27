"""Test actual queued TCP writes across partial writes, would-block, and corruption.

Extracts production Datagram, calcsum, and SendWaitingData; stubs socket I/O.
"""
from pathlib import Path
import argparse, subprocess, tempfile
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref', help='Extract TCP implementation from a Git revision.')
parser.add_argument('--opt', choices=['Od', 'O2'], default='O2')
args = parser.parse_args()
root=Path(__file__).resolve().parents[2]
out=Path(tempfile.mkdtemp(prefix='en-tcp-write-'))
tcp=(root/'tools/vdmplay/tcpnet.cpp').read_text(encoding='utf-8')
if args.baseline_ref:
 tcp = subprocess.check_output(['git', '-C', str(root), 'show',
     args.baseline_ref + ':tools/vdmplay/tcpnet.cpp']).decode('utf-8')
dat=(root/'tools/vdmplay/datagram.h').read_text(encoding='utf-8')
def body(text,signature):
 i=text.index(signature); j=text.index('{',i); depth=1; k=j+1
 while depth:
  depth+=(text[k]=='{')-(text[k]=='}'); k+=1
 return text[i:k]
prefix=r'''
#include <winsock2.h>
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#define VPASSERT(x) ((void)0)
#define VPTRACE(x) ((void)0)
#define _fmemcpy memcpy
struct CVPLink {};
'''
prefix+=body(dat,'class Datagram :')+';\n'
prefix+=r'''
struct DataQueue {
 std::deque<Datagram*> q;
 void Add(Datagram* d){q.push_back(d);} void Insert(Datagram*d){q.push_front(d);}
 unsigned Count(){return (unsigned)q.size();}
 Datagram* Get(){if(q.empty())return nullptr; auto d=q.front();q.pop_front();return d;}
 void Lock(){} void Unlock(){} ~DataQueue(){for(auto d:q)delete d;}
};
static int gMaxSends=10, gLogSocketFull=0, errorCode=0, allowance=100000, partial=0;
static std::string sent;
static int FakeSend(SOCKET, const char* p, int n, int) {
 if(allowance--<=0)return SOCKET_ERROR;
 if(partial>0){n=partial;partial=0;} sent.append(p,n); return n;
}
static int FakeClose(SOCKET){return 0;}
#define send FakeSend
#define closesocket FakeClose
#define WSAGetLastError() WSAEWOULDBLOCK
enum{VPNET_ERR_LINK_LOST=1,VPNET_ERR_WSOCK=2};
struct CTcpNet { struct CTCPLink {
 SOCKET m_socket=1; DataQueue m_queue; DWORD m_nextDgramToSend=0;
 void Log(const char*){} void SetError(int code, DWORD=0){errorCode=code;}
 void SendWaitingData();
};};
'''
prefix+=body(tcp,'static unsigned short calcsum(')+'\n'
prefix+=body(tcp,'void CTcpNet::CTCPLink::SendWaitingData()')+'\n'
tests=r'''
static void Add(CTcpNet::CTCPLink& link, const char* bytes, DWORD seq) {
 auto d=new Datagram((LPVOID)bytes,(WORD)strlen(bytes),FALSE);
 d->SetSeq(seq); d->SetSum(calcsum(bytes,(DWORD)strlen(bytes))); link.m_queue.Add(d);
}
int main(){int failures=0;
 { CTcpNet::CTCPLink link; sent.clear();allowance=100000;errorCode=0;
   for(int i=0;i<3000;++i)Add(link,"abc",i);
   link.SendWaitingData();
   printf("writable: remaining=%u bytes=%zu error=%d\n",link.m_queue.Count(),sent.size(),errorCode);
   failures+=(link.m_queue.Count()!=0||sent.size()!=9000||errorCode!=0); }
 { CTcpNet::CTCPLink link; sent.clear();allowance=15;errorCode=0;
   for(int i=0;i<30;++i)Add(link,"abc",i);
   link.SendWaitingData();
   failures+=(link.m_queue.Count()!=15||sent.size()!=45||errorCode!=0);
   allowance=100000;link.SendWaitingData();
   failures+=(link.m_queue.Count()!=0||sent.size()!=90||errorCode!=0); }
 { CTcpNet::CTCPLink link; sent.clear();allowance=100000;partial=2;errorCode=0;
   Add(link,"abc",0);Add(link,"xyz",1);
   link.SendWaitingData(); link.SendWaitingData();
   printf("partial: remaining=%u bytes=%s error=%d\n",link.m_queue.Count(),sent.c_str(),errorCode);
   failures+=(link.m_queue.Count()!=0||sent!="abcxyz"||errorCode!=0); }
 { CTcpNet::CTCPLink link; sent.clear();allowance=0;errorCode=0;
   Add(link,"abc",0);link.SendWaitingData();
   failures+=(link.m_queue.Count()!=1||!sent.empty()||errorCode!=0);
   allowance=100000;link.SendWaitingData();
   failures+=(link.m_queue.Count()!=0||sent!="abc"||errorCode!=0); }
 { CTcpNet::CTCPLink link; sent.clear();allowance=100000;errorCode=0;
   Add(link,"abc",0); link.m_queue.q.front()->m_data[0]='z'; link.SendWaitingData();
   failures+=(errorCode!=VPNET_ERR_LINK_LOST||!sent.empty()||link.m_socket!=INVALID_SOCKET); }
 { CTcpNet::CTCPLink link; sent.clear();allowance=100000;errorCode=0;
   Add(link,"abc",1); link.SendWaitingData();
   failures+=(errorCode!=VPNET_ERR_LINK_LOST||!sent.empty()||link.m_socket!=INVALID_SOCKET); }
 printf("tcp writes: %d failing scenarios\n",failures); return failures?1:0; }
'''
(out/'tcp-drain-repro.cpp').write_text(prefix+tests,encoding='utf-8')
(out/'tcp-drain-repro.cmd').write_text('@echo off\ncall "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\ncl /nologo /EHsc /std:c++17 tcp-drain-repro.cpp /Fe:tcp-drain-repro.exe user32.lib\nif errorlevel 1 exit /b 2\ntcp-drain-repro.exe\n')

cmd = out/'tcp-drain-repro.cmd'
cmd.write_text(cmd.read_text().replace('cl /nologo', 'cl /nologo /' + args.opt))
subprocess.run(['cmd','/d','/c',str(cmd)],cwd=out,check=True)
