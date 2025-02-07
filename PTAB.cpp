#include "front_IO.h"
#include "front_module.h"
#include <queue>

struct PTAB_entry {
  bool predict_dir[FETCH_WIDTH];
  uint32_t predict_next_fetch_address;
  uint32_t predict_base_pc[FETCH_WIDTH];
};

// the FIFO control of PTAB is the same as instruction FIFO !
// the FIFO control of PTAB is the same as instruction FIFO !
// the FIFO control of PTAB is the same as instruction FIFO !
static std::queue<PTAB_entry> ptab;

void PTAB_top(struct PTAB_in *in, struct PTAB_out *out) {
  if (in->reset) {
    while (!ptab.empty()) {
      ptab.pop();
    }
    return;
  }
  if (in->refetch) {
    while (!ptab.empty()) {
      ptab.pop();
    }
  }
  // when there is new prediction, add it to PTAB
  if (in->write_enable) {
    PTAB_entry entry;
    for (int i = 0; i < FETCH_WIDTH; i++) {
      entry.predict_dir[i] = in->predict_dir[i];
      entry.predict_base_pc[i] = in->predict_base_pc[i];
    }
    entry.predict_next_fetch_address = in->predict_next_fetch_address;
    ptab.push(entry);
  }

  // output prediction
  if (in->read_enable) {
    for (int i = 0; i < FETCH_WIDTH; i++) {
      out->predict_dir[i] = ptab.front().predict_dir[i];
      out->predict_base_pc[i] = ptab.front().predict_base_pc[i];
    }
    out->predict_next_fetch_address = ptab.front().predict_next_fetch_address;
    ptab.pop();
  }
}