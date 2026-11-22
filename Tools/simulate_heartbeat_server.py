import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from random import gauss


class GetHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        bpm = round(max(40, min(180, gauss(75, 15))))
        data = json.dumps({"bpm": bpm}).encode()

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


HTTPServer(("localhost", 8000), GetHandler).serve_forever()
