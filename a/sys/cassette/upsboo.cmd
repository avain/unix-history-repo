!
! BOOTSTRAP ON UP, LEAVING SINGLE USER
!
SET DEF HEX
SET DEF LONG
SET REL:0
HALT
UNJAM
INIT
LOAD BOOT
D R10 0		! DEVICE CHOICE 0=UP
D R11 2		! 2= RB_SINGLE
START 2
