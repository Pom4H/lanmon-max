#!/usr/bin/env python3
import json,sys
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from urllib.parse import urlparse,parse_qs
TOKEN="lanmon-e2e-token";PORT=int(sys.argv[1]) if len(sys.argv)>1 else 18081
COMMANDS=["STOP","MAP 2","SCREEN 1","SCREEN","LOG","LOGXLS","ALARM","HELP"]
EXPECTED=["text","image","image","image","file","file","file","text"]
state={"poll":0,"messages":[],"uploads":[],"prepare":0}
class H(BaseHTTPRequestHandler):
 protocol_version="HTTP/1.1"
 def log_message(self,fmt,*args):print("MOCK",fmt%args,flush=True)
 def js(self,code,p):
  d=json.dumps(p,ensure_ascii=False,separators=(",",":")).encode();self.send_response(code);self.send_header("Content-Type","application/json");self.send_header("Content-Length",str(len(d)));self.end_headers();self.wfile.write(d)
 def auth(self):return self.headers.get("Authorization")==TOKEN
 def do_GET(self):
  p=urlparse(self.path);q=parse_qs(p.query)
  if p.path=="/_state":return self.js(200,state)
  if not self.auth():return self.js(401,{"code":"unauthorized"})
  if p.path!="/updates":return self.js(404,{"code":"not_found"})
  i=state["poll"];expected=None if i==0 else str(200+i)
  if expected is None and "marker" in q:return self.js(422,{"code":"unexpected_marker"})
  if expected is not None and q.get("marker")!=[expected]:return self.js(422,{"code":"bad_marker","expected":expected,"query":q})
  if i>=len(COMMANDS):return self.js(200,{"updates":[],"marker":208})
  marker=201+i;state["poll"]+=1
  return self.js(200,{"updates":[{"update_type":"message_created","timestamp":1000+i,"message":{"sender":{"user_id":42,"first_name":"Operator","is_bot":False},"recipient":{"chat_id":777,"chat_type":"dialog"},"timestamp":1000+i,"body":{"mid":"cmd-%d"%i,"text":COMMANDS[i]}}}],"marker":marker})
 def do_POST(self):
  p=urlparse(self.path);q=parse_qs(p.query);n=int(self.headers.get("Content-Length","0"));raw=self.rfile.read(n)
  if not self.auth() and not p.path.startswith("/upload/"):return self.js(401,{"code":"unauthorized"})
  if p.path=="/uploads":
   typ=q.get("type",[""])[0];state["prepare"]+=1;ident=state["prepare"];return self.js(200,{"url":f"http://127.0.0.1:{PORT}/upload/{typ}/{ident}"})
  if p.path.startswith("/upload/"):
   typ=p.path.split("/")[2];state["uploads"].append({"type":typ,"bytes":len(raw)});return self.js(200,{"token":"tok-%s-%d"%(typ,len(state["uploads"]))})
  if p.path=="/messages":
   try:b=json.loads(raw.decode())
   except Exception as e:return self.js(400,{"code":"bad_json","e":str(e)})
   state["messages"].append(b);idx=len(state["messages"])-1
   if idx<8:
    if q.get("chat_id")!=["777"]:return self.js(422,{"code":"bad_chat","q":q})
    exp=EXPECTED[idx];att=b.get("attachments") or [];got="text" if not att else att[0].get("type")
    if got!=exp:return self.js(422,{"code":"bad_type","index":idx,"expected":exp,"got":got,"body":b})
   else:
    if q.get("user_id")!=["778"] or b.get("text")!="ALARM BROADCAST":return self.js(422,{"code":"bad_alarm","q":q,"b":b})
   return self.js(200,{"message":{"body":{"mid":"reply-%d"%(idx+1)}}})
  return self.js(404,{"code":"not_found"})
print(f"LanMon MAX mock listening on 127.0.0.1:{PORT}",flush=True);ThreadingHTTPServer(("127.0.0.1",PORT),H).serve_forever()
