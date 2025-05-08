#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>

// Globals mirroring the C++ version
int IC, PTR, VA, RA, flag[30], PI, TI, SI;
char M[300][4], IR[4], R[4];
bool C;
FILE *fin, *fout;
char line[1000];

struct PCB {
    int job, TTL, TLL, TTC, TLC;
} pcb;

// Initialize memory, flags, registers, and PCB
void init() {
    printf("\n--- Initializing Job Setup ---\n");
    for (int i = 0; i < 300; i++)
        for (int j = 0; j < 4; j++)
            M[i][j] = '_';
    for (int i = 0; i < 30; i++)
        flag[i] = 0;
    for (int i = 0; i < 4; i++) {
        IR[i] = '-';
        R[i] = '-';
    }
    IC = VA = RA = 0;
    C = false;
    PTR = PI = TI = SI = 0;
    pcb.TTC = pcb.TLC = 0;
}

// Return a random frame between 0 and 29
int allocate() {
    int frame = rand() % 30;
    printf("Allocated frame %d\n", frame);
    return frame;
}

// Map virtual reg to real address, or return -1 on page fault
int AddMap(int reg) {
    int PTE = PTR * 10 + reg / 10;
    printf("Translating VA=%d using PTE index %d\n", reg, PTE);
    char temp[5] = {0};
    int tp = 0;
    if (M[PTE][0] == '*') {
        printf("Page fault at PTE %d\n", PTE);
        return -1;
    }
    for (int i = 0; i < 4; i++) {
        if (M[PTE][i] != ' ')
            temp[tp++] = M[PTE][i];
    }
    int frame = atoi(temp);
    int data = frame * 10 + (reg % 10);
    printf("Resolved to real address %d (frame %d offset %d)\n", data, frame, reg % 10);
    return data;
}

// Print PCB status to output file and console
void status() {
    fprintf(fout, "\nJob ID: %d\tSI: %d\tTI: %d\tPI: %d\tTTC : %d\tLLC: %d\tIR: ",
            pcb.job, SI, TI, PI, pcb.TTC, pcb.TLC);
    for (int i = 0; i < 4; i++)
        fputc(IR[i], fout);
    fprintf(fout, "\n\n");
    printf("Status dumped for Job %d\n", pcb.job);
}

// Abnormal termination banner and console message
void endprogram() {
    fprintf(fout, "\n---PROGRAM TERMINATED ABNORMALLY---\n");
    printf("Abnormal termination for Job %d\n", pcb.job);
}

// Supervisor (MOS) handling GD, PD, H, with console error prints
void MOS() {
    if (SI == 1) {
        printf("MOS: GD (Get Data) called for Job %d\n", pcb.job);
        if (!fgets(line, sizeof(line), fin)) return;
        if (strncmp(line, "$END", 4) == 0) {
            endprogram();
            fprintf(fout, "Error: Out of Data \n");
            printf("Error: Out of Data occurred for Job %d\n", pcb.job);
            return;
        }
        line[strcspn(line, "\n")] = '\0';
        int frame = allocate();
        while (flag[frame]) frame = allocate();
        flag[frame] = 1;
        int i = PTR * 10;
        while (M[i][0] != '*') i++;
        M[i][0] = ' '; M[i][1] = ' ';
        M[i][2] = '0' + frame/10;
        M[i][3] = '0' + frame%10;
        int l = 0, addr = frame * 10;
        for (int k = 0; line[k] && k < 40; k++) {
            M[addr][l++] = line[k];
            if (l == 4) { l = 0; addr++; }
        }
    }
    else if (SI == 2) {
        printf("MOS: PD (Put Data) called for Job %d\n", pcb.job);
        pcb.TLC++;
        if (pcb.TLC > pcb.TLL) {
            endprogram();
            fprintf(fout, "Error: Line Limit Exceed\n");
            printf("Error: Line Limit Exceed occurred for Job %d\n", pcb.job);
            return;
        }
        int add = (IR[2] - '0') * 10;
        RA = AddMap(add);
        if (RA != -1) {
            char out[41] = {0}; int p = 0;
            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 4; j++)
                    out[p++] = M[RA + i][j];
            fprintf(fout, "%s\n", out);
        } else {
            endprogram();
            PI = 3;
            fprintf(fout, "Error ( PI = 3 ): Invalid Page Fault\n");
            printf("Error (PI=3): Invalid Page Fault occurred for Job %d\n", pcb.job);
        }
    }
    else if (SI == 3) {
        printf("MOS: H (Halt) called for Job %d\n", pcb.job);
        fprintf(fout, "\nProgram Terminated Successfully :- \n");
        status();
        printf("Program terminated successfully for Job %d\n", pcb.job);
    }
}

// Core fetch-decode-execute with console prints
void executeProgram() {
    printf("Starting execution for Job %d\n", pcb.job);
    while (1) {
        if (PI || TI) {
            status();
            break;
        }
        RA = AddMap(IC);
        if (RA == -1) break;
        memcpy(IR, M[RA], 4);
        IC++;
        int add = (IR[2] - '0') * 10 + (IR[3] - '0');
        if ((IR[0]=='G' && IR[1]=='D') || (IR[0]=='S' && IR[1]=='R'))
            pcb.TTC += 2;
        else
            pcb.TTC += 1;
        if (pcb.TTC > pcb.TTL) {
            TI = 2;
            endprogram();
            fprintf(fout, "Error ( TI = 2 )= Time Limit Exceed\n");
            printf("Error (TI=2): Time Limit Exceed occurred for Job %d\n", pcb.job);
            status();
            break;
        }
        if (IR[0]=='L' && IR[1]=='R') {
            printf("Executing LR for Job %d\n", pcb.job);
            int raddr = AddMap(add);
            if (raddr >= 0) memcpy(R, M[raddr], 4);
        }
        else if (IR[0]=='S' && IR[1]=='R') {
            printf("Executing SR for Job %d\n", pcb.job);
            int raddr = AddMap(add);
            if (raddr >= 0) memcpy(M[raddr], R, 4);
            else MOS();
        }
        else if (IR[0]=='C' && IR[1]=='R') {
            printf("Executing CR for Job %d\n", pcb.job);
            int raddr = AddMap(add);
            if (raddr >= 0) {
                bool eq = true;
                for (int i = 0; i < 4; i++) if (R[i] != M[raddr][i]) eq = false;
                C = eq;
            } else {
                endprogram(); PI = 3;
                fprintf(fout, "Error ( PI = 3 ): Invalid Page Fault\n");
                printf("Error (PI=3): Invalid Page Fault in CR for Job %d\n", pcb.job);
            }
        }
        else if (IR[0]=='B' && IR[1]=='T') {
            printf("Executing BT for Job %d\n", pcb.job);
            if (C) IC = add;
        }
        else if (IR[0]=='G' && IR[1]=='D') { SI = 1; MOS(); }
        else if (IR[0]=='P' && IR[1]=='D') { SI = 2; MOS(); }
        else if (IR[0]=='H') { SI = 3; MOS(); break; }
        else {
            endprogram();
            fprintf(fout, "Error ( PI = 1 ): Opcode Error\n");
            printf("Error (PI=1): Opcode Error occurred for Job %d\n", pcb.job);
            status();
            break;
        }
    }
}

// Load control cards and program into memory
void load() {
    printf("Loading jobs from input.txt\n");
    while (fgets(line, sizeof(line), fin)) {
        if (strncmp(line, "$AMJ", 4) == 0) {
            printf("$AMJ found, starting new job\n");
            init();
            PTR = allocate();
            flag[PTR] = 1;
            for (int i = PTR*10; i < PTR*10+10; i++)
                for (int j = 0; j < 4; j++)
                    M[i][j] = '*';
            char id[5]={0}, ttl[5]={0}, tll[5]={0};
            memcpy(id,  line+4, 4);
            memcpy(ttl, line+8, 4);
            memcpy(tll, line+12,4);
            pcb.job = atoi(id);
            pcb.TTL = atoi(ttl);
            pcb.TLL = atoi(tll);
            printf("JobID=%d TTL=%d TLL=%d\n", pcb.job, pcb.TTL, pcb.TLL);
        }
        else if (strncmp(line, "$DTA", 4) == 0) {
            printf("$DTA found, executing program for Job %d\n", pcb.job);
            IC = 0;
            executeProgram();
        }
        else if (strncmp(line, "$END", 4) == 0) {
            printf("$END found, job %d completed\n", pcb.job);
        }
        else {
            line[strcspn(line, "\n")] = '\0';
            int frame = allocate();
            while (flag[frame]) frame = allocate();
            flag[frame] = 1;
            int p = PTR*10;
            while (M[p][0] != '*') p++;
            M[p][0] = ' ';
            M[p][1] = ' ';
            M[p][2] = '0' + frame/10;
            M[p][3] = '0' + frame%10;
            int l = 0;
            for (int k = 0; line[k]; k++) {
                int b = frame*10 + (l/4);
                M[b][l%4] = line[k];
                l++;
            }
            printf("Loaded program card into frame %d for Job %d\n", frame, pcb.job);
        }
    }
}

int main() {
    srand((unsigned)time(NULL));
    printf("OS Simulator starting...\n");
    fin  = fopen("input.txt",  "r");
    fout = fopen("output.txt", "w");
    if (!fin || !fout) {
        printf("Error opening input/output files\n");
        return EXIT_FAILURE;
    }
    load();
    fclose(fin);
    fclose(fout);
    printf("OS Simulator finished. Check output.txt\n");
    return EXIT_SUCCESS;
}
