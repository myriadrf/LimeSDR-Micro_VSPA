#ifndef LIME_COMPILER_DEFINES
#define LIME_COMPILER_DEFINES

// Macro to conditionally disable static declaration to show specific names in memory .map file
#if 1
#define D_STATIC
#else
#define D_STATIC static
#endif

#endif // LIME_COMPILER_DEFINES