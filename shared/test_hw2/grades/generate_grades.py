import pandas as pd
import re
import sys

if len(sys.argv) != 3:
	print("Wrong command. Please use generate_grades.py results.csv moodle_grading.csv")
	quit()

# Read the first file
df1 = pd.read_csv(str(sys.argv[1]))

# Read the second file
df2 = pd.read_csv(str(sys.argv[2]))

# Update grades and notes in the second file based on the first file
df2_updated = pd.merge(df2, df1, on='ID', how='left')

# Modify the 'notes' column in the second file
df2_updated['הערות למשוב'] = df2_updated.apply(lambda row: '<p dir="rtl" style="text-align: right;">Wet part notes: {}</p><p dir="rtl" style="text-align: right;">Dry part notes: {}</p><p dir="rtl" style="text-align: right;">Penalty notes: {}</p>'.format(re.sub(r'--newline--', '<br>', str(row['Wet Notes'])), re.sub(r'\n', '<br>', str(row['Dry Notes'])), re.sub(r'\n', '<br>', str(row['Penalty Notes']))), axis=1)

# Save the modified second file
df2_updated.to_excel('new_grade.xlsx', index=False)



#grades_hw0 = ["ID,Wet (84),Dry (0),Penalty(10),Bonus(6),Final(100),failed tests,dry notes,Penalty notes,comments\n"]
#grades_hw1 = ["ID,Wet (48),Dry (30),Penalty(7),Bonus(15),Final(100),failed tests,dry notes,Penalty notes,comments\n"]
#grades_hw2 = ["ID,Wet (90),Dry (0),Penalty(10),Bonus(0),Final(100),failed tests,dry notes,Penalty notes,comments\n"]
#grades_hw3 = ["ID,Wet (90),Dry (0),Penalty(10),Bonus(0),Final(100),failed tests,dry notes,Penalty notes,comments\n"]
#grades_hw4 = ["ID,Wet (91),Dry (0),Penalty(9),Bonus(0),Final(100),failed tests,dry notes,Penalty notes,comments\n"]