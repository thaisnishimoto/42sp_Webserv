<?php

// Define file paths
$content_dir = dirname(__FILE__);
$content_root = realpath($content_dir . '/../');
$guestbook_file = $content_root . '/guestbook.txt';

// Function to read the guestbook messages
function read_guestbook() {
    global $guestbook_file;
    if (file_exists($guestbook_file)) {
        $content = file_get_contents($guestbook_file);
        if (trim($content)) {
            return $content; // Return guestbook content if there are messages
        }
    }
    return null; // Return null if empty or non-existent
}

// Function to append a message to the guestbook
function append_message($name, $message) {
    global $guestbook_file;
    $timestamp = date('Y-m-d H:i:s');
    $entry = "$timestamp - <b>" . htmlspecialchars($name) . "</b>: " . htmlspecialchars($message) . "<br>\n";
    file_put_contents($guestbook_file, $entry, FILE_APPEND);
}

// Read input from STDIN (for CGI scripts)
$input = stream_get_contents(STDIN);
parse_str($input, $form_data);

// Process form data if available
if (!empty($form_data['name']) && !empty($form_data['message'])) {
    $name = trim($form_data['name']);
    $message = trim($form_data['message']);
    append_message($name, $message);
}

// Read guestbook messages after form submission
$guestbook_content = read_guestbook();

$template = <<<HTML
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Guestbook</title>
    <link rel="stylesheet" href="../assets/styles.css">
</head>
<body>
    <header>
        <h2>Guestbook</h2>
    </header>
    <br><br>
    <form method="POST" action="/cgi-bin/guestbook.php">
        <label for="name">Name:</label>
        <input type="text" id="name" name="name" required>
        <br><br>
        <label for="message">Message:</label>
        <textarea id="message" name="message" required></textarea>
        <br><br>
        <button type="submit">Submit</button>
    </form>
</body>
</html>
HTML;

// Append messages to the template
if ($guestbook_content) {
    $guestbook_html = "<br> 
    <h3>Messages:</h3>
    <div class='guestbook-container'>
        <div class='guestbook-messages'>
            $guestbook_content
        </div>
    </div> 
    <br>";
    $guestbook_html .= '<button onclick="window.location.href=\'/\'">Return to Homepage</button>';
    $template .= $guestbook_html;
}

// Output the required headers with a double newline for CG
echo "Content-Type: text/html\n\n";

// Output the final HTML
echo $template;

?>