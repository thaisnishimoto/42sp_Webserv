<?php
set_time_limit(0); // Prevent script timeout

header("Content-Type: text/plain");

echo "Starting infinite loop...\n";
flush(); // Ensure output is sent to the client

while (true) {
    echo "Still running...\n";
    flush();
    sleep(1); // Prevent CPU overuse
}
?>
