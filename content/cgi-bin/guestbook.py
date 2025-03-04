#!/usr/bin/python3

import os
import time
import sys
from urllib.parse import parse_qs

# Get the absolute path to the content directory
content_dir = os.path.dirname(os.path.abspath(__file__))  # This gets the directory where guestbook.py is located (content/cgi-bin)
content_root = os.path.join(content_dir, '../')  # Get the parent directory, which is 'content'

guestbook_file = os.path.join(content_root, 'guestbook.txt')  # Path to guestbook.txt in the content folder
template_file = os.path.join(content_root, 'cgi-guestbook.html')  # Path to guestbook_template.html in the content folder

# Set the header for HTML content
print("Content-Type: text/html\n\n")

# Function to read the guestbook messages
def read_guestbook():
    if os.path.exists(guestbook_file):
        with open(guestbook_file, 'r') as file:
            return file.read()
    else:
        return "No messages yet."

# Function to append a message to the guestbook
def append_message(name, message):
    with open(guestbook_file, 'a') as file:
        file.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - <b>{name}</b>: {message}<br>\n")

# Read POST data directly (handling form submission manually)
def get_post_data():
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length:
        post_data = sys.stdin.read(content_length)
        return parse_qs(post_data)
    return {}

# Process form data if available
form_data = get_post_data()

if 'name' in form_data and 'message' in form_data:
    name = form_data['name'][0]
    message = form_data['message'][0]
    append_message(name, message)

# Read and display the guestbook messages
guestbook_content = read_guestbook()

# Read the HTML template
with open(template_file, "r") as html_file:
    template = html_file.read()

# Replace the placeholder in the template with the guestbook content
output = template.replace("{{guestbook_messages}}", guestbook_content)

print(output)
