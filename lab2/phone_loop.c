#include <stdio.h>
#include <stdlib.h>

int main(){
    char phone[11];
    int input;
    int error = 0;
    scanf("%s", phone);
    while(scanf("%d", &input) != EOF){
        if (input == -1){
        printf("%s\n", phone);
    } 
    else if (input >= 0 && input <= 9) {
        printf("%c\n", phone[input]);
    }
    else {
        printf("ERROR\n");
        error = 1;
    }
    return error;
    }
}