
// on Windows, the standard assert does not immediately break into code
// instead it shows an annoying dialog box and does NOT pause execution
// this can result in many difficulties when dealing with multiple threads
// we REALLY want assert to stop execution, so we overwrite the assert behavior
#ifdef assert
#undef assert
#endif
#define assert(expression) if (!(expression)) { __debugbreak(); }