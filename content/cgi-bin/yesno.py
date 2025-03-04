#!/usr/bin/env python3

import requests
import cgi
import html

# Read POST data
form = cgi.FieldStorage()
user_question = form.getvalue("question", "").strip()

# Sanitize user input to prevent HTML injection
user_question = html.escape(user_question)

# If no question was provided, ask for one
if not user_question:
    print("Content-type: text/html\n")
    print("<h2>Error: Please provide a question.</h2>")
    exit()

# Fetch Yes/No API response
API_URL = "https://yesno.wtf/api"
try:
    response = requests.get(API_URL, timeout=5)
    response.raise_for_status()  # Raise an error for bad responses (4xx, 5xx)
    data = response.json()
    
    answer = data.get("answer", "Unknown").upper()  # "YES" or "NO"
    gif_url = data.get("image", "")
    
except requests.RequestException as e:
    answer = "ERROR"
    gif_url = ""
    error_message = f"Failed to fetch response: {e}"

# Print CGI response headers
print("Content-type: text/html\n")

# HTML response
print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>YesNo Answer</title>
    <style>
        body {{ font-family: Arial, sans-serif; text-align: center; padding: 20px; }}
        textarea {{ width: 80%; height: 100px; font-size: 16px; padding: 10px; }}
        .gif-container {{
            position: relative;
            display: inline-block;
        }}
        .answer-overlay {{
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            font-size: 50px;
            font-weight: bold;
            color: white;
            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.7);
            opacity: 0;
            animation: fadeIn 2s forwards 1s;  /* Delay the appearance of the text */
        }}
        img {{ max-width: 700px; height: auto; }}
        .error {{ color: red; }}
        
        @keyframes fadeIn {{
            to {{
                opacity: 1;
            }}
        }}
    </style>
</head>
<body>
    <h2>Your Question:</h2>
    <p><strong>{user_question}</strong></p>

    <div class="gif-container">
        {"<img src='" + gif_url + "' width='900' alt='Yes/No GIF'>" if gif_url else "<p class='error'>Failed to load GIF.</p>"}
        <span class="answer-overlay">{answer}</span>  <!-- YES/NO Overlayed on GIF -->
    </div>

    <br><br>
    <button onclick="window.location.href='/cgi_yesno.html'">Ask Another Question</button>
</body>
</html>
""")
