#!/usr/bin/env python3
"""Local HTTP model of the MAX endpoints exercised by the C++98 E2E harness."""

import json
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

# The client must send this value only to Bot API endpoints.
TOKEN = "e2e-secret-token"

# Cross-request state proves Long Poll continuity and records outgoing operations.
state = {"poll": 0, "received": [], "uploads": []}


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    # Keep server diagnostics in the CI log with an obvious prefix.
    def log_message(self, fmt, *args):
        sys.stdout.write("MOCK " + (fmt % args) + "\n")
        sys.stdout.flush()

    # Return compact UTF-8 JSON, matching the shape expected from MAX.
    def reply(self, code, payload):
        data = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    # MAX Bot API authentication contract used by the adapter.
    def authorized(self):
        return self.headers.get("Authorization") == TOKEN

    def do_GET(self):
        # Every Bot API read in this E2E requires the bot token.
        if not self.authorized():
            return self.reply(401, {"code": "unauthorized"})

        p = urlparse(self.path)

        # GET /me returns deterministic bot identity.
        if p.path == "/me":
            return self.reply(200, {
                "user_id": 9001,
                "first_name": "LanMon E2E",
                "username": "lanmon_e2e_bot",
                "is_bot": True,
            })

        # GET /updates models two consecutive Long Poll reads.
        if p.path == "/updates":
            q = parse_qs(p.query)
            if state["poll"] == 0:
                # First read must not invent a marker.
                assert "marker" not in q, q
                state["poll"] += 1
                return self.reply(200, {
                    "updates": [{
                        "update_type": "message_created",
                        "timestamp": 1720000000123,
                        "message": {
                            "sender": {
                                "user_id": 42,
                                "first_name": "Ivan",
                                "username": "ivan",
                                "is_bot": False,
                            },
                            "recipient": {
                                "chat_id": 777,
                                "chat_type": "dialog",
                                "user_id": 9001,
                            },
                            "timestamp": 1720000000000,
                            "body": {
                                "mid": "m-e2e-1",
                                "seq": 1,
                                "text": "PING \"MAX\"\nПривет",
                            },
                        },
                    }],
                    "marker": 101,
                })

            # Second read proves that MAX_API_CLIENT persisted marker=101.
            assert q.get("marker") == ["101"], q
            state["poll"] += 1
            return self.reply(200, {"updates": [], "marker": 102})

        # Diagnostic endpoint is useful when debugging the local E2E manually.
        if p.path == "/_state":
            return self.reply(200, state)

        return self.reply(404, {"code": "not_found"})

    def do_POST(self):
        p = urlparse(self.path)
        q = parse_qs(p.query)
        n = int(self.headers.get("Content-Length", "0"))
        raw = self.rfile.read(n)

        # Upload-host requests are intentionally separate from Bot API auth.
        if p.path in ("/upload/image", "/upload/file"):
            # Multipart mode from MAX docs does not require leaking bot Authorization
            # to the returned upload host. Fail loudly if the client does it.
            if self.headers.get("Authorization") is not None:
                return self.reply(400, {"code": "bot_token_leaked_to_upload_host"})
            content_type = self.headers.get("Content-Type", "")
            if "multipart/form-data" not in content_type:
                return self.reply(415, {"code": "expected_multipart"})
            if b'name="data"' not in raw:
                return self.reply(422, {"code": "missing_data_field"})
            kind = p.path.rsplit("/", 1)[-1]
            state["uploads"].append({
                "kind": kind,
                "query": q,
                "size": len(raw),
                "authorization": self.headers.get("Authorization"),
            })
            token = "image-e2e-token" if kind == "image" else "file-e2e-token"
            return self.reply(200, {"token": token})

        # Everything below is a Bot API endpoint and requires Authorization.
        if not self.authorized():
            return self.reply(401, {"code": "unauthorized"})

        # POST /uploads returns a one-use URL on a separate upload host.
        if p.path == "/uploads":
            kind = q.get("type", [""])[0]
            if kind not in ("image", "file"):
                return self.reply(400, {"code": "unsupported_upload_type", "type": kind})
            port = self.server.server_address[1]
            return self.reply(200, {
                "url": f"http://127.0.0.1:{port}/upload/{kind}?ticket={kind}-e2e-1"
            })

        if p.path != "/messages":
            return self.reply(404, {"code": "not_found"})

        # Dedicated address exercises non-2xx error propagation in the client.
        if q.get("user_id") == ["401"]:
            return self.reply(401, {"code": "unauthorized_test"})

        # Message endpoints require JSON.
        try:
            body = json.loads(raw.decode("utf-8"))
        except Exception as e:
            return self.reply(400, {"code": "bad_json", "message": str(e)})

        state["received"].append({
            "query": q,
            "body": body,
            "authorization": self.headers.get("Authorization"),
        })

        # Text reply to chat 777 must survive UTF-8 and JSON escaping exactly.
        if q.get("chat_id") == ["777"]:
            expected = "PONG \"LanMon\"\nПривет из E2E"
            if body.get("text") != expected:
                return self.reply(422, {
                    "code": "unexpected_body",
                    "got": body.get("text"),
                    "expected": expected,
                })

        # Image attachment is routed to a signed group chat id.
        if q.get("chat_id") == ["-777"]:
            attachments = body.get("attachments") or []
            if body.get("text") != "Карта \"1\"":
                return self.reply(422, {"code": "unexpected_image_caption"})
            if len(attachments) != 1:
                return self.reply(422, {"code": "unexpected_image_attachments"})
            if attachments[0].get("type") != "image":
                return self.reply(422, {"code": "unexpected_image_type"})
            if (attachments[0].get("payload") or {}).get("token") != "image-e2e-token":
                return self.reply(422, {"code": "unexpected_image_token"})

        # File attachment is routed directly to user 42.
        if q.get("user_id") == ["42"] and body.get("attachments"):
            attachments = body.get("attachments") or []
            if body.get("text") != "Тревоги E2E":
                return self.reply(422, {"code": "unexpected_file_caption"})
            if len(attachments) != 1:
                return self.reply(422, {"code": "unexpected_file_attachments"})
            if attachments[0].get("type") != "file":
                return self.reply(422, {"code": "unexpected_file_type"})
            if (attachments[0].get("payload") or {}).get("token") != "file-e2e-token":
                return self.reply(422, {"code": "unexpected_file_token"})

        return self.reply(200, {"message": {"body": {"mid": "sent-e2e"}}})


if __name__ == "__main__":
    # run_e2e.sh may override the port to avoid collisions on a CI worker.
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 18080
    print(f"Mock MAX listening on 127.0.0.1:{port}", flush=True)
    ThreadingHTTPServer(("127.0.0.1", port), Handler).serve_forever()
