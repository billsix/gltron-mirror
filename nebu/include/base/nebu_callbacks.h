#ifndef NEBU_CALLBACKS_H
#define NEBU_CALLBACKS_H

typedef struct Callbacks {
  void (*display)(void);
  void (*idle)(void);
  void (*keyboard)(int, int, int, int);
  void (*init)(void);
  void (*exit)(void);
  void (*mouse)(int, int, int, int);
  void (*mouseMotion)(int, int);
  const char* name;
} Callbacks;

#endif
