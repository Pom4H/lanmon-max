#!/usr/bin/env python3
"""Local HTTP model of the MAX endpoints exercised by the C++98 E2E harness."""

import json
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

# The client must send this value only in the Authorization header.
TOKEN = "e2e-secret-token"

# Cross-request state is used to prove Long Poll marker continuity and capture sends.
state = {"poll": 0, "received": []}


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

    # MAX API authentication contract used by the adapter.
    def authorized(self):
        return self.headers.get("Authorization") == TOKEN

    def do_GET(self):
        # Every API read in this E2E requires the bot token.
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

        if not self.authorized():
            return self.reply(401, {"code": "unauthorized"})

        # This server endpoint validates JSON text sends; upload flow has separate unit coverage.
        try:
            body = json.loads(raw.decode("utf-8"))
        except Exception as e:
            return self.reply(400, {"code": "bad_json", "message": str(e)})

        if p.path != "/messages":
            return self.reply(404, {"code": "not_found"})

        # Dedicated address exercises non-2xx error propagation in the client.
        if q.get("user_id") == ["401"]:
            return self.reply(401, {"code": "unauthorized_test"})

        # Capture request so a developer can inspect query/body/auth in /_state or CI logs.
        state["received"].append({
            "query": q,
            "body": body,
            "authorization": self.headers.get("Authorization"),
        })

        # Reply sent to chat 777 must survive UTF-8 and JSON escaping exactly.
        if q.get("chat_id") == ["777"]:
            expected = "PONG \"LanMon\"\nПривет из E2E"
            if body.get("text") != expected:
                return self.reply(422, {
                    "code": "unexpected_body",
                    "got": body.get("text"),
                    "expected": expected,
                })

        return self.reply(200, {"message": {"body": {"mid": "sent-e2e"}}})


if __name__ == "__main__":
    # run_e2e.sh may override the port to avoid collisions on a CI worker.
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 18080
    print(f"Mock MAX listening on 127.0.0.1:{port}", flush=True)
    ThreadingHTTPServer(("127.0.0.1", port), Handler).serve_forever()
