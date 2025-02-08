import os

print("=== CGI Environment Variables ===\n")

for key, value in sorted(os.environ.items()):
    print(f"{key}={value}")