#include <stdio.h>
#include <stdlib.h>

int main(){
    char phone[11];
    int input;
    scanf("%s %d", phone, &input);
    if (input == -1) {
        printf("%s\n", phone);
    } 
    else if (input >= 0 && input <= 9) {
        printf("%c\n", phone[input]);
    }
    else {
        printf("ERROR\n");
        return 1;
    }
    return 0;
}