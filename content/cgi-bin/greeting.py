import cgi

# Set content type
print("Content-type: text/html\n")

# Get form data
form = cgi.FieldStorage()
name = form.getvalue("name", "Guest")
color = form.getvalue("color", "black")

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