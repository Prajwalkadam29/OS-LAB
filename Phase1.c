#include <stdio.h>    // For standard I/O functions like printf, fopen, fgetc, etc.
#include <stdlib.h>   // For exit() function used for error handling.
#include <string.h>   // For string functions such as strlen and strncmp.

// Global Declarations and Definitions:

int flag = 0;

int currentLine;

char Memory_block[100][4];

char Instruction_register[4];

char General_purpose_register[4];

int Instruction_counter;

int Toggle_register;

int System_interrupt;


// This function prints the contents of the simulated memory to the console.
void print_memory_block() {
    printf("Memory block is:\n\n");
    for (int i = 0; i < 100; i++) {

        if (i >= 10)
            printf("%d  ", i);
        else
            printf("%d   ", i);

        for (int j = 0; j < 4; j++) {
            printf("%c", Memory_block[i][j]);
        }
        printf("\n");
    }
}

// This function prints the contents of the Instruction_register.
void print_instruction_register() {
    for (int i = 0; i < 4; i++) {
        printf("%c", Instruction_register[i]);
    }
    printf("\n");
}

// This function prints the contents of the General_purpose_register.
void print_general_purpose_register() {
    for (int i = 0; i < 4; i++) {
        printf("%c", General_purpose_register[i]);
    }
    printf("\n");
}


// This function initializes (or resets) the memory block, registers,
void INIT() {
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 4; j++) {
            Memory_block[i][j] = ' ';
        }
    }
    for (int i = 0; i < 4; i++) {
        Instruction_register[i] = ' ';
        General_purpose_register[i] = ' ';
    }
    Instruction_counter = 0;
    System_interrupt = 0;
    Toggle_register = 0;
    currentLine = 0;  // Reset the file position marker.
}


void START_EXECUTION();
void EXECUTE_USER_PROGRAM();
void MOS();
void GD();
void PD();
void H();
void LR();
void SR();
void CR();
void BT();


void Load() {
    int i = 0;          // Counter for the number of jobs processed.
    FILE *fp = NULL;
    char ch;

    fp = fopen("input.txt", "r");
    if (fp == NULL) {
        printf("File not found");
        exit(1);
    }

    char str[100];  // Buffer to hold lines read from the file.

    // Read each line from the file until the end is reached.
    while (fgets(str, 41, fp) != NULL) {

        if (str[0]=='$' && str[1]=='A' && str[2]=='M' && str[3]=='J') {
            int row = 0;
            int col = 0;

            // Read subsequent lines until the "$DTA" marker (data section) is encountered.
            while (1) {
                fgets(str, 100, fp);

                if ((str[0]=='$' && str[1]=='D' && str[2]=='T' && str[3]=='A'))
                    break;

                for (int i = 0; i < (int)strlen(str) - 1; i++) {

                    if (str[i]==' ')
                        break;
                    Memory_block[row][col] = str[i];
                    col++;       // Move to next column.

                    // When a row (4 characters) is full, move to the next row.
                    if (col == 4) {
                        row++;
                        col = 0;
                    }
                }
            }
            // Save the current file position. This position is used to resume reading data.
            currentLine = ftell(fp);
        }

        // If the line starts with "$DTA", this signals the start of the data section.
        if (str[0]=='$' && str[1]=='D' && str[2]=='T' && str[3]=='A') {
            START_EXECUTION();
        }

        // If the line starts with "$END", the job is finished.
        if (str[0]=='$' && str[1]=='E' && str[2]=='N' && str[3]=='D') {
            i++;
            print_memory_block();
            printf("\n\nEND OF JOB no. %d\n\n\n", i);
            printf("Press Enter to load another job:");
            getchar();
            INIT();
        }
    }
    printf("All jobs are completed!");
    printf("\nExiting...");
    fclose(fp);
}


// Resets the Instruction_counter and calls the main execution function.
void START_EXECUTION() {
    Instruction_counter = 0;
    EXECUTE_USER_PROGRAM();
}


void EXECUTE_USER_PROGRAM() {
    while (1) {
        // Fetch the current instruction (4 characters) from the memory block.
        for (int i = 0; i < 4; i++) {
            Instruction_register[i] = Memory_block[Instruction_counter][i];
        }
        Instruction_counter++;       // Move to the next instruction.


        if (Instruction_register[0]=='G' && Instruction_register[1]=='D') {
            System_interrupt = 1;
            MOS();
        }
        else if (Instruction_register[0]=='P' && Instruction_register[1]=='D') {
            System_interrupt = 2;
            MOS();
        }
        else if (Instruction_register[0]=='H' && Instruction_register[1]==' ') {
            System_interrupt = 3;
            MOS();
            break;
        }
        else if (Instruction_register[0]=='L' && Instruction_register[1]=='R') {
            LR();
        }
        else if (Instruction_register[0]=='S' && Instruction_register[1]=='R') {
            SR();
        }
        else if (Instruction_register[0]=='B' && Instruction_register[1]=='T') {
            BT();
        }
        else if (Instruction_register[0]=='C' && Instruction_register[1]=='R') {
            CR();
        }
    }
}

// Based on the System_interrupt flag, this function dispatches the appropriate service.
void MOS() {
    if (System_interrupt == 1) {
        GD();
    }
    else if (System_interrupt == 2) {
        PD();
    }
    else if (System_interrupt == 3) {
        H();
    }
}


void GD() {
    int row = (Instruction_register[2]-'0')*10 + (Instruction_register[3]-'0');
    int col = 0;
    char str[100];
    FILE *fp = NULL;

    fp = fopen("input.txt", "r");
    if (fp == NULL) {
        printf("File not found");
        exit(1);
    }

    fseek(fp, currentLine, SEEK_SET);


    if (fgets(str, 100, fp) != NULL) {
        // Process each character in the data line (ignoring newline at end).
        for (int i = 0; i < (int)strlen(str) - 1; i++) {
            Memory_block[row][col] = str[i];
            col++;
            // When 4 characters have been written, move to the next row.
            if (col == 4) {
                row++;
                col = 0;
            }
        }
        // Update currentLine to the new file position so that the next GD call continues where this left off.
        currentLine = ftell(fp);
    } else {
        printf("Error reading data from file.\n");
    }
    fclose(fp);
}


void PD() {
    FILE *fp = NULL;
    // Open the output file in append mode.
    fp = fopen("output.txt", "a");
    if (fp == NULL) {
        perror("Error opening output file");
        return;
    }

    // Check flag to decide if a newline should be added.
    if (flag == 0)
        flag++;
    else
        fprintf(fp, "\n");

    // Compute the starting row from the instruction.
    int row = (Instruction_register[2]-'0')*10 + (Instruction_register[3]-'0');
    int col = 0;
    // Loop over 10 rows starting at the computed row.
    for (int i = row; i < row + 10; i++) {
        for (int j = 0; j < 4; j++) {
            // Stop printing if an underscore is encountered, which might denote the end of meaningful data.
            if (Memory_block[i][j] == '_')
                break;
            // Write each character to the output file.
            fputc(Memory_block[i][j], fp);
        }
    }
    fclose(fp);
}


void H() {
    FILE *fp = NULL;
    fp = fopen("output.txt", "a");
    if (fp == NULL) {
        perror("Error opening output file");
        return;
    }
    fputc('\n', fp);
    fputc('\n', fp);
    fclose(fp);
}


void LR() {
    int row = (Instruction_register[2]-'0')*10 + (Instruction_register[3]-'0');
    // Copy 4 characters from memory into the register.
    for (int i = 0; i < 4; i++) {
        General_purpose_register[i] = Memory_block[row][i];
    }
}


void SR() {
    int row = (Instruction_register[2]-'0')*10 + (Instruction_register[3]-'0');
    for (int i = 0; i < 4; i++) {
        Memory_block[row][i] = General_purpose_register[i];
    }
}


void CR() {
    int row = (Instruction_register[2]-'0')*10 + (Instruction_register[3]-'0');
    // Initialize Toggle_register to 1 assuming a match.
    Toggle_register = 1;
    // Compare each character in the register with the corresponding memory location.
    for (int i = 0; i < 4; i++) {
        if (General_purpose_register[i] != Memory_block[row][i]) {
            Toggle_register = 0;
            break;         // Exit loop as soon as a mismatch is found.
        }
    }
}


void BT() {
    if (Toggle_register == 1) {
        Instruction_counter = (Instruction_register[2]-'0')*10 + (Instruction_register[3]-'0');
        Toggle_register = 0;       // Reset the toggle after branching.
    }
}


// The entry point of the program. It performs the following steps:
// 1. Reads and displays the content of "input.txt" so the user can see the job.
// 2. Initializes the system state.
// 3. Loads and executes jobs from the input file.
// 4. Exits when all jobs are processed.
int main() {
    FILE *f = NULL;
    char ch;

    // Open the input file in read mode.
    f = fopen("input.txt", "r");
    if (f == NULL) {
        // If the file cannot be found, display an error message and exit.
        printf("File not found");
        exit(1);
    }

    // Read and print the entire contents of input.txt for preview.
    while ((ch = fgetc(f)) != EOF) {
        printf("%c", ch);
    }
    fclose(f);

    // Prompt the user before proceeding.
    printf("\n\nPress Enter to continue: ");
    getchar();

    // Initialize system state (clear memory and registers).
    INIT();

    // Load the job(s) from the input file and start execution.
    Load();

    return 0;
}
