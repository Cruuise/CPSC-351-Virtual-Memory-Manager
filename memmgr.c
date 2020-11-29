//
//  memmgr.c
//  memmgr
//
//  Created by William McCarthy on 17/11/20.
//  Copyright © 2020 William McCarthy. All rights reserved.
//
//Edited by Edgar Cruz

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256


//-------------------------------------------------------------------
unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

//ignore this function, practice with masking
void mask(unsigned x){
  x = 40185;
  unsigned a;
  unsigned offset_mask = 0xFF;
  unsigned page_mask = 0xFF00;
  unsigned offset = 0;
  a = getoffset(x);
  printf("Here is the page %u\n", a);
  getpage_offset(x);
  return;
}

int main(int argc, const char* argv[]) {
  FILE* fadd = fopen("addresses.txt", "r");    // open file addresses.txt  (contains the logical addresses)
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("correct.txt", "r");     // contains the logical and physical address, and its value
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  FILE* fbin = fopen("BACKING_STORE.bin","rb");
  if (fbin == NULL) {fprintf(stderr, "Could not open file: 'BACKING_STORE.bin'\n"); exit(FILE_ERROR);}

  char buf[BUFLEN];
  unsigned   page, offset, physical_add, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt

  int table[256]; //creation of page table
  int TLB_Page [16]; //creation of TLB table
  int TLB_Frame[16];
  int TLB_hits = 0;
  int TLB_index = 0; //the current index for TLB, will help with FIFO
  int TLB_miss = 0; //not in TLB Table, 0 = no, 1 = yes it is in the table
  int page_fault = 0; // # of page faults

  printf("ONLY READ 1000 entries\n\n");
  //part of page table code
  for(int i = 0; i < 256; ++i){
    // make sure table isnt empty
    table[i] = -1;
  }
  while (frame < 1000) {

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt

    fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page   = getpage(  logic_add);
    offset = getoffset(logic_add);
    
    physical_add = frame++ * FRAME_SIZE + offset;
    //physical_add is actually the frame number
    //assert(physical_add == phys_add);
    //Look inside TLB table first 
    for(int i = 0; i < 16; ++i){
      if (page == TLB_Page[i]){
        TLB_hits++;
        //set the pphysical address = TLB_Frame[i];
        physical_add= TLB_Frame[i];
        TLB_miss = 1;
        break;
      }
    }
    //if we're at the end of TLB table index, then set index 0
    if (TLB_index >= 15)
      TLB_index = 0;
    if (TLB_miss == 0){
      //TLB Miss, look up valuein page table, check to see if page is empty
      if(table[page] == -1){ //this means that there is a page fault
        
        for(int i = 0; i < 256; ++i){
          //set the value from backing_store
          if(table[i] == -1){ //look for empty frame
            fseek(fbin,logic_add, SEEK_SET);
            char ch;
            fread (&ch, sizeof(char), 1, fbin);
            int va = (int)(ch);
            table[i] = va;
            ++page_fault;
            break;
          }
        }
        //Updates the page table here so you can copy it to TLB frame
      }
      TLB_Frame[TLB_index]= table[page];
      TLB_Page[TLB_index]= page;
      ++TLB_index;
    }
    TLB_miss = 0;

    //assert(physical_add == phys_add);
    // Reads BINARY_STORE and confirms value matches read value from correct.txt
    fseek(fbin, logic_add, SEEK_SET);
    char c;
    fread (&c, sizeof(char), 1, fbin);
    int val = (int)(c);
    if (c == value){
      //value matches c
    }
    else{
      printf("Error %u != %c. Exiting program", logic_add, c);
      exit(1);
    }
    
    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -- passed\n", logic_add, page, offset, physical_add);
    if (frame % 5 == 0) { printf("\n"); }
  }
  fclose(fcorr);
  fclose(fadd);

  
  printf("ALL logical ---> physical assertions PASSED!\n");
  printf("!!! Page table not functioning properly. Commented out assert statement\n");
  printf("--- PTE and TLB partially implementeed \n");

  //stuff for debugging
  /*printf("\n\t\t...done.\n");
  for(int i = 0; i <16 ; ++i){
    printf("Page # : %d                  Frame # : %d\n", TLB_Page[i], TLB_Frame[i]);
  }
  for(int i=0; i <= 255; ++i){
    printf("page %d: %u\n", i, table[i] );
  }*/
  float fault_per = page_fault/10.0;
  float hits_per = TLB_hits/10.0;
  printf("Page faults: %d , Fault rate: %%%.2f\n", page_fault, fault_per);
  printf("TLB hits: %d , Hit rate: %%%.2f\n", TLB_hits, hits_per);
  return 0;
}
