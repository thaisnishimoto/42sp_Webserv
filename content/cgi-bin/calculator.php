<?php

// Function to process the calculation
function calculate($num1, $num2, $operator) {
    switch ($operator) {
        case "add":
            return $num1 + $num2;
        case "subtract":
            return $num1 - $num2;
        case "multiply":
            return $num1 * $num2;
        case "divide":
            return ($num2 != 0) ? $num1 / $num2 : "Error: Division by zero";
        default:
            return "Error: Invalid operator";
    }
}

// Get query string parameters
$query_string = getenv("QUERY_STRING");
parse_str($query_string, $form_data);

// Process form data if available
$result = null;
if (isset($form_data['num1']) && isset($form_data['num2']) && isset($form_data['operator'])) {
    $num1 = (float) trim($form_data['num1']);
    $num2 = (float) trim($form_data['num2']);
    $operator = trim($form_data['operator']);
    $result = calculate($num1, $num2, $operator);
}

$template = <<<HTML
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Calculator</title>
    <link rel="stylesheet" href="../assets/styles.css">
</head>
<body>
    <header>
        <h2>Simple Calculator</h2>
    </header>
    <br><br>
    <form action="/cgi-bin/calculator.php" method="get">
        <label for="num1">Number 1:</label>
        <input type="number" id="num1" name="num1" required>
        <br><br>

        <label for="operator">Operator:</label>
        <select id="operator" name="operator">
            <option value="add">+</option>
            <option value="subtract">−</option>
            <option value="multiply">×</option>
            <option value="divide">÷</option>
        </select>
        <br><br>

        <label for="num2">Number 2:</label>
        <input type="number" id="num2" name="num2" required>
        <br><br>

        <button type="submit">Calculate</button>
    </form>
</body>
</html>
HTML;

// Append the result and the return button to the template
if ($result !== null) {
    $result_html = <<<HTML
    <h3>Result:</h3>
    <div><strong>$result</strong></div>
    <br><br>
    <button onclick="window.location.href='/'">Return to Homepage</button>
HTML;
    $template .= $result_html;
}

// Output the required headers for CGI
echo "Content-Type: text/html\n\n";

// Output the final HTML
echo $template;

?>