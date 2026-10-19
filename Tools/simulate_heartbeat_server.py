from http.server import BaseHTTPRequestHandler, HTTPServer
from random import randint
import json

class GetHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        bpm = randint(50, 200)
        data = {"bpm": bpm}
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode('utf-8'))

HTTPServer(('localhost', 8000), GetHandler).serve_forever()
