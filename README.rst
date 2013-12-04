Ethernet Powerlink Mii Layer Test Scripts (using interface)

The project requires the unreleased 13.0 tools (13.0.0 Community Version).

1. Only packet number, frame delay, frame size and frame checksum are send from host application. No delays added in between frames
2. Packet Control structure modified, timeout and delay added 30mSec
3. Time stamp calculation code modified
4. Code-cleanup, code added for tx/rx on both Ethernet slices
5. New debug_print.c file changed,latest module_utils code used, code optimised to get better timestamp values
6. Bug fixes
7. code cleanup, optimized, debug prints added
8. host application updated with latest sc_xscope_support files



