#include <stdlib.h>
#include <stdio.h>

int isHit(int currentPage, int* frames, int no_phys_pages){
    for(int j = 0; j < no_phys_pages; j++){
        if(frames[j]==currentPage){
            return 1;
        }
    }
    return 0;
}

int freeFrameAvailable(int currentPage, int* frames, int no_phys_pages){
    for(int j = 0; j < no_phys_pages; j++){
        if(frames[j]==-1){
            frames[j]=currentPage;
            return 1;
        }
    }
    return 0;
}

void doOptimalReplacement(int currentPage, int* frames, int no_phys_pages, int addressCount, int currentAddressIndex, int* pageAddress){
    //Använd en array, nextUse- loopa igenom alla adresser som är kvar. Finns det en page i mina frames som aldrig mer används - byt ut den. Annars - byt ut den som är längst ifrån.

    int* nextUse = malloc(sizeof(int)*no_phys_pages);
    for(int i = 0; i < no_phys_pages; i++){
        nextUse[i]=-1; //indikerar inget satt värde
    }
    //räkna ut nextUse för alla frames
    for(int i = 0; i < no_phys_pages; i++){
        for(int j = currentAddressIndex+1; j < addressCount; j++){
            if(pageAddress[j]==frames[i]){
                nextUse[i]=j;
                break;
            }
        }
        if(nextUse[i]==-1){
            nextUse[i]=0; //adressen används aldrig igen.
        }
    }
    for(int i = 0; i < no_phys_pages; i++){
        if(nextUse[i]==0){
            frames[i]=currentPage;
            return;
        }
    }
    //Byt ut den som är längst ifrån.
    int highestIndex = 0; // index för adressen längst bort
    int indexInNextUse;
    for(int i = 0; i < no_phys_pages; i++){
        if(nextUse[i]>highestIndex){
            highestIndex=nextUse[i];
            indexInNextUse = i;
        }
    }
    frames[indexInNextUse]=currentPage;
    free(nextUse);
}

int main(int argc, char *argv[])
{
    int no_phys_pages = atoi(argv[1]); //antal frames - så många platser kan hållas i minnet
    int pageSize = atoi(argv[2]); //hur stor en page är i bytes
    char *fileName = argv[3]; 



    //Öppna fil.
    FILE *file = fopen(fileName, "r");
    if (!file) {
        printf("Could not open file %s\n", fileName);
        return 1;
    }
    printf("No physical pages = %i, page size = %i\n", no_phys_pages, pageSize);

    int addresses[100000];
    int addressCount = 0; //Antal adresser

    //adding addresses from file to array addresses
    while (fscanf(file, "%d", &addresses[addressCount]) == 1) {
        addressCount++;
    }
    fclose(file);

    int pageAddress[addressCount];
    for (int i = 0; i < addressCount; i++) {
        pageAddress[i] = addresses[i] / pageSize;
    }

    int *frames = malloc(sizeof(int)*no_phys_pages);
    for(int i = 0; i < no_phys_pages; i++){
        frames[i]=-1;
    }

    int pageFaults = 0; //ökas varje gång "pagen" inte finns i minnet
    int pageHits = 0; //ökas varje gång den finns i minnet.
    


    //Loopa igenom page adresses
    for(int i = 0; i < addressCount; i++){
        int currentPage = pageAddress[i];
        
        //Plocka ut första adressen, finns den i minnet? Ja hit, nej miss.
        if(isHit(currentPage, frames, no_phys_pages)==1){
            pageHits++;
            continue;
        } else {
            pageFaults++;
        }
        
        //Finns det en ledig frame? Sätt in den på den lediga platsen och fortsätt loopen
        if(freeFrameAvailable(currentPage, frames, no_phys_pages)==1){
            continue;
        }

        //Om ej ledig frame - gör optimal replacement.
        doOptimalReplacement(currentPage, frames, no_phys_pages, addressCount,i, pageAddress);

        
        
        
    }
    
    free(frames);

    printf("Nr of pageFaults: %d \n",pageFaults);

}