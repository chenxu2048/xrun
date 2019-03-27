#ifndef XR_PTRACE_ELF_H
#define XR_PTRACE_ELF_H

#define ELF_NIDENT 16

#define EI_MAG0 0    /* File identification byte 0 index */
#define ELFMAG0 0x7f /* Magic number byte 0 */

#define EI_MAG1 1   /* File identification byte 1 index */
#define ELFMAG1 'E' /* Magic number byte 1 */

#define EI_MAG2 2   /* File identification byte 2 index */
#define ELFMAG2 'L' /* Magic number byte 2 */

#define EI_MAG3 3   /* File identification byte 3 index */
#define ELFMAG3 'F' /* Magic number byte 3 */

#define EI_CLASS 4     /* File class byte index */
#define ELFCLASSNONE 0 /* Invalid class */
#define ELFCLASS32 1   /* 32-bit objects */
#define ELFCLASS64 2   /* 64-bit objects */
#define ELFCLASSNUM 3

#define ELF_CLASS(e_ident) ((e_ident).ident[EI_CLASS])

#define ELF_MAGIC_CHECK(e_ident)            \
  (((e_ident).ident[EI_MAG0] == ELFMAG0) && \
   ((e_ident).ident[EI_MAG1] == ELFMAG1) && \
   ((e_ident).ident[EI_MAG2] == ELFMAG2) && \
   ((e_ident).ident[EI_MAG3] == ELFMAG3))

typedef struct {
  unsigned char ident[ELF_NIDENT];
} e_ident_t;

#endif
