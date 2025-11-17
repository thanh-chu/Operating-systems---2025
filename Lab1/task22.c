 /*
 Table 1: run task17
 | Page size |     1 |     2 |     4 |    8 |   16 |   32 |   64 | 128 |
| --------: | ----: | ----: | ----: | ---: | ---: | ---: | ---: | --: |
|   **128** | 55421 | 22741 | 13606 | 6810 | 3121 | 1503 | 1097 | 877 |
|   **256** | 54357 | 20395 | 11940 | 4845 | 1645 |  939 |  669 | 478 |
|   **512** | 52577 | 16188 |  9458 | 2372 |  999 |  629 |  417 | 239 |
|  **1024** | 51804 | 15393 |  8362 | 1330 |  687 |  409 |  193 |  99 |

Table 3: run tast 20
| Page size   |     1 |     2 |     4 |     8 |    16 |   32 |   64 |  128 |
| ----------- | -----:| -----:| -----:| -----:| -----:| ----:| ----:| ----:|
| 128         |  55421|  16973|  11000|   6536|   1907|   995|   905|   796|
| 256         |  54357|  14947|   9218|   3811|    794|   684|   577|   417|
| 512         |  52577|  11432|   6828|   1617|    603|   503|   362|   206|
| 1024        |  51804|  10448|   5605|    758|    472|   351|   167|    99|

Which of the page replacement policies FIFO and LRU seems to give the lowest number of page faults? Explain why!
When only 1 page so both 2 measure is the same result but more than that is LRU give the lowest
number of page faults.
I think it because of the way the solve problem when the page does not exist and the page reach the limit
- with FIFO: maybe the page number that recent removed it the page that need to find after that
=> more page fault. 
- with LRU: it focus to the page number that do not have much times to use for removing so 
it less page faults than the FIFO mensure.

In some of the cases, the number of page faults are the same for both FIFO and LRU. 
Which are these cases? Why is the number of page faults equal for FIFO and LRU in those cases? Explain why!
Yes. When the total of page is 1 so it the same. With only one frame, every new page that is not currently in 
memory will replace the existing page. Since there is only one slot, both FIFO and LRU behave the same way — 
every new page causes a page fault. There is no room to take advantage of LRU’s “recently used” strategy, 
so the results are identical.

*/