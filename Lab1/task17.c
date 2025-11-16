#include<stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char *argv[])
{
    int frames = atoi(argv[1]);
    int pageSize = atoi(argv[2]);
    char *fileName = argv[3];

    FILE *file = fopen(fileName, "r");
    if (!file) {
        printf("Could not open file %s\n", fileName);
        return 1;
    }
    printf("No physical pages = %i, page size = %i", frames, pageSize);

    int addresses[100000];
    int addressCount = 0;

    while (fscanf(file, "%d", &addresses[addressCount]) == 1) {
        addressCount++;
    }
    fclose(file);

    int pageAddress[addressCount];
    for (int i = 0; i < addressCount; i++) {
        pageAddress[i] = addresses[i] / pageSize;
    }

    int pageFaults = 0;
    bool inMemory;
    int page, frame, pages;

    pages = addressCount;

    int temp[frames];

    for(page = 0; page < pages; page++)
    {
        inMemory = false;

        for(frame = 0; frame < frames; frame++)
        {
            if(pageAddress[page] == temp[frame])
            {
                inMemory = true;
                pageFaults--;
                break;
            }
        }
        pageFaults++;

        //finns plats i minne
        if((pageFaults <= frames) && (inMemory == false))
        {
            temp[pageFaults - 1] = pageAddress[page];
        }

        //byt ut älsta
        else if(inMemory == false)
        {
            temp[(pageFaults - 1) % frames] = pageAddress[page];
        }
        // printf("\n");
        // printf("%d\t\t\t",pageAddress[page]);
        // for(frame = 0; frame < frames; frame++)
        // {
        //     if(temp[frame] != -1)
        //         printf(" %d\t\t\t", temp[frame]);
        //     else
        //         printf(" - \t\t\t");
        // }
    }

    printf("\nTotal Page Faults:\t%d\n", pageFaults);
    return 0;
}

