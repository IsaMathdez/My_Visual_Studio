// Recordando como hacer esto :D

#include <stdio.h>

int main(){

    char name[20];
    int edad;
    printf("Hola como te llamas? \n");
    fflush(stdin);
    scanf("%s", &name);
    printf("Un placer %s, que edad tienes? \n", name);
    scanf("%i", edad);

    if (edad <= 18){
        printf("%s!! Eres menor de edad! \n", name);
    } else if (edad > 18 & edad < 60){
        printf("Entonces eres un adulto ya %s verdad? \n", name);
    } else if (edad >= 60){
        printf("Pero %s!! Eres todo un ancian@ \n", name);
    } else {
        printf("Que raro, no tienes edad. \n");
    }

}

// The End