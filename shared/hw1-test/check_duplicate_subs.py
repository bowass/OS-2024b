import re
import sys

filename = str(sys.argv[1])

# Open the file in read mode
with open(filename, "r") as file:
    # Read the contents of the file
    contents = file.read()

# Use regular expressions to find all 9-digit strings
pattern = re.compile(r"\b\d{9}\b")
matches = pattern.findall(contents)

set_matches = set(matches)

for match in set_matches:
    print(match, matches.count(match))

