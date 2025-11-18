//  Professorer, chopsticks, tänkfunktion, tachopstickfunktion med argument häger/vänster, ätfunktion.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

struct professor {
    struct chopStick *right;
    struct chopStick *left;
    char* name; //array av chars - "sträng"
};

struct chopStick {
    pthread_mutex_t isUsedNow; //shared resource, funkar ev. inte.
    int id;
};


void think(){
    int x = (rand() % 5)+1;
    usleep(x*1000000); //suspend execution for x seconds.
}

void think2(){
    int x = (rand() % 7)+2;
    usleep(x*1000000); //suspend execution for x seconds.
}

void eat(){
    int x = (rand() % 6)+5;
    usleep(x*1000000); //suspend execution for x seconds.
}

bool pickUpChopStick(struct professor *prof, int chopId){
    if(chopId == 0){ //0 vänster, 1 höger
        printf("%s trying to pick up left chopStick\n", prof->name);
        pthread_mutex_lock(&prof->left->isUsedNow);
        printf("%s picked up left chopStick\n", prof->name);
    } else {
        printf("%s trying to pick up right chopStick\n", prof->name);
        if(pthread_mutex_trylock(&prof->right->isUsedNow)==0){
            printf("%s picked up right chopStick\n", prof->name);
        } else {
            printf("%s dropped left chopStick\n", prof->name);
            pthread_mutex_unlock(&prof->left->isUsedNow);
            return false;
        }
    }
    return true;
}

void putDownChopSticks(struct professor *prof){
    pthread_mutex_unlock(&prof->left->isUsedNow);
    pthread_mutex_unlock(&prof->right->isUsedNow);
}

void* child(void* arg) {
    struct professor *prof = arg;
    while (true){
        printf("%s thinking\n", prof->name);
        think();
        pickUpChopStick(prof, 0);
        printf("%s thinking\n", prof->name);
        think2();
        if(pickUpChopStick(prof, 1)==false){
            continue;
        }
        printf("%s eating\n", prof->name);
        eat();
        putDownChopSticks(prof);
        break;
    }
    return NULL;
}

int main(int argc, char** argv) {
    srand(time(NULL)); //seed for random time
    pthread_t *children;
    struct chopStick *chopSticks;
    struct professor *professors;
    
    chopSticks = malloc(sizeof(struct chopStick)*5); //malloc funktion som allokerar minne.
    professors = malloc(sizeof(struct professor)*5);
    children = malloc(sizeof(pthread_t)*5);

    char* name[5];
    name[0] = "Tanenbaum";
    name[1] = "Bos";
    name[2] = "Lamport";
    name[3] =  "Stallings";
    name[4] = "Silberschatz"; 

    for(int i = 0; i < 5; i++){
        chopSticks[i].id = i;
        professors[i].name = name[i];
        professors[i].left = &chopSticks[i];
        professors[i].right = &chopSticks[(i+1)%5];
        pthread_mutex_init(&chopSticks[i].isUsedNow, NULL);
    }

    for (int id = 0; id < 5; id++){
        pthread_create(&(children[id]), NULL, child, (void*)&professors[id]);
    }
    for (int id = 0; id < 5; id++){
        pthread_join(children[id], NULL);
    }

    free(children);
    free(professors);
    for(int i = 0; i < 5; i++){
        pthread_mutex_destroy(&chopSticks[i].isUsedNow);
    }
    free(chopSticks);
    return 0;

    //Allokera 2 arrays med 5 av varje chopsticks och professorer.
    //skapa objecten
    //Skapa array av pekare till trådar.
    //Skapa trådarna(skickar in plats för tråd, child och professor) så de körs automatiskt (görs vid create)
    //Joina trådarna (inväntar att alla trådar kört färdigt och stänger dem)
    //Frigör minnet
}