#!/usr/bin/env python3
import cgi
import datetime

# Zodiac signs with date ranges
ZODIAC_SIGNS = {
    "Capricorn": ((12, 22), (1, 19)),
    "Aquarius": ((1, 20), (2, 18)),
    "Pisces": ((2, 19), (3, 20)),
    "Aries": ((3, 21), (4, 19)),
    "Taurus": ((4, 20), (5, 20)),
    "Gemini": ((5, 21), (6, 20)),
    "Cancer": ((6, 21), (7, 22)),
    "Leo": ((7, 23), (8, 22)),
    "Virgo": ((8, 23), (9, 22)),
    "Libra": ((9, 23), (10, 22)),
    "Scorpio": ((10, 23), (11, 21)),
    "Sagittarius": ((11, 22), (12, 21)),
}

# Positive messages for each sign
MESSAGES = {
    "Capricorn": "Hard work pays off! Keep pushing forward, and success will follow.",
    "Aquarius": "Your creativity and uniqueness make the world a better place!",
    "Pisces": "Your kindness and intuition inspire those around you. Trust your gut!",
    "Aries": "Your energy and passion drive you to greatness. Keep leading the way!",
    "Taurus": "Your patience and dedication will bring amazing rewards soon.",
    "Gemini": "Your adaptability and intelligence help you shine in any situation!",
    "Cancer": "Your empathy and caring nature make you a rock for those who need it.",
    "Leo": "Your confidence and strength inspire others—keep shining like the star you are!",
    "Virgo": "Your attention to detail and kindness make a huge difference in people's lives.",
    "Libra": "Your balance and fairness bring harmony to those around you.",
    "Scorpio": "Your determination and depth make you unstoppable!",
    "Sagittarius": "Your adventurous spirit and optimism light up every room you enter.",
}

# Determine zodiac sign based on birthdate
def get_zodiac_sign(month, day):
    for sign, ((start_month, start_day), (end_month, end_day)) in ZODIAC_SIGNS.items():
        if (month == start_month and day >= start_day) or (month == end_month and day <= end_day):
            return sign
    return "Unknown"

# Main function to handle the CGI request
def main():
    print("Content-Type: text/html\n")  # HTTP header

    form = cgi.FieldStorage()
    birthdate = form.getvalue("birthdate")  # Expecting YYYY-MM-DD format

    if birthdate:
        try:
            birth_date = datetime.datetime.strptime(birthdate, "%Y-%m-%d")
            zodiac_sign = get_zodiac_sign(birth_date.month, birth_date.day)
            message = MESSAGES.get(zodiac_sign, "You have a bright future ahead!")
            
            print(f"""
            <html>
            <head><title>Your Horoscope</title></head>
            <body>
                <h1>Your Zodiac Sign: {zodiac_sign}</h1>
                <p><strong>Positive Message:</strong> {message}</p>
                <br>
                <a href='/index.html'>Go Back</a>
            </body>
            </html>
            """)
        except ValueError:
            print("<p>Invalid date format. Please enter a valid birthdate.</p>")
    else:
        print("<p>Please enter your birthdate.</p>")

if __name__ == "__main__":
    main()