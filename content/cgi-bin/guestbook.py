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
            content = file.read().strip()
            if content:
                return content  # Return the content of the guestbook if there are messages
            else:
                return None  # Return None if the file is empty
    else:
        return None  # Return None if the file doesn't exist

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

# Show the form page and handle the submission
if 'name' in form_data and 'message' in form_data:
    name = form_data['name'][0]
    message = form_data['message'][0]
    append_message(name, message)

# Read guestbook messages after form submission
guestbook_content = read_guestbook()

# Read the template (which contains only the form)
with open(template_file, "r") as html_file:
    template = html_file.read()

# Add the messages below the form if available
if guestbook_content:
    guestbook_html = f"<h3>Messages:</h3><div>{guestbook_content}</div>"
    # Replace the placeholder with guestbook messages
    template += guestbook_html  # Append the messages below the form

# Output the final HTML page with form and messages (if any)
print(template)
