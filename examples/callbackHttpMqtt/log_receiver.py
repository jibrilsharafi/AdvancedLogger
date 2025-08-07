#!/usr/bin/env python3
"""
Simple HTTP server to receive log messages from AdvancedLogger ESP32 library.

This script creates a lightweight HTTP server that:
- Receives POST requests with JSON log data
- Pretty prints the log messages to console
- Optionally saves logs to a file
- Provides basic statistics

Author: GitHub Copilot
Usage: python log_receiver.py [--port PORT] [--save-to-file] [--file FILENAME]
"""

import json
import argparse
from datetime import datetime
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse
import signal
import sys

class LogReceiver(BaseHTTPRequestHandler):
    log_count = 0
    start_time = datetime.now()
    save_to_file = False
    log_filename = "received_logs.txt"
    
    def do_POST(self):
        """Handle POST requests with log data"""
        try:
            # Parse the request path
            path = urlparse(self.path).path
            
            if path == '/test':
                # Read the request body
                content_length = int(self.headers.get('Content-Length', 0))
                if content_length == 0:
                    self.send_error(400, "Empty request body")
                    return
                
                post_data = self.rfile.read(content_length)
                
                # Parse JSON
                try:
                    log_data = json.loads(post_data.decode('utf-8'))
                except json.JSONDecodeError as e:
                    self.send_error(400, f"Invalid JSON: {e}")
                    return
                
                # Process the log entry
                self._process_log_entry(log_data)
                
                # Send success response
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                response = {"status": "success", "message": "Log received"}
                self.wfile.write(json.dumps(response).encode())
                
            else:
                self.send_error(404, "Endpoint not found")
                
        except Exception as e:
            print(f"Error processing request: {e}")
            self.send_error(500, f"Internal server error: {e}")
    
    def do_GET(self):
        """Handle GET requests for status and stats"""
        if self.path == '/status':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            
            uptime = datetime.now() - self.start_time
            stats = {
                "status": "running",
                "logs_received": self.log_count,
                "uptime_seconds": int(uptime.total_seconds()),
                "start_time": self.start_time.isoformat(),
                "saving_to_file": self.save_to_file,
                "log_filename": self.log_filename if self.save_to_file else None
            }
            self.wfile.write(json.dumps(stats, indent=2).encode())
        else:
            self.send_error(404, "Endpoint not found. Try /status for server info")
    
    def _process_log_entry(self, log_data):
        """Process and display a log entry"""
        LogReceiver.log_count += 1
        
        # Extract log fields
        timestamp = log_data.get('timestamp', 'N/A')
        millis = log_data.get('millis', 'N/A')
        level = log_data.get('level', 'N/A')
        core = log_data.get('core', 'N/A')
        file = log_data.get('file', 'N/A')
        function = log_data.get('function', 'N/A')
        line = log_data.get('line', 'N/A')
        message = log_data.get('message', 'N/A')
        
        # Color coding for different log levels
        colors = {
            'VERBOSE': '\033[37m',  # White
            'DEBUG': '\033[36m',    # Cyan
            'INFO': '\033[32m',     # Green
            'WARNING': '\033[33m',  # Yellow
            'ERROR': '\033[31m',    # Red
            'FATAL': '\033[35m'     # Magenta
        }
        reset_color = '\033[0m'
        color = colors.get(level, '')
        
        # Format the log entry for console display
        if 'line' in log_data:
            location = f"{file}:{function}:{line}"
        else:
            location = f"{file}:{function}"
            
        console_output = (
            f"{color}[{self.log_count:04d}] "
            f"[{timestamp}] [{millis} ms] [{level:8s}] [Core {core}] "
            f"[{location}] {message}{reset_color}"
        )
        
        print(console_output)
        
        # Save to file if enabled
        if self.save_to_file:
            file_output = (
                f"[{self.log_count:04d}] "
                f"[{timestamp}] [{millis} ms] [{level:8s}] [Core {core}] "
                f"[{location}] {message}\n"
            )
            try:
                with open(self.log_filename, 'a', encoding='utf-8') as f:
                    f.write(file_output)
            except Exception as e:
                print(f"Error writing to file: {e}")
    
    def log_message(self, format, *args):
        """Override to customize server log messages"""
        # Only log errors and important messages, not every request
        if "POST /test" not in format % args:
            super().log_message(format, *args)

def signal_handler(sig, frame):
    """Handle Ctrl+C gracefully"""
    print(f"\n\nShutting down log receiver...")
    print(f"Total logs received: {LogReceiver.log_count}")
    if LogReceiver.save_to_file:
        print(f"Logs saved to: {LogReceiver.log_filename}")
    sys.exit(0)

def main():
    parser = argparse.ArgumentParser(description='HTTP Log Receiver for AdvancedLogger')
    parser.add_argument('--port', '-p', type=int, default=8080, 
                       help='Port to listen on (default: 8080)')
    parser.add_argument('--save-to-file', '-s', action='store_true',
                       help='Save received logs to a file')
    parser.add_argument('--file', '-f', type=str, default='received_logs.txt',
                       help='Filename to save logs to (default: received_logs.txt)')
    parser.add_argument('--host', type=str, default='0.0.0.0',
                       help='Host to bind to (default: 0.0.0.0 - all interfaces)')
    
    args = parser.parse_args()
    
    # Configure the log receiver
    LogReceiver.save_to_file = args.save_to_file
    LogReceiver.log_filename = args.file
    
    # Set up signal handler for graceful shutdown
    signal.signal(signal.SIGINT, signal_handler)
    
    # Create and start the server
    server_address = (args.host, args.port)
    httpd = HTTPServer(server_address, LogReceiver)
    
    print("=" * 80)
    print("AdvancedLogger HTTP Receiver")
    print("=" * 80)
    print(f"Server running on http://{args.host}:{args.port}")
    print(f"Log endpoint: http://{args.host}:{args.port}/test")
    print(f"Status endpoint: http://{args.host}:{args.port}/status")
    print(f"Save to file: {'Yes' if args.save_to_file else 'No'}")
    if args.save_to_file:
        print(f"Log file: {args.file}")
    print("=" * 80)
    print("Waiting for log messages... (Press Ctrl+C to stop)")
    print()
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        signal_handler(None, None)

if __name__ == '__main__':
    main()
