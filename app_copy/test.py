#!/usr/bin/env python3

import time
import os

# time.sleep(6)  # sleep 6 seconds (longer than your 4s timeout)

print("Status: 200 OK")
print("Content-Type: text/html")
print()

print("<html>")
print("<head><title>Slow CGI</title></head>")
print("<body>")
print("<h1>Response after 19 seconds</h1>")
print("</body>")
question = os.environ.get("QUERY_STRING", "")
print(f"<p>Query string: {question}</p>")
print("</html>")
