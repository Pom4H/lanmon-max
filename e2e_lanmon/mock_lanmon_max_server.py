#!/usr/bin/env python3
import json
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

TOKEN="lanmon-e2e-token"
PORT=int(sys.argv[1]) if len(sys.argv)>1 else 18081
state={"poll":0,"messages":[],"uploads":[]}

COMMANDS=[
    ("STOP",201,"cmd-stop"),
    ("MAP 2",202,"cmd-map"),
    ("HELP",203,"cmd-help"),
]

class Handler(BaseHTTPRequestHandler):
    protocol_version="HTTP/1.1"
    def log_message(self,fmt,*args):
        print("MOCK",fmt%args,flush=True)
    def send_json(self,code,payload):
        data=json.dumps(payload,ensure_ascii=False,separators=(",",":")).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type","application/json; charset=utf-8")
        self.send_header("Content-Length",str(len(data)))
        self.end_headers(); self.wfile.write(data)
    def auth(self):
        return self.headers.get("Authorization")==TOKEN
    def do_GET(self):
        p=urlparse(self.path); q=parse_qs(p.query)
        if p.path=="/_state": return self.send_json(200,state)
        if not self.auth(): return self.send_json(401,{"code":"unauthorized"})
        if p.path=="/updates":
            i=state["poll"]
            expected_marker=None if i==0 else str(COMMANDS[i-1][1]) if i<=len(COMMANDS) else str(COMMANDS[-1][1])
            if expected_marker is None:
                if "marker" in q: return self.send_json(422,{"code":"unexpected_marker","query":q})
            else:
                if q.get("marker") != [expected_marker]: return self.send_json(422,{"code":"bad_marker","query":q,"expected":expected_marker})
            if i < len(COMMANDS):
                text,marker,mid=COMMANDS[i]
                state["poll"]+=1
                return self.send_json(200,{"updates":[{"update_type":"message_created","timestamp":1720000000000+i,
                    "message":{"sender":{"user_id":42,"first_name":"Operator","is_bot":False},
                    "recipient":{"chat_id":777,"chat_type":"dialog","user_id":9001},
                    "timestamp":1720000000000+i,"body":{"mid":mid,"text":text}}}],"marker":marker})
            return self.send_json(200,{"updates":[],"marker":COMMANDS[-1][1]})
        return self.send_json(404,{"code":"not_found"})
    def do_POST(self):
        p=urlparse(self.path); q=parse_qs(p.query)
        if not self.auth(): return self.send_json(401,{"code":"unauthorized"})
        length=int(self.headers.get("Content-Length","0")); raw=self.rfile.read(length)
        if p.path=="/uploads":
            if q.get("type") != ["image"]: return self.send_json(422,{"code":"wrong_upload_type","query":q})
            return self.send_json(200,{"url":f"http://127.0.0.1:{PORT}/upload/image/map-token"})
        if p.path=="/upload/image/map-token":
            ctype=self.headers.get("Content-Type","")
            if "multipart/form-data" not in ctype or b'name="data"' not in raw:
                return self.send_json(422,{"code":"bad_multipart"})
            if b"LANMON_FAKE_PNG" not in raw:
                return self.send_json(422,{"code":"wrong_file"})
            state["uploads"].append({"bytes":len(raw),"content_type":ctype})
            return self.send_json(200,{"token":"uploaded-map-token"})
        if p.path=="/messages":
            try: body=json.loads(raw.decode("utf-8"))
            except Exception as e: return self.send_json(400,{"code":"bad_json","message":str(e)})
            if q.get("chat_id") != ["777"]: return self.send_json(422,{"code":"wrong_peer","query":q})
            state["messages"].append(body)
            n=len(state["messages"])
            if n==1 and body.get("text")!="Команда СТОП выполнена":
                return self.send_json(422,{"code":"bad_stop_reply","body":body})
            if n==2:
                a=body.get("attachments") or []
                if body.get("text")!="Карта 2" or len(a)!=1 or a[0].get("type")!="image" or a[0].get("payload",{}).get("token")!="uploaded-map-token":
                    return self.send_json(422,{"code":"bad_map_reply","body":body})
            if n==3 and "MAP x - карта номер x" not in body.get("text",""):
                return self.send_json(422,{"code":"bad_help_reply","body":body})
            return self.send_json(200,{"message":{"body":{"mid":f"reply-{n}"}}})
        return self.send_json(404,{"code":"not_found"})

print(f"LanMon MAX mock listening on 127.0.0.1:{PORT}",flush=True)
ThreadingHTTPServer(("127.0.0.1",PORT),Handler).serve_forever()
