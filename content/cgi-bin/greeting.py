import sys
import os
import urllib.parse

# Set content type
print("Content-type: text/html\n")

# Parse GET and POST data
query_string = os.environ.get("QUERY_STRING", "")
parsed_query = urllib.parse.parse_qs(query_string)

content_length = int(os.environ.get("CONTENT_LENGTH", 0))
post_data = sys.stdin.read(content_length) if content_length > 0 else ""
parsed_post = urllib.parse.parse_qs(post_data)

# Get values with default fallbacks
name = (parsed_query.get("name") or parsed_post.get("name") or ["Guest"])[0]
color = (parsed_query.get("color") or parsed_post.get("color") or ["black"])[0]

# Print the HTML response
print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Welcome</title>
</head>
<body>
    <h1 style="text-align: center; color: {color};">
        Welcome {name}!
    </h1>
</body>
</html>
""")