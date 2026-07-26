#ifndef HSN_RUN_H
#define HSN_RUN_H

/* Full HSN run: EAL + port setup (RSS/VLAN-strip/jumbo) + workers + stats. */
int hsn_run(int argc, char const *argv[]);

#endif /* HSN_RUN_H */
