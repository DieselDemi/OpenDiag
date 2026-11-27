#include "configuration.h"
#include "globals.h"

void ConfigureOffsetValuesVID(USHORT layout) {
    if (layout > 2) return;
#define VID(field, a, b, c) field = layout == 0 ? a : layout == 1 ? b : c;
#include "native/vid_fields.inc"
#undef VID
}
