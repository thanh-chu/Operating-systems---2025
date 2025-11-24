# Operating Systems – 2025

## Viktiga datum
| Modul               | Titel                                                        | Poäng | Deadline 1  | Deadline 2  | Deadline 3  |
|---------------------|--------------------------------------------------------------|--------|--------------|--------------|--------------|
| **Laboration 1**    | Processes, Threads, Synchronization, and Memory Systems      | 1.5 hp | 2024-11-24   | 2025-03-27   | 2025-08-20   |
| **Laboration 2**    | File Systems                                                 | 1.5 hp | 2025-01-23   | 2025-03-27   | 2025-08-20   |

## 1. LAB 1:
### 1.1. Information:
Examination and grading
Present and discuss your solutions orally with a teacher or lab assistant. Once all tasks are completed:
- Prepare a compressed file (either in .tar or .zip format), including the source code for the working solutions of both Part 1 and Part 2 and submit it for evaluation. Take into account that:
  - Part 1 should contain the solutions of the following tasks Task 7, Task 9, Task 11, Task 13, and Task 14.
  - Part 2 should contain the solutions of the following tasks Task 17, Task 20, and Task 23.
- Write and submit a short report (approximately 2-3 pages, in PDF format) detailing your answers to the questions in the assignment and describing your implementations. Be sure to include responses to all questions posed in the tasks and any measurements, results, and relevant details.
---

### 1.2. Plan
#### a. Uppgifter
Vi har totalt **25 tasks** som kan dela så här:
| Person | Inlämningar | Övriga | Tasks innehåll                                                                  |
| ------ | ------------------------- | -------------------------- | -------------------------------------------------------------------------------- |
| **Ebba**  | 7,13,17            | 1, 4, 12, 18, 19                 | Matrix multiplication (seq + parallel + speedup), FIFO algorithm, analysis       |
| **Thanh**  | 9,14,20            | 2, 3, 15, 16, 21, 22             | Shared memory + semaphores, synchronization, LRU, system utilization             |
| **Signe**  | 11, 23       | 5, 6, 8, 10, 24, 25                     | Thread basics, Dining Professors (deadlock-free), Optimal algorithm, comparisons |

Innan lämna in uppgifter måste man skriva reporter:
- Each summarizes their relevant tasks: A (Task 7, 13, 17), B (Task 9, 14, 20), C (Task 11, 23) 
---

#### b. Hur vi jobbar
- Alla kan arbeta på det sätt som känns bäst.  
- När man är klar med en eller flera uppgifter → lägg upp dem på **GitHub**, så att de andra kan se.  
  (Vi behöver inte göra *merge requests*, eftersom alla arbetar i sina egna filer. Det borde därför inte bli några konflikter.)  
- När vi har visat resultaten för **Elias** och fått godkänt, lägger vi ihop de godkända delarna i en gemensam fil och fortsätter arbetet med dem resten (om det finns)
---

#### c. Veckoplan
- **Första laborationen:** Onsdag den **18 november kl. 15–17** i **G332**.
  - Mål: Ebba och Thanh visar sina delar först för Elias och frågar om raportet och extra tillfället
  - Uppdate:     
- **Andra laborationen:** Freadag den **21 november kl. 8–10** i **G332**.  
  
## 2. LAB 2:
### 1.1. Information:
- All your code should be in the files fs.h and fs.cpp, i.e., those are the only files copied to the test directory where all tests are executed.
- A general requirement of your implementation is that it should follow the C++11 standard, and correctly compile and execute on Ubuntu Linux (see Task 1). That means:
  - The tests will be compiled and executed on Ubuntu Linux, failing to do that means grade ’U’. – We will use exactly the same version of all other files that you got with the lab assignment.
  - We will use exactly the same Makefile that you got with the lab assignment.
- Paths to directories are written as ’/dir1/subdir2’, and the root directory should be ’/’ (not ’root’)
- You cannot assume that the disk (i.e., the file diskfile.bin) is formatted or formatted with your file system.
  - We have tested other file systems before, i.e., the disk may have a formatting or content that is inconsistent with your file system.
  - The first command we execute at the tests is ’format’, i.e., we format an empty disk with your file system before we execute the rest of the tests.
- Basic error checks must be done, e.g., it should not be possible to create a new file or directory with the same name as an existing file or directory, a directory and a file cannot have the same name (leads to a number of strange behaviors and errors), and the access rights on a directory must be correct for various file operations (e.g., moving/copying a file to a directory requires write access on that directory), etc.
- Access rights of a file or directory should be ’rw-’ or ’rwx’ when the file or directory is created.
- No extra or unnecessary text should be printed on the terminal, e.g., the initial help text in the provided functions that just
prints that a function is called should be removed.
To your help, we provide a file with test commands that your program should handle (test_commands.txt). In addi- tion, we have written some test programs that do some basic automatic tests for each task. The test programs are named test_script1.cpp for Task 1, etc. You compile and run them as:
```
make test1
./test1
```
### 1.2. Plan
#### a. Uppgifter
Vi har totalt **5 tasks** med flerar funktionerna som kan dela så här:
```
👤 Person 1 - FAT Manager
find_free_block()
get_chain()
free_chain()
load_fat(), save_fat()
format()

👤 Person 2 - Directory Manager
load_dir(), save_dir()
mkdir(), ls(), cd(), pwd()
resolve_path()

👤 Person 3 - File Operations
create()
cat()
append()
rm(), mv(), cp(), chmod()
```
- Written report: You should write a short report (approximately 2-3 pages) describing your implementations, with a general description of your solution, the data structures used, motivation of design decisions, etc. The format of the report must be pdf.
#### b. Hur vi jobbar
- Välj vilken del du vill jobba med.
- Ladda ner filerna från GitHub. Jag ändrar bara fs.h och fs.cpp (lägger till funktioner, tar inget bort) så att det fortfarande funkar med testfilerna.
- Implementera och testa din del lokalt, gärna med eget main. När det funkar kan du lägga upp på GitHub och säga till så att alla vet.
- Vill man kan man ha en egen mapp, men då behöver man kanske ändra i de andra filerna också. Annars kan man ha allt lokalt och på GitHub, och det brukar funka okej också.
- Man kan absolut ändra arbetssätt om något inte fungerar som förväntat.
- Vi kan ha ett gruppmöte via Zoom en gång i veckan för att diskutera och förklara för de andra vad man jobbar med. Jag tycker det är viktigt eftersom Elias kommer att fråga alla om delar man inte arbetat med vid redovisningen.
#### c. Veckoplan
- Gruppmöte vecka 48:
- Gruppmöte vecka 49: färdigt med sin del för att vara redo för redovisning vecka 50


