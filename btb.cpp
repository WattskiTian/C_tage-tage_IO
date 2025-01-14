#include <cstdint>
#include <cstdio>
#include <sys/types.h>

#define BTB_ENTRY_NUM 2048
#define BTB_TAG_LEN 8

uint32_t btb_tag[BTB_ENTRY_NUM];
uint32_t btb_bta[BTB_ENTRY_NUM];
bool btb_valid[BTB_ENTRY_NUM];
uint32_t btb_br_type[BTB_ENTRY_NUM];

#define BR_DIRECT 0
#define BR_CALL 1
#define BR_RET 2
#define BR_IDIRECT 3

// RAS
#define RAS_ENTRY_NUM 64
#define RAS_CNT_LEN 8 // cnt for repeated call
uint32_t ras[RAS_ENTRY_NUM];
uint32_t ras_cnt[RAS_ENTRY_NUM];
uint32_t ras_sp;

// TARGET_CAHCE
#define TC_ENTRY_NUM 2048
#define BHT_ENTRY_NUM 2048
#define BHT_LEN 32
uint32_t bht[BHT_ENTRY_NUM];
uint32_t target_cache[TC_ENTRY_NUM];

uint32_t tc_pred(uint32_t pc) {
  uint32_t bht_idx = pc % BHT_ENTRY_NUM;
  uint32_t tc_idx = (bht[bht_idx] ^ pc) % TC_ENTRY_NUM;
  return target_cache[tc_idx];
}

void bht_update(uint32_t pc, bool pc_dir) {
  uint32_t bht_idx = pc % BHT_ENTRY_NUM;
  bht[bht_idx] = (bht[bht_idx] << 1) | pc_dir;
}

// if needed. firest do tc_update, then update bht
void tc_update(uint32_t pc, uint32_t actualAddr) {
  uint32_t bht_idx = pc % BHT_ENTRY_NUM;
  uint32_t tc_idx = (bht[bht_idx] ^ pc) % TC_ENTRY_NUM;
  target_cache[tc_idx] = actualAddr;
}

void ras_push(uint32_t addr) {
  if (addr == ras[ras_sp]) {
    ras_cnt[ras_sp]++;
    return;
  }
  ras_sp = (ras_sp + 1) % RAS_ENTRY_NUM;
  ras[ras_sp] = addr;
  ras_cnt[ras_sp] = 1;
}

uint32_t ras_pop() {
  if (ras_cnt[ras_sp] > 1) {
    ras_cnt[ras_sp]--;
    return ras[ras_sp];
  } else if (ras_cnt[ras_sp] == 1) {
    ras_cnt[ras_sp] = 0;
    ras_sp = (ras_sp + RAS_ENTRY_NUM - 1) % RAS_ENTRY_NUM;
    return ras[ras_sp + 1];
  } else
    return -1; // null on top
}

uint32_t btb_get_tag(uint32_t pc) { return pc & ((1 << BTB_TAG_LEN) - 1); }
uint32_t btb_get_idx(uint32_t pc) {
  /*return (pc >> BTB_TAG_LEN) % BTB_ENTRY_NUM;*/
  return pc % BTB_ENTRY_NUM;
}

// only for statistic
uint64_t dir_cnt = 0;
uint64_t call_cnt = 0;
uint64_t ret_cnt = 0;
uint64_t indir_cnt = 0;

uint32_t btb_pred(uint32_t pc) {
  uint32_t idx = btb_get_idx(pc);
  uint32_t tag = btb_get_tag(pc);
  uint32_t br_type = btb_br_type[idx];
  /*if (tag != btb_tag[idx] || btb_valid[idx] != true)*/
  /*  return -1;*/

  /*return btb_bta[idx];*/

  if (br_type == BR_DIRECT) {
    dir_cnt++;
    return btb_bta[idx];
  } else if (br_type == BR_CALL) {
    call_cnt++;
    ras_push(pc + 4);
    return btb_bta[idx];
  } else if (br_type == BR_RET) {
    ret_cnt++;
    return ras_pop();
  } else {
    indir_cnt++;
    return tc_pred(pc);
  }
}

void btb_update(uint32_t pc, uint32_t actualAddr, uint32_t br_type,
                bool actualdir) {
  uint32_t idx = btb_get_idx(pc);
  uint32_t tag = btb_get_tag(pc);
  btb_valid[idx] = true;
  btb_bta[idx] = actualAddr;
  btb_tag[idx] = tag;
  btb_br_type[idx] = br_type;
  if (br_type == BR_IDIRECT) {
    tc_update(pc, actualAddr);
  }
}

// using namespace std;

// // file data
// FILE *log_file;
// bool log_dir;
// uint32_t log_pc;
// uint32_t log_nextpc;
// uint32_t log_br_type;
// bool show_details = false;

// uint64_t line_cnt = 0;
// int readFileData() {
//   uint32_t num1, num2, num3, num4;
//   if (fscanf(log_file, "%u %x %x %u\n", &num1, &num2, &num3, &num4) == 4) {
//     /*printf("%u 0x%08x 0x%08x %u\n", num1, num2, num3, num4);*/
//     line_cnt++;
//     log_dir = (bool)num1;
//     log_pc = num2;
//     log_nextpc = num3;
//     log_br_type = num4;
//     /*printf("%u 0x%08x 0x%08x %u\n", log_dir, log_pc, log_nextpc,
//      * log_br_type);*/
//     return 0;
//   } else {
//     printf("log file END at line %lu\n", line_cnt);
//     return 1;
//   }
// }

// #define DEBUG false
// uint64_t control_cnt = 0;
// uint64_t btb_hit = 0;
// uint64_t dir_hit = 0;
// uint64_t ras_hit = 0;
// uint64_t call_hit = 0;
// uint64_t ret_hit = 0;
// uint64_t indir_hit = 0;

// int main() {
//   log_file = fopen("/home/watts/dhrystone/gem5output_rv/fronted_log", "r");
//   if (log_file == NULL) {
//     printf("log_file open error\n");
//     return 0;
//   }
//   int log_pc_max = DEBUG ? 10 : 1000000;
//   while (log_pc_max--) {
//     int log_eof = readFileData();
//     if (log_eof != 0)
//       break;

//     if (log_dir != 1)
//       continue; // not a control inst, need to coop with tage

//     control_cnt++;
//     uint32_t pred_npc = btb_pred(log_pc);
//     if (pred_npc == log_nextpc) {
//       btb_hit++;
//       if (log_br_type == BR_DIRECT) {
//         dir_hit++;
//       } else if (log_br_type == BR_CALL) {
//         call_hit++;
//       } else if (log_br_type == BR_RET) {
//         ret_hit++;
//       } else if (log_br_type == BR_IDIRECT) {
//         indir_hit++;
//       }
//       bht_update(log_pc, log_dir);
//       continue;
//     } else {
//       btb_update(log_pc, log_nextpc, log_br_type, log_dir);
//       bht_update(log_pc, log_dir);
//     }
//   }
//   fclose(log_file);

//   ras_hit = call_hit + ret_hit;
//   double btb_acc = (double)btb_hit / control_cnt;
//   printf("[version btb]   branch_cnt = %8lu  hit = %8lu  ACC = %6.3f%%\n",
//          control_cnt, btb_hit, btb_acc * 100);
//   double dir_acc = (double)dir_hit / dir_cnt;
//   printf("[version btb]      dir_cnt = %8lu  hit = %8lu  ACC = %6.3f%%\n",
//          dir_cnt, dir_hit, dir_acc * 100);
//   double call_acc = (double)call_hit / call_cnt;
//   printf("[version btb]     call_cnt = %8lu  hit = %8lu  ACC = %6.3f%%\n",
//          call_cnt, call_hit, call_acc * 100);
//   double ret_acc = (double)ret_hit / ret_cnt;
//   printf("[version btb]      ret_cnt = %8lu  hit = %8lu  ACC = %6.3f%%\n",
//          ret_cnt, ret_hit, ret_acc * 100);
//   double ras_acc = (double)ras_hit / (call_cnt + ret_cnt);
//   printf("[version btb]      ras_cnt = %8lu  hit = %8lu  ACC = %6.3f%%\n",
//          ret_cnt + call_cnt, ras_hit, ras_acc * 100);
//   double indir_acc = (double)indir_hit / indir_cnt;
//   printf("[version btb]    indir_cnt = %8lu  hit = %8lu  ACC = %6.3f%%\n",
//          indir_cnt, indir_hit, indir_acc * 100);
//   return 0;
// }
