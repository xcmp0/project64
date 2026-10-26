/*
 * ReactOS's ATL guards one block with portable SEH (PSEH). Without SEH in
 * this build, the blocks run as plain blocks.
 */
#pragma once

#define _SEH2_TRY {
#define _SEH2_FINALLY } {
#define _SEH2_EXCEPT(...) } if (0) {
#define _SEH2_END }
#define _SEH2_YIELD(STMT_) STMT_
#define _SEH2_LEAVE
#define _SEH2_GetExceptionCode() 0
#define _SEH2_AbnormalTermination() 0
