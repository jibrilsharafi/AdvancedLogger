#!/usr/bin/env python3
"""
Minimal HTTP server to receive and print log messages.
"""

import json
from http.server import HTTPServer, BaseHTTPRequestHandler

class LogReceiver(BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length)
        
        try:
            log_data = json.loads(post_data.decode('utf-8'))
            print(json.dumps(log_data))
            
            self.send_response(200)
            self.end_headers()
        except:
            self.send_response(400)
            self.end_headers()
    
    def log_message(self, format, *args):
        pass  # Suppress server logs

if __name__ == '__main__':
    server = HTTPServer(('0.0.0.0', 8080), LogReceiver)
    print("Server running on port 8080")
    server.serve_forever()
