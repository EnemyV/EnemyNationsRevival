import hashlib,json,os,signal,subprocess,sys,tempfile
from pathlib import Path
fake_source="#!/usr/bin/env python3\nimport os,socket,sys,pwd\nassert os.environ['HOME'] == pwd.getpwuid(os.getuid()).pw_dir\nassert os.environ['EN_REGISTRY_DIR'] == os.path.join(os.environ['EN_LOG_DIR'], '.config', 'enations')\nassert os.path.isfile(os.path.join(os.environ['EN_REGISTRY_DIR'], 'registry.ini'))\nmode=os.path.basename(sys.argv[0]); s=socket.socket(); s.bind(('127.0.0.1',0)); s.listen(4)\np=os.environ['EN_P1_PORT_FILE']; open(p+'.tmp','w').write(str(s.getsockname()[1])+'\\n'); os.replace(p+'.tmp',p)\ncount=0; occ_count=0\nwhile True:\n c,_=s.accept(); command=c.recv(512).decode().strip().split(' ',1)[1]\n if command=='READY': reply='OK protocol=1 fixture=fixture\\n'\n elif command=='LOAD fixture': reply='OK loaded\\n'\n elif command.startswith('CAMERA '): reply='OK camera_x=10 camera_y=20\\n'\n elif command.startswith('SPEED '): reply='OK speed=8\\n'\n elif command=='STATUS':\n  count+=1; reply=('OK ingame=1 camera_available=1 camera_x=10 camera_y=20 speed=8 players=1 buildings=1 vehicles=1 game_frame=%d game_seconds=%d assert_fires=1\\n' % (count,count))\n elif command=='OCCUPANCY':\n  occ_count+=1\n  reply=('WAIT occupancy_pending\\n' if (mode=='pending' or (mode=='pending_then_capture' and occ_count==1)) else 'WAIT occupancy_pending EXTRA\\n' if mode=='malformed' else 'OCC_MISMATCH_V1 address=42\\nOCC_HISTORY_V4 count=1 overwritten=0 rejected=0 snapshot_busy=0 sequence_exhausted=0 matching_omitted=0\\nOCC seq=1\\nOCC_HISTORY_END count=1\\n')\n elif command=='MARK BEGIN': reply='OK mark=begin monotonic_ns=1 game_frame=1 game_seconds=1 speed=8\\n'\n elif command=='MARK END': reply='OK mark=end monotonic_ns=2 game_frame=2 game_seconds=2 speed=8\\n'\n elif command=='QUIT': c.sendall(b'OK quitting\\n'); c.close(); break\n else: reply='ERR unexpected\\n'\n if command=='STATUS':\n  if mode=='missing': reply=reply.replace(' assert_fires=1','')\n  elif mode=='duplicate': reply=reply.replace(' assert_fires=1',' assert_fires=1 assert_fires=0')\n  elif mode=='invalid': reply=reply.replace(' assert_fires=1',' assert_fires=-1')\n  elif mode=='zero' or (mode=='warmup' and count<3): reply=reply.replace(' assert_fires=1',' assert_fires=0')\n c.sendall(reply.encode()); c.close()\n"
fake_source = '\n'.join(line for line in fake_source.splitlines() if not line.startswith('assert os.environ') and not line.startswith('assert os.path.isfile')) + '\n'
fake_source = fake_source.replace('count=0; occ_count=0', 'count=0; occ_count=0; ended=False')
fake_source = fake_source.replace("elif command=='MARK END': reply=", "elif command=='MARK END': ended=True; reply=")
fake_source = fake_source.replace("if mode=='missing':", "if mode=='after_end' and not ended: reply=reply.replace(' assert_fires=1',' assert_fires=0')\n  elif mode=='missing':")
fake_source = fake_source.replace("elif command=='QUIT': c.sendall(b'OK quitting\\n'); c.close(); break", "elif command=='QUIT':\n  if mode=='quit_rejected': reply='ERR quit\\n'\n  else: c.sendall(b'OK quitting\\n'); c.close(); break")
runner=Path(__file__).with_name('run_fixture.py')
with tempfile.TemporaryDirectory(prefix='assert-control-',dir=Path(__file__).parent) as td:
 root=Path(td); fixture=root/'fixture.en'; fixture.write_bytes(b'fixture'); digest=hashlib.sha256(fixture.read_bytes()).hexdigest()
 for mode in ('missing','duplicate','invalid','zero','nonzero','warmup','after_end','quit_rejected'):
  game=root/mode; game.write_text(fake_source); game.chmod(0o700); out=root/(mode+'-out')
  cmd=[sys.executable,'-I','-B',str(runner),'--executable',str(game),'--run-dir',str(root),'--fixture',str(fixture),'--fixture-sha256',digest,'--output',str(out),'--seconds','1','--warmup',('1' if mode=='warmup' else '0'),'--dummy-sdl','--camera-x','10','--camera-y','20','--speed','8']
  p=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,start_new_session=True)
  try: stdout,stderr=p.communicate(timeout=30)
  except subprocess.TimeoutExpired: os.killpg(p.pid,signal.SIGKILL); p.communicate(); raise
  assert p.returncode==1,(mode,p.returncode,stdout,stderr)
  result=json.loads((out/'result.json').read_text())
  if mode in ('missing','duplicate','invalid'): assert result.get('error')=='missing, duplicate or invalid assert_fires',result
  elif mode=='zero': assert result['qualification_checks']['no_reported_invariant_fires'],result
  else:
   assert result.get('error')=='invariant fires observed',result
   assert result['status']=='incomplete',result
   if mode=='quit_rejected': assert result.get('abort_quit_response')=='ERR quit' and result.get('abort_quit_error')=='quit rejected',result
   else: assert result.get('abort_quit_response')=='OK quitting' and result.get('abort_exit_code')==0,result
   if mode=='warmup': assert 'mark_begin' not in result,result
   if mode=='after_end': assert 'mark_end' in result,result
  print(mode,'PASS')
