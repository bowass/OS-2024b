#include <stdio.h>
#include <string.h>
#define MAX_LEN 100

// given a string (next input line)
// return if it starts with exit (after skipping whitespace) 
int starts_with_exit(char* str) {
    int n = strlen(str), i = 0;
    // contains exit
    if (i+3 < n && str[i]=='e' && str[i+1]=='x' && str[i+2] == 'i' && str[i+3] == 't')
        return 1;
    return 0;
}

int main() {
    char line[MAX_LEN], *curr;
    int a, b, error, result, n;
    char op;
    // scan another query
    while (!starts_with_exit(curr = fgets(line, MAX_LEN, stdin))) {
        a = b = -1; op = '.';
        error = 0;
        /* process current string, error if:
         *  - wrong order OR
         *  - non-numeric/ops chars OR
         *  - div by 0 OR
         *  - multiple operations in a row
         */
        n = strlen(curr);
        for (int i = 0; !error && i < n; i++) {
            // skip whitespace
            if (curr[i] != ' ' && curr[i] != '\n') {
                // get a OR b
                if (curr[i] >= '0' && curr[i] <= '9') {
                    if (a == -1 && op == '.') a = curr[i] - '0';
                    else if (op != '.') {
                        b = curr[i] - '0';
                        break;
                    }
                    else error = 1;
                }
                // get operation
                else if (op == '.' && (curr[i] == '+' || curr[i] == '-' || curr[i] == '*' || curr[i] == '/'))
                    op = curr[i];
                // non-numeric/ops char
                else error = 1;
            }
        }
        // catch div by 0
        if (b == 0 && op == '/') error = 1;
        if (error) fprintf(stdout, "error\n");
        // print result: a op b
        else {
            switch (op) {
                case '+':
                    result = a + b;
                    break;
                case '-':
                    result = a - b;
                    break;
                case '*':
                    result = a * b;
                    break;
                case '/':
                    result = a / b;
                    break;
                default:
                    break;
            }
            fprintf(stdout, "%d\n", result);
        }
    }
    return 0;
}

